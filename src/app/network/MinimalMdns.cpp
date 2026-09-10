// IR Tracker specific IPv4 mDNS responder.
// Included by main.cpp inside its private namespace.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

namespace MinimalMdns {

constexpr uint16_t kPort = 5353;
constexpr uint32_t kUniqueTtl = 120;
constexpr uint32_t kPtrTtl = 4500;
constexpr uint32_t kRefreshMs = 60UL * 1000UL;
constexpr uint32_t kProbeStepMs = 250;
constexpr uint8_t kProbeCount = 3;
constexpr size_t kPacketBytes = 1200;

constexpr uint16_t kTypeA = 1;
constexpr uint16_t kTypePtr = 12;
constexpr uint16_t kTypeTxt = 16;
constexpr uint16_t kTypeSrv = 33;
constexpr uint16_t kTypeAny = 255;
constexpr uint16_t kClassIn = 1;
constexpr uint16_t kCacheFlush = 0x8000;
constexpr uint16_t kFlagResponseAuthoritative = 0x8400;

constexpr char kGroup[] = "224.0.0.251";
constexpr char kServiceEnumeration[] = "_services._dns-sd._udp.local";

enum class ServiceKind : uint8_t {
  Http,
  IrTracker,
  Modbus,
  Shelly,
  Everhome,
};

struct ServiceSpec {
  const char *type;
  uint16_t port;
  ServiceKind kind;
};

constexpr ServiceSpec kServices[] = {
    {"_http._tcp.local", 80, ServiceKind::Http},
    {"_irtracker._tcp.local", 80, ServiceKind::IrTracker},
    {"_modbus._tcp.local", 502, ServiceKind::Modbus},
    {"_shelly._tcp.local", 80, ServiceKind::Shelly},
    {"_everhome._tcp.local", 80, ServiceKind::Everhome},
};

struct State {
  int socket = -1;
  bool active = false;
  bool announced = false;
  bool modbus = false;
  bool probing = false;
  uint8_t probesSent = 0;
  uint32_t nextProbeMs = 0;
  uint32_t nextRefreshMs = 0;

  IPAddress ip;
  char baseHostname[64] = {};
  char hostname[64] = {};
  char hostFqdn[72] = {};
  char instance[64] = {};
  char serial[24] = {};
  char model[32] = {};
  char shellyModel[40] = {};
  char shellyId[32] = {};
  char suffix[8] = {};
} state;

uint8_t rxPacket[kPacketBytes];
uint8_t txPacket[kPacketBytes];

static uint16_t read16(const uint8_t *p) {
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

static void put16(uint8_t *p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value >> 8);
  p[1] = static_cast<uint8_t>(value);
}

static void put32(uint8_t *p, uint32_t value) {
  p[0] = static_cast<uint8_t>(value >> 24);
  p[1] = static_cast<uint8_t>(value >> 16);
  p[2] = static_cast<uint8_t>(value >> 8);
  p[3] = static_cast<uint8_t>(value);
}

static uint32_t ipToNetwork(const IPAddress &ip) {
  const uint32_t host =
      (static_cast<uint32_t>(ip[0]) << 24) |
      (static_cast<uint32_t>(ip[1]) << 16) |
      (static_cast<uint32_t>(ip[2]) << 8) |
      static_cast<uint32_t>(ip[3]);
  return htonl(host);
}

static bool sameIp(const uint8_t *bytes, const IPAddress &ip) {
  return bytes[0] == ip[0] && bytes[1] == ip[1] &&
         bytes[2] == ip[2] && bytes[3] == ip[3];
}

static bool serviceEnabled(const ServiceSpec &service) {
  return service.kind != ServiceKind::Modbus || state.modbus;
}

static bool nameEquals(const char *a, const char *b) {
  return a && b && strcasecmp(a, b) == 0;
}

// Read a DNS name, including compressed names. offset advances past the
// encoded name at its original location.
static bool readName(const uint8_t *data, size_t length, size_t &offset,
                     char *out, size_t outSize) {
  if (!outSize || offset >= length) return false;
  size_t cursor = offset;
  size_t outPos = 0;
  bool jumped = false;
  size_t jumps = 0;

  while (cursor < length && jumps <= 12) {
    const uint8_t label = data[cursor++];
    if (label == 0) {
      if (!jumped) offset = cursor;
      out[outPos] = 0;
      return true;
    }
    if ((label & 0xc0) == 0xc0) {
      if (cursor >= length) return false;
      const uint16_t pointer =
          static_cast<uint16_t>(((label & 0x3f) << 8) | data[cursor++]);
      if (pointer >= length) return false;
      if (!jumped) offset = cursor;
      cursor = pointer;
      jumped = true;
      ++jumps;
      continue;
    }
    if ((label & 0xc0) || label > 63 || cursor + label > length)
      return false;
    if (outPos) {
      if (outPos + 1 >= outSize) return false;
      out[outPos++] = '.';
    }
    if (outPos + label >= outSize) return false;
    memcpy(out + outPos, data + cursor, label);
    outPos += label;
    cursor += label;
  }
  return false;
}

struct Builder {
  uint8_t *data;
  size_t capacity;
  size_t length = 12;
  uint16_t answers = 0;

