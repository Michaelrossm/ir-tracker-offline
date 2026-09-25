// Included by main.cpp; all OTA callers share this adapter and engine.
// ESP-IDF protects the whole running app partition, including its unused tail.
// Permit only this task's verified journal write, not app bytes or other callers.
static const esp_flash_os_functions_t *journalOriginalOs = nullptr;
static esp_flash_os_functions_t journalWriteOs;
static TaskHandle_t journalWriteTask = nullptr;
static size_t journalWriteAddress = 0, journalWriteSize = 0;
static esp_err_t journalRegionCheck(void *context, size_t address, size_t size) {
  if (journalWriteTask == xTaskGetCurrentTaskHandle() &&
      address == journalWriteAddress && size == journalWriteSize)
    return ESP_OK;
  return journalOriginalOs->region_protected(context, address, size);
}

class AssetRollbackIo : public AssetRollback::Io {
 public:
  bool ready() {
    debugStorage.checkLayout();
    if (!debugStorage.fixedLayoutValid()) return false;
    for (uint8_t i = 0; i < 2; ++i)
      slots_[i] = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
          static_cast<esp_partition_subtype_t>(ESP_PARTITION_SUBTYPE_APP_OTA_0 + i), nullptr);
    slots_[2] = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS, debugStorage.observedTargetLabel());
    for (auto *slot : slots_) if (!slot || slot->encrypted) return false;
    return true;
  }
  uint8_t running() const {
    const auto *current = esp_ota_get_running_partition();
    for (uint8_t i = 0; i < 2; ++i)
      if (current && slots_[i] && current->address == slots_[i]->address) return i;
    return 255;
  }
  bool updateTargetValid() const {
    const uint8_t current = running();
    const auto *next = esp_ota_get_next_update_partition(nullptr);
    return current < 2 && next && slots_[current ^ 1U] &&
        next->address == slots_[current ^ 1U]->address && next->size == 0x150000U;
  }
  bool read(uint8_t slot, uint32_t offset, void *data, size_t size) override {
    return bounds(slot, offset, size) &&
        esp_partition_read(slots_[slot], offset, data, size) == ESP_OK;
  }
  bool write(uint8_t slot, uint32_t offset, const void *data, size_t size) override {
    if (!writable(slot, offset, size)) return false;
    if (slot != running())
      return esp_partition_write(slots_[slot], offset, data, size) == ESP_OK;
    if (!size || offset < AssetRollback::kAppLimit ||
        offset >= AssetRollback::kBackupOffset ||
        size > AssetRollback::kBackupOffset - offset || journalWriteTask)
      return false;
    esp_image_metadata_t image = {};
    const esp_partition_pos_t position = {slots_[slot]->address, slots_[slot]->size};
    if (esp_image_verify(ESP_IMAGE_VERIFY, &position, &image) != ESP_OK ||
        image.image_len > AssetRollback::kAppLimit ||
        !esp_flash_default_chip || !esp_flash_default_chip->os_func ||
        !esp_flash_default_chip->os_func->region_protected) return false;
    journalOriginalOs = esp_flash_default_chip->os_func;
    journalWriteOs = *journalOriginalOs;
    journalWriteOs.region_protected = journalRegionCheck;
    journalWriteAddress = slots_[slot]->address + offset;
    journalWriteSize = size;
    journalWriteTask = xTaskGetCurrentTaskHandle();
    esp_flash_default_chip->os_func = &journalWriteOs;
    const esp_err_t result = esp_partition_write(slots_[slot], offset, data, size);
    esp_flash_default_chip->os_func = journalOriginalOs;
    journalWriteTask = nullptr;
    return result == ESP_OK;
  }
  bool erase(uint8_t slot, uint32_t offset, size_t size) override {
    if (!writable(slot, offset, size)) return false;
    if (slot == AssetRollback::kAssets)
      return offset == 0 && size == AssetRollback::kAssetSize && debugStorage.beginRawAssetUpdate();
    return esp_partition_erase_range(slots_[slot], offset, size) == ESP_OK;
  }
  bool hash(uint8_t slot, uint32_t offset, size_t size, uint8_t out[32]) override {
    if (!bounds(slot, offset, size)) return false;
    mbedtls_sha256_context context;
    mbedtls_sha256_init(&context);
    bool ok = mbedtls_sha256_starts_ret(&context, 0) == 0;
    uint8_t buffer[512];
    while (ok && size) {
      const size_t count = std::min(size, sizeof(buffer));
      ok = read(slot, offset, buffer, count) &&
          mbedtls_sha256_update_ret(&context, buffer, count) == 0;
      offset += count; size -= count;
      service();
    }
    if (ok) ok = mbedtls_sha256_finish_ret(&context, out) == 0;
    mbedtls_sha256_free(&context);
    return ok;
  }
  bool digest(const void *data, size_t size, uint8_t out[32]) override {
    return mbedtls_sha256_ret(static_cast<const uint8_t *>(data), size, out, 0) == 0;
  }
  bool appIdentity(uint8_t slot, uint8_t out[32]) override {
    return slot < 2 && slots_[slot] && esp_partition_get_sha256(slots_[slot], out) == ESP_OK;
  }
  bool boot(uint8_t slot) override {
    return slot < 2 && slots_[slot] && esp_ota_set_boot_partition(slots_[slot]) == ESP_OK;
  }
  void service() override {
    esp_task_wdt_reset();
    if (uartReady) serviceMeterInput();
    // ESP32-C3 is single-core. Yield a full RTOS tick so the TCP/IP task can
    // drain the socket during asset hashing/backup/rollback work.
    delay(1);
  }
  bool uartReady = false;
 private:
  const esp_partition_t *slots_[3] = {};
  bool bounds(uint8_t slot, uint32_t offset, size_t size) const {
    return slot < 3 && slots_[slot] && offset <= slots_[slot]->size &&
        size <= slots_[slot]->size - offset;
  }
  bool writable(uint8_t slot, uint32_t offset, size_t size) const {
    return bounds(slot, offset, size) &&
        (slot == AssetRollback::kAssets || offset >= AssetRollback::kAppLimit);
  }
} assetRollbackIo;
AssetRollback::Engine assetRollback(assetRollbackIo);
const char *assetRollbackStatus = "not_checked";
bool assetRollbackBlocked = false;

bool recoverAssetTransaction() {
  if (!assetRollbackIo.ready()) {
    assetRollbackStatus = "layout_unsupported";
    assetRollbackBlocked = true;
    return false;
  }
  const auto result = assetRollback.recover(assetRollbackIo.running());
  assetRollbackBlocked = result == AssetRollback::Result::Error;
  switch (result) {
    case AssetRollback::Result::Clean: assetRollbackStatus = "clean"; break;
    case AssetRollback::Result::Restored: assetRollbackStatus = "restored"; break;
    case AssetRollback::Result::NewReady: assetRollbackStatus = "new_ready"; break;
    case AssetRollback::Result::Reboot:
      assetRollbackStatus = "reboot_old_app";
      ESP.restart(); return false;
    default: assetRollbackStatus = "recovery_failed"; break;
  }
  return !assetRollbackBlocked;
}
