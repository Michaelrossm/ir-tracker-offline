#pragma once

#include <Arduino.h>
#include <esp_partition.h>
#include <algorithm>

// Read failures are never evidence that a partition is safe to format.
inline bool partitionIsBlank(const esp_partition_t *partition) {
  if (!partition) return false;
  uint8_t buffer[256];
  for (size_t offset = 0; offset < partition->size; offset += sizeof(buffer)) {
    const size_t length = std::min<size_t>(sizeof(buffer), partition->size - offset);
    if (esp_partition_read(partition, offset, buffer, length) != ESP_OK) return false;
    for (size_t i = 0; i < length; ++i)
      if (buffer[i] != 0xff) return false;
    if ((offset & 0x3fffU) == 0) delay(0);
  }
  return true;
}
