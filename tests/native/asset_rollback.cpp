#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../../src/app/update/AssetRollback.h"

using namespace AssetRollback;
struct PowerCut {};
struct Flash : Io {
  std::vector<uint8_t> bytes = std::vector<uint8_t>(0x400000, 0xff);
  uint32_t base[3] = {0x10000, 0x160000, 0x2B0000};
  int cut = -1, operations = 0;
  size_t torn = SIZE_MAX;
  bool failRead = false, failBoot = false;
  uint8_t bootSlot = 0;
  unsigned writes = 0;
  uint32_t address(uint8_t slot, uint32_t offset, size_t size) {
    assert(slot < 3 && size <= (slot == 2 ? kAssetSize : 0x150000));
    assert(offset <= (slot == 2 ? kAssetSize : 0x150000) - size);
    return base[slot] + offset;
  }
  bool read(uint8_t slot, uint32_t offset, void *data, size_t size) override {
    if (failRead) return false;
    memcpy(data, bytes.data() + address(slot, offset, size), size); return true;
  }
  bool mutate(uint8_t slot, uint32_t offset, const void *data, size_t size, bool erase) {
    const auto addr = address(slot, offset, size);
    assert(slot == kAssets || offset >= kAppLimit);
    const bool stop = operations++ == cut;
    const size_t count = stop ? std::min(size, torn) : size;
    for (size_t i = 0; i < count; ++i) {
      if (erase) bytes[addr + i] = 0xff;
      else {
        const uint8_t value = static_cast<const uint8_t *>(data)[i];
        assert((bytes[addr + i] & value) == value);
        bytes[addr + i] &= value;
      }
    }
    ++writes;
    if (stop) throw PowerCut{};
    return true;
  }
  bool write(uint8_t s, uint32_t o, const void *d, size_t n) override { return mutate(s,o,d,n,false); }
  bool erase(uint8_t s, uint32_t o, size_t n) override { return mutate(s,o,nullptr,n,true); }
  bool digest(const void *data, size_t n, uint8_t out[32]) override {
    return BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
        (PUCHAR)data, static_cast<ULONG>(n), out, 32) == 0;
  }
  bool hash(uint8_t s, uint32_t o, size_t n, uint8_t out[32]) override {
    return !failRead && digest(bytes.data() + address(s,o,n), n, out);
  }
  bool appIdentity(uint8_t s, uint8_t out[32]) override { return hash(s,0,1024,out); }
  bool boot(uint8_t s) override { if(failBoot) return false; bootSlot=s; return true; }
  void service() override {}
};