  explicit Builder(uint8_t *buffer, size_t size, uint16_t id = 0)
      : data(buffer), capacity(size) {
    memset(data, 0, capacity);
    put16(data, id);
    put16(data + 2, kFlagResponseAuthoritative);
  }

  bool room(size_t count) const { return length + count <= capacity; }

  bool raw(const void *source, size_t count) {
    if (!room(count)) return false;
    memcpy(data + length, source, count);
    length += count;
    return true;
  }

  bool u8(uint8_t value) {
    if (!room(1)) return false;
    data[length++] = value;
    return true;
  }

  bool u16(uint16_t value) {
    if (!room(2)) return false;
    put16(data + length, value);
    length += 2;
    return true;
  }

  bool u32(uint32_t value) {
    if (!room(4)) return false;
    put32(data + length, value);
    length += 4;
    return true;
  }

  bool name(const char *fqdn) {
    const char *cursor = fqdn;
    while (*cursor) {
      const char *dot = strchr(cursor, '.');
      const size_t labelLength =
          dot ? static_cast<size_t>(dot - cursor) : strlen(cursor);
      if (!labelLength || labelLength > 63 || !room(labelLength + 1))
        return false;
      data[length++] = static_cast<uint8_t>(labelLength);
      memcpy(data + length, cursor, labelLength);
      length += labelLength;
      if (!dot) break;
      cursor = dot + 1;
    }
    return u8(0);
  }

  bool rrHeader(const char *owner, uint16_t type, uint16_t rrClass,
                uint32_t ttl, size_t &rdLengthOffset) {
    if (!name(owner) || !u16(type) || !u16(rrClass) || !u32(ttl) || !room(2))
      return false;
    rdLengthOffset = length;
    length += 2;
    return true;
  }

  bool finishRdata(size_t rdLengthOffset, size_t rdataStart) {
    if (length < rdataStart || length - rdataStart > 0xffff) return false;
    put16(data + rdLengthOffset,
          static_cast<uint16_t>(length - rdataStart));
    ++answers;
    return true;
  }

  bool ptr(const char *owner, const char *target, uint32_t ttl) {
    size_t rd = 0;
    if (!rrHeader(owner, kTypePtr, kClassIn, ttl, rd)) return false;
    const size_t start = length;
    return name(target) && finishRdata(rd, start);
  }

  bool a(const char *owner, const IPAddress &ip, uint32_t ttl) {
    size_t rd = 0;
    if (!rrHeader(owner, kTypeA, kClassIn | kCacheFlush, ttl, rd))
      return false;
    const size_t start = length;
    const uint8_t bytes[4] = {ip[0], ip[1], ip[2], ip[3]};
    return raw(bytes, sizeof(bytes)) && finishRdata(rd, start);
  }

