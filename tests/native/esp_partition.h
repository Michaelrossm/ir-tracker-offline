#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
struct esp_partition_t { size_t size; };
constexpr int ESP_OK = 0;
constexpr int ESP_PARTITION_TYPE_DATA = 1;
constexpr int ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 130;
inline esp_partition_t testPartition{65536};
inline std::vector<uint8_t> testPartitionBytes(65536, 0xff);
inline bool testPartitionMissing = false, testPartitionReadError = false;
inline const esp_partition_t *esp_partition_find_first(int, int, const char *) {
  return testPartitionMissing ? nullptr : &testPartition;
}
inline int esp_partition_read(const esp_partition_t *, size_t offset, void *data, size_t length) {
  if (testPartitionReadError) return -1;
  memcpy(data, testPartitionBytes.data() + offset, length);
  return ESP_OK;
}