void runRollbackTests() {
  unsigned scenarios = 0;
  for (uint8_t source = 0; source < 2; ++source) {
    const uint8_t target = source ^ 1;
    Flash initial;
    initial.bootSlot = source;
    for (uint32_t i = 0; i < 1024; ++i) {
      initial.bytes[initial.base[source]+i] = static_cast<uint8_t>(i);
      initial.bytes[initial.base[target]+i] = static_cast<uint8_t>(i ^ 0x77);
    }
    std::vector<uint8_t> oldAssets(kAssetSize), newAssets(kAssetSize);
    for (size_t i = 0; i < kAssetSize; ++i) {
      oldAssets[i] = uint8_t(i * 37); newAssets[i] = uint8_t(i * 17 + 3);
    }
    memcpy(initial.bytes.data()+initial.base[2], oldAssets.data(), kAssetSize);
    uint8_t appHash[32], assetsHash[32];
    assert(initial.hash(target,0,1024,appHash));
    assert(initial.digest(newAssets.data(),newAssets.size(),assetsHash));
    assert(!Engine(initial).sameVerifiedAppPair(source));
    Flash identical=initial;
    memcpy(identical.bytes.data()+identical.base[target],
           identical.bytes.data()+identical.base[source],1024);
    assert(Engine(identical).sameVerifiedAppPair(source));
    assert(!Engine(identical).sameVerifiedAppPair(2));
    identical.failRead=true;
    assert(!Engine(identical).sameVerifiedAppPair(source));
    const auto transaction = [&](Flash &f, bool confirm) {
      Engine engine(f);
      assert(engine.prepare(source,target,1024,appHash,assetsHash));
      assert(f.erase(2,0,kAssetSize));
      for (uint32_t i=0;i<kAssetSize;i+=512) assert(f.write(2,i,newAssets.data()+i,512));
      assert(engine.verifyAssets(target));
      f.bootSlot=target; // Model successful Update.end only here.
      assert(engine.recover(target)==Result::NewReady);
      if(confirm) assert(engine.confirm(target));
    };
    Flash success=initial;
    transaction(success,true);
    assert(Engine(success).idle());
    const int operationCount=success.operations;
    for (int cut=0;cut<operationCount;++cut) {
      for (size_t torn : {size_t(0),size_t(1),size_t(100),size_t(187),SIZE_MAX}) {
        Flash f=initial; f.cut=cut; f.torn=torn;
        try { transaction(f,true); } catch(const PowerCut &) {}
        f.cut=-1;
        Engine engine(f);
        auto result=engine.recover(f.bootSlot);
        if(result==Result::Reboot) result=engine.recover(f.bootSlot);
        if(result==Result::NewReady) assert(engine.confirm(f.bootSlot));
        assert(result!=Result::Error);
        const auto &expected=f.bootSlot==source ? oldAssets:newAssets;
        assert(std::equal(expected.begin(),expected.end(),f.bytes.begin()+f.base[2]));
        // No writes to NVS, history, bootloader, partition table or either app.
        for(size_t i=0;i<f.bytes.size();++i) {
          const bool mutableArea=(i>=f.base[target]+kAppLimit && i<f.base[target]+0x150000) ||
                                  (i>=f.base[2] && i<f.base[2]+kAssetSize);
          if(!mutableArea) assert(f.bytes[i]==initial.bytes[i]);
        }
        ++scenarios;
      }
    }
    Flash pending=initial;
    assert(Engine(pending).prepare(source,target,1024,appHash,assetsHash));
    assert(!Engine(pending).idle());
    assert(!Engine(pending).sameVerifiedAppPair(source));
    assert(!Engine(pending).prepare(source,target,1024,appHash,assetsHash));
    assert(pending.erase(2,0,kAssetSize));
    Flash restored=pending;
    assert(Engine(restored).recover(source)==Result::Restored);
    const int restoreOperations=restored.operations-pending.operations;
    for(int i=0;i<restoreOperations;++i) {
      Flash f=pending; f.cut=f.operations+i; f.torn=100;
      try { Engine(f).recover(source); } catch(const PowerCut &) {}
      f.cut=-1;
      assert(Engine(f).recover(source)!=Result::Error);
      assert(std::equal(oldAssets.begin(),oldAssets.end(),f.bytes.begin()+f.base[2]));
      ++scenarios;
    }
    Flash invalid=pending;
    invalid.bytes[invalid.base[target]+kAppLimit+offsetof(Record,checksum)]^=1;
    assert(Engine(invalid).recover(source)==Result::Error);
    invalid=pending;
    invalid.bytes[invalid.base[target]+kBackupOffset]^=1;
    auto before=invalid.bytes;
    assert(Engine(invalid).recover(source)==Result::Error);
    assert(invalid.bytes==before);
    invalid=pending; invalid.failBoot=true;
    assert(Engine(invalid).recover(target)==Result::Error);
    // Activation can have selected the target while the old app is still
    // executing. A runtime abort must reset the next boot before marking the
    // restored pair terminal, even when the source is the current app.
    invalid=pending; invalid.bootSlot=target;
    assert(Engine(invalid).recover(source)==Result::Restored);
    assert(invalid.bootSlot==source);
    invalid=pending; invalid.bootSlot=target; invalid.failBoot=true;
    assert(Engine(invalid).recover(source)==Result::Error);
    assert(!Engine(invalid).idle());
    invalid.failBoot=false;
    assert(Engine(invalid).recover(source)==Result::Restored);
    assert(invalid.bootSlot==source);
    invalid=pending; invalid.failRead=true;
    assert(Engine(invalid).recover(source)==Result::Error);
    invalid=pending;
    invalid.bytes[invalid.base[source]]^=1;
    assert(Engine(invalid).recover(source)==Result::Error);
    // A verified new pair remains protected until confirmation; bad new assets
    // make a running target return to the known source pair.
    invalid=initial; transaction(invalid,false);
    invalid.bytes[invalid.base[2]]^=1;
    assert(Engine(invalid).recover(target)==Result::Reboot);
    assert(invalid.bootSlot==source);
    assert(Engine(invalid).recover(source)==Result::Restored);
    invalid=initial;
    assert(!Engine(invalid).prepare(source,target,kAppLimit+1,appHash,assetsHash));
    assert(invalid.writes==0);
    assert(Engine(invalid).prepare(source,target,kAppLimit,appHash,assetsHash));
    assert(!Engine(invalid).verifyAssets(target));
    // Torn append must not replace the last valid READY record.
    memset(invalid.bytes.data()+invalid.base[target]+kAppLimit+kStride,0x11,kJournalSize-kStride);
    assert(Engine(invalid).recover(source)==Result::Error); // no room to commit
    ++scenarios;
  }
  printf("PASS: %u rollback scenarios, real SHA-256, both slots and repeated power cuts\n",scenarios);
}
int main() { runRollbackTests(); }