  bool srv(const char *owner, uint16_t port, const char *target,
           uint32_t ttl) {
    size_t rd = 0;
    if (!rrHeader(owner, kTypeSrv, kClassIn | kCacheFlush, ttl, rd))
      return false;
    const size_t start = length;
    return u16(0) && u16(0) && u16(port) && name(target) &&
           finishRdata(rd, start);
  }

  bool txtStart(const char *owner, uint32_t ttl, size_t &rd,
                size_t &start) {
    if (!rrHeader(owner, kTypeTxt, kClassIn | kCacheFlush, ttl, rd))
      return false;
    start = length;
    return true;
  }

  bool txtItem(const char *key, const char *value) {
    char item[192];
    const int written =
        snprintf(item, sizeof(item), "%s=%s", key, value ? value : "");
    if (written < 0 || written > 255 ||
        static_cast<size_t>(written) >= sizeof(item))
      return false;
    return u8(static_cast<uint8_t>(written)) &&
           raw(item, static_cast<size_t>(written));
  }

  bool txtFinish(size_t rd, size_t start) {
    return finishRdata(rd, start);
  }

  void finalize() { put16(data + 6, answers); }
};

static void serviceInstance(const ServiceSpec &service, char *out,
                            size_t outSize) {
  snprintf(out, outSize, "%s.%s", state.instance, service.type);
}

static bool addServiceTxt(Builder &builder, const ServiceSpec &service,
                          const char *instanceFqdn, uint32_t ttl) {
  size_t rd = 0;
  size_t start = 0;
  if (!builder.txtStart(instanceFqdn, ttl, rd, start)) return false;
  switch (service.kind) {
    case ServiceKind::Http:
      if (!builder.txtItem("model", state.model) ||
          !builder.txtItem("serial", state.serial))
        return false;
      break;
    case ServiceKind::IrTracker:
      if (!builder.txtItem("model", state.model) ||
          !builder.txtItem("serial", state.serial) ||
          !builder.txtItem("api", "/api/v1/meter"))
        return false;
      break;
    case ServiceKind::Modbus:
      if (!builder.txtItem("schema", "irtracker.meter.v1") ||
          !builder.txtItem("model", state.model))
        return false;
      break;
    case ServiceKind::Shelly:
      if (!builder.txtItem("id", state.shellyId) ||
          !builder.txtItem("model", state.shellyModel) ||
          !builder.txtItem("gen", "2"))
        return false;
      break;
    case ServiceKind::Everhome: {
      char ipText[16];
      snprintf(ipText, sizeof(ipText), "%u.%u.%u.%u", state.ip[0],
               state.ip[1], state.ip[2], state.ip[3]);
      if (!builder.txtItem("serial-number", state.serial) ||
          !builder.txtItem("product", state.model) ||
          !builder.txtItem("product-id", "IRT1000") ||
          !builder.txtItem("ip", ipText))
        return false;
      break;
    }
  }
  return builder.txtFinish(rd, start);
}

static bool addServiceRecords(Builder &builder, const ServiceSpec &service,
                              uint32_t ptrTtl, uint32_t uniqueTtl,
                              bool includePtr = true) {
  if (!serviceEnabled(service)) return true;
  char instanceFqdn[160];
  serviceInstance(service, instanceFqdn, sizeof(instanceFqdn));
  if (includePtr && !builder.ptr(service.type, instanceFqdn, ptrTtl))
    return false;
  if (!builder.srv(instanceFqdn, service.port, state.hostFqdn, uniqueTtl))
    return false;
  if (!addServiceTxt(builder, service, instanceFqdn, uniqueTtl))
    return false;
  return builder.a(state.hostFqdn, state.ip, uniqueTtl);
}

static bool sendBuffer(size_t length, const sockaddr_in *unicastTarget) {
  if (state.socket < 0 || !length) return false;
  sockaddr_in target = {};
  if (unicastTarget) {
    target = *unicastTarget;
  } else {
    target.sin_family = AF_INET;
    target.sin_port = htons(kPort);
    target.sin_addr.s_addr = inet_addr(kGroup);
  }
  return sendto(state.socket, txPacket, length, 0,
                reinterpret_cast<const sockaddr *>(&target),
                sizeof(target)) == static_cast<ssize_t>(length);
}

static void sendHostAnswer(uint32_t ttl,
                           const sockaddr_in *unicastTarget = nullptr,
                           uint16_t id = 0) {
  Builder builder(txPacket, sizeof(txPacket), unicastTarget ? id : 0);
  if (!builder.a(state.hostFqdn, state.ip, ttl)) return;
  builder.finalize();
  sendBuffer(builder.length, unicastTarget);
}

static void sendServiceAnswer(const ServiceSpec &service, uint32_t ptrTtl,
                              uint32_t uniqueTtl,
                              const sockaddr_in *unicastTarget = nullptr,
                              uint16_t id = 0) {
  if (!serviceEnabled(service)) return;
  Builder builder(txPacket, sizeof(txPacket), unicastTarget ? id : 0);
  if (!addServiceRecords(builder, service, ptrTtl, uniqueTtl)) return;
  builder.finalize();
  sendBuffer(builder.length, unicastTarget);
}

static void sendEnumerationAnswer(const sockaddr_in *unicastTarget = nullptr,
                                  uint16_t id = 0) {
  Builder builder(txPacket, sizeof(txPacket), unicastTarget ? id : 0);
  for (const auto &service : kServices) {
    if (!serviceEnabled(service)) continue;
    if (!builder.ptr(kServiceEnumeration, service.type, kPtrTtl)) return;
  }
  builder.finalize();
  sendBuffer(builder.length, unicastTarget);
}

static void announceAll(uint32_t ptrTtl, uint32_t uniqueTtl) {
  sendHostAnswer(uniqueTtl);
  for (const auto &service : kServices)
    sendServiceAnswer(service, ptrTtl, uniqueTtl);
}

static void sendProbe() {
  if (state.socket < 0) return;
  memset(txPacket, 0, sizeof(txPacket));
  put16(txPacket + 4, 1);
  size_t length = 12;
  const char *cursor = state.hostFqdn;
  while (*cursor) {
    const char *dot = strchr(cursor, '.');
    const size_t labelLength =
        dot ? static_cast<size_t>(dot - cursor) : strlen(cursor);
    if (!labelLength || labelLength > 63 ||
        length + labelLength + 1 >= sizeof(txPacket))
      return;
    txPacket[length++] = static_cast<uint8_t>(labelLength);
    memcpy(txPacket + length, cursor, labelLength);
    length += labelLength;
    if (!dot) break;
    cursor = dot + 1;
  }
  if (length + 5 > sizeof(txPacket)) return;
  txPacket[length++] = 0;
  put16(txPacket + length, kTypeAny);
  length += 2;
  put16(txPacket + length, kClassIn);
  length += 2;
  sendBuffer(length, nullptr);
}

static void rebuildHostFqdn() {
  snprintf(state.hostFqdn, sizeof(state.hostFqdn), "%s.local",
           state.hostname);
}

static void useConflictFallback() {
  if (!state.suffix[0]) return;
  char candidate[64];
  snprintf(candidate, sizeof(candidate), "%.50s-%s", state.baseHostname,
           state.suffix);
  if (nameEquals(candidate, state.hostname)) return;
  strlcpy(state.hostname, candidate, sizeof(state.hostname));
  rebuildHostFqdn();
  state.announced = false;
  state.probing = true;
  state.probesSent = 0;
  state.nextProbeMs = millis();
}

static bool detectAddressConflict(const uint8_t *data, size_t length) {
  if (length < 12 || !(read16(data + 2) & 0x8000)) return false;
  const uint16_t questions = read16(data + 4);
  const uint16_t answers = read16(data + 6);
  const uint16_t authority = read16(data + 8);
  const uint16_t additional = read16(data + 10);
  size_t offset = 12;
  char name[192];
  for (uint16_t i = 0; i < questions; ++i) {
    if (!readName(data, length, offset, name, sizeof(name)) ||
        offset + 4 > length)
      return false;
    offset += 4;
  }
  const uint32_t records =
      static_cast<uint32_t>(answers) + authority + additional;
  for (uint32_t i = 0; i < records; ++i) {
    if (!readName(data, length, offset, name, sizeof(name)) ||
        offset + 10 > length)
      return false;
    const uint16_t type = read16(data + offset);
    const uint16_t rdLength = read16(data + offset + 8);
    offset += 10;
    if (offset + rdLength > length) return false;
    if (type == kTypeA && rdLength == 4 &&
        nameEquals(name, state.hostFqdn) &&
        !sameIp(data + offset, state.ip))
      return true;
    offset += rdLength;
  }
  return false;
}

static const ServiceSpec *findServiceByType(const char *name) {
  for (const auto &service : kServices)
    if (serviceEnabled(service) && nameEquals(name, service.type))
      return &service;
  return nullptr;
}

static const ServiceSpec *findServiceByInstance(const char *name) {
  char fqdn[160];
  for (const auto &service : kServices) {
    if (!serviceEnabled(service)) continue;
    serviceInstance(service, fqdn, sizeof(fqdn));
    if (nameEquals(name, fqdn)) return &service;
  }
  return nullptr;
}

static void answerQuestion(const char *name, uint16_t type, bool unicast,
                           const sockaddr_in &source, uint16_t queryId) {
  const sockaddr_in *target = unicast ? &source : nullptr;
  const uint16_t responseId = unicast ? queryId : 0;
  if (nameEquals(name, state.hostFqdn) &&
      (type == kTypeA || type == kTypeAny)) {
    sendHostAnswer(kUniqueTtl, target, responseId);
    return;
  }
  if (nameEquals(name, kServiceEnumeration) &&
      (type == kTypePtr || type == kTypeAny)) {
    sendEnumerationAnswer(target, responseId);
    return;
  }
  if (const ServiceSpec *service = findServiceByType(name)) {
    if (type == kTypePtr || type == kTypeAny)
      sendServiceAnswer(*service, kPtrTtl, kUniqueTtl, target, responseId);
    return;
  }
  if (const ServiceSpec *service = findServiceByInstance(name)) {
    if (type == kTypeSrv || type == kTypeTxt || type == kTypeAny)
      sendServiceAnswer(*service, kPtrTtl, kUniqueTtl, target, responseId);
  }
}

static void handleQuery(const uint8_t *data, size_t length,
                        const sockaddr_in &source) {
  if (length < 12) return;
  const uint16_t flags = read16(data + 2);
  if (flags & 0x8000) return;
  const uint16_t queryId = read16(data);
  const uint16_t questions = read16(data + 4);
  size_t offset = 12;
  char name[192];
  for (uint16_t i = 0; i < questions; ++i) {
    if (!readName(data, length, offset, name, sizeof(name)) ||
        offset + 4 > length)
      return;
    const uint16_t type = read16(data + offset);
    const uint16_t rrClass = read16(data + offset + 2);
    offset += 4;
    if ((rrClass & 0x7fff) != kClassIn) continue;
    const bool qu = (rrClass & 0x8000) != 0;
    const bool legacyUnicast = ntohs(source.sin_port) != kPort;
    answerQuestion(name, type, qu || legacyUnicast, source, queryId);
  }
}

static void receivePackets() {
  if (state.socket < 0) return;
  for (uint8_t budget = 0; budget < 8; ++budget) {
    sockaddr_in source = {};
    socklen_t sourceLength = sizeof(source);
    const ssize_t received =
        recvfrom(state.socket, rxPacket, sizeof(rxPacket), 0,
                 reinterpret_cast<sockaddr *>(&source), &sourceLength);
    if (received < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) return;
      return;
    }
    if (received < 12) continue;
    if (detectAddressConflict(rxPacket, static_cast<size_t>(received))) {
      useConflictFallback();
      continue;
    }
    handleQuery(rxPacket, static_cast<size_t>(received), source);
  }
}

static bool openSocket(const IPAddress &ip) {
  const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) return false;
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
    close(sock);
    return false;
  }
  sockaddr_in bindAddress = {};
  bindAddress.sin_family = AF_INET;
  bindAddress.sin_port = htons(kPort);
  bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, reinterpret_cast<const sockaddr *>(&bindAddress),
           sizeof(bindAddress)) != 0) {
    close(sock);
    return false;
  }
  ip_mreq membership = {};
  membership.imr_multiaddr.s_addr = inet_addr(kGroup);
  membership.imr_interface.s_addr = ipToNetwork(ip);
  if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership,
                 sizeof(membership)) != 0) {
    close(sock);
    return false;
  }
  in_addr outbound = {};
  outbound.s_addr = ipToNetwork(ip);
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &outbound, sizeof(outbound));
  uint8_t ttl = 255;
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
  uint8_t loopback = 0;
  setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback,
             sizeof(loopback));
  const int flags = fcntl(sock, F_GETFL, 0);
  if (flags >= 0) fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  state.socket = sock;
  return true;
}

bool begin(const char *hostname, const char *instance, const char *serial,
           const char *model, const char *shellyModel, const char *shellyId,
           const char *suffix, const IPAddress &ip, bool modbusEnabled) {
  if (!hostname || !hostname[0] || !instance || !instance[0] ||
      ip == IPAddress(0, 0, 0, 0))
    return false;
  if (state.active) {
    if (state.announced) announceAll(0, 0);
    if (state.socket >= 0) close(state.socket);
    state = State{};
  }
  state.ip = ip;
  state.modbus = modbusEnabled;
  strlcpy(state.baseHostname, hostname, sizeof(state.baseHostname));
  strlcpy(state.hostname, hostname, sizeof(state.hostname));
  strlcpy(state.instance, instance, sizeof(state.instance));
  strlcpy(state.serial, serial, sizeof(state.serial));
  strlcpy(state.model, model, sizeof(state.model));
  strlcpy(state.shellyModel, shellyModel, sizeof(state.shellyModel));
  strlcpy(state.shellyId, shellyId, sizeof(state.shellyId));
  strlcpy(state.suffix, suffix, sizeof(state.suffix));
  rebuildHostFqdn();
  if (!openSocket(ip)) {
    state = State{};
    return false;
  }
  state.active = true;
  state.probing = true;
  state.nextProbeMs = millis();
  return true;
}

void end() {
  if (!state.active) return;
  if (state.announced) {
    announceAll(0, 0);
    delay(5);
  }
  if (state.socket >= 0) close(state.socket);
  state = State{};
}

void loop() {
  if (!state.active) return;
  receivePackets();
  const uint32_t now = millis();
  if (state.probing && static_cast<int32_t>(now - state.nextProbeMs) >= 0) {
    if (state.probesSent < kProbeCount) {
      sendProbe();
      ++state.probesSent;
      state.nextProbeMs = now + kProbeStepMs;
      return;
    }
    state.probing = false;
    state.announced = true;
    announceAll(kPtrTtl, kUniqueTtl);
    state.nextRefreshMs = now + 1000;
    return;
  }
  if (state.announced &&
      static_cast<int32_t>(now - state.nextRefreshMs) >= 0) {
    announceAll(kPtrTtl, kUniqueTtl);
    state.nextRefreshMs = now + kRefreshMs;
  }
}

const char *effectiveHostname() { return state.hostname; }

}  // namespace MinimalMdns
