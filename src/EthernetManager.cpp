#include "EthernetManager.h"

#include "HardwareProfile.h"

#if IR_TRACKER_LAN_PROFILE
extern "C" {
#include "driver/spi_master.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_netif_glue.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
}

namespace {

constexpr spi_host_device_t kSpiHost = SPI2_HOST;
constexpr int kSpiClockHz = 20 * 1000 * 1000;
constexpr int kEthernetRoutePriority = 150;  // Wi-Fi STA default is 100.

spi_device_handle_t spiHandle = nullptr;
esp_eth_handle_t ethHandle = nullptr;
esp_netif_t *ethNetif = nullptr;
esp_eth_netif_glue_handle_t ethGlue = nullptr;
esp_event_handler_instance_t ethEventInstance = nullptr;
esp_event_handler_instance_t ipEventInstance = nullptr;
esp_event_handler_instance_t ipLostEventInstance = nullptr;

void ethernetEvent(void *arg, esp_event_base_t, int32_t eventId, void *) {
  static_cast<EthernetManager *>(arg)->onEthernetEvent(eventId);
}

void ethernetIpEvent(void *arg, esp_event_base_t, int32_t eventId,
                     void *eventData) {
  auto *manager = static_cast<EthernetManager *>(arg);
  if (eventId == IP_EVENT_ETH_GOT_IP && eventData) {
    const auto *event = static_cast<ip_event_got_ip_t *>(eventData);
    manager->onGotIp(event->ip_info.ip.addr);
  } else if (eventId == IP_EVENT_ETH_LOST_IP) {
    manager->onLostIp();
  }
}

bool acceptedInitResult(esp_err_t result) {
  return result == ESP_OK || result == ESP_ERR_INVALID_STATE;
}

bool probeW5500() {
  // W5500 VERSIONR is read-only and must contain 0x04. Testing this register
  // before installing the driver cleanly distinguishes an absent daughterboard
  // from an unplugged Ethernet cable.
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    spi_transaction_t transaction = {};
    transaction.cmd = 0x0039;   // VERSIONR in the common register block.
    transaction.addr = 0x00;    // Common block, read, variable length mode.
    transaction.length = 8;
    transaction.rxlength = 8;
    transaction.flags = SPI_TRANS_USE_RXDATA;
    if (spi_device_polling_transmit(spiHandle, &transaction) == ESP_OK &&
        transaction.rx_data[0] == 0x04) {
      return true;
    }
    delay(2);
  }
  return false;
}

}  // namespace

bool EthernetManager::begin(const char *hostname) {
  if (initialized_) return true;
  lastError_ = "";
  // Arduino normally creates these while Wi-Fi starts. Creating them here is
  // safe and allows Ethernet to initialize first.
  if (!acceptedInitResult(esp_netif_init())) {
    lastError_ = "esp_netif_init_failed";
    return false;
  }
  if (!acceptedInitResult(esp_event_loop_create_default())) {
    lastError_ = "event_loop_init_failed";
    return false;
  }

  pinMode(HardwareProfile::kW5500CsPin, OUTPUT);
  digitalWrite(HardwareProfile::kW5500CsPin, HIGH);
  pinMode(HardwareProfile::kW5500IntPin, INPUT_PULLUP);

  spi_bus_config_t busConfig = {};
  busConfig.mosi_io_num = HardwareProfile::kW5500MosiPin;
  busConfig.miso_io_num = HardwareProfile::kW5500MisoPin;
  busConfig.sclk_io_num = HardwareProfile::kW5500SckPin;
  busConfig.quadwp_io_num = -1;
  busConfig.quadhd_io_num = -1;
  busConfig.max_transfer_sz = 1600;
  esp_err_t result = spi_bus_initialize(kSpiHost, &busConfig, SPI_DMA_CH_AUTO);
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    lastError_ = "spi_bus_init_failed:" + String(esp_err_to_name(result));
    return false;
  }

  spi_device_interface_config_t deviceConfig = {};
  deviceConfig.command_bits = 16;
  deviceConfig.address_bits = 8;
  deviceConfig.mode = 0;
  deviceConfig.clock_speed_hz = kSpiClockHz;
  deviceConfig.spics_io_num = HardwareProfile::kW5500CsPin;
  deviceConfig.queue_size = 20;
  result = spi_bus_add_device(kSpiHost, &deviceConfig, &spiHandle);
  if (result != ESP_OK) {
    lastError_ = "spi_device_failed:" + String(esp_err_to_name(result));
    return false;
  }
  if (!probeW5500()) {
    spi_bus_remove_device(spiHandle);
    spiHandle = nullptr;
    spi_bus_free(kSpiHost);
    lastError_ = "w5500_not_present";
    return false;
  }
  hardwareDetected_ = true;

  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phyConfig = ETH_PHY_DEFAULT_CONFIG();
  phyConfig.phy_addr = 1;
  phyConfig.reset_gpio_num = -1;  // RESET_N is the shared board reset.
  eth_w5500_config_t w5500Config = ETH_W5500_DEFAULT_CONFIG(spiHandle);
  w5500Config.int_gpio_num = HardwareProfile::kW5500IntPin;

  esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500Config, &macConfig);
  esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phyConfig);
  if (!mac || !phy) {
    if (mac) mac->del(mac);
    if (phy) phy->del(phy);
    lastError_ = "w5500_driver_allocation_failed";
    return false;
  }

  esp_eth_config_t ethConfig = ETH_DEFAULT_CONFIG(mac, phy);
  result = esp_eth_driver_install(&ethConfig, &ethHandle);
  if (result != ESP_OK) {
    mac->del(mac);
    phy->del(phy);
    lastError_ = "ethernet_driver_install_failed:" +
                 String(esp_err_to_name(result));
    return false;
  }

  uint8_t universalMac[6] = {};
  uint8_t ethernetMac[6] = {};
  if (esp_read_mac(universalMac, ESP_MAC_WIFI_STA) == ESP_OK &&
      esp_derive_local_mac(ethernetMac, universalMac) == ESP_OK) {
    esp_eth_ioctl(ethHandle, ETH_CMD_S_MAC_ADDR, ethernetMac);
  }

  // A higher route priority makes LAN the default route while its link and
  // DHCP address are available. lwIP automatically falls back to Wi-Fi.
  esp_netif_inherent_config_t inherent = ESP_NETIF_INHERENT_DEFAULT_ETH();
  inherent.if_key = "IRTRACKER_ETH";
  inherent.if_desc = "w5500";
  inherent.route_prio = kEthernetRoutePriority;
  esp_netif_config_t netifConfig = {};
  netifConfig.base = &inherent;
  netifConfig.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH;
  ethNetif = esp_netif_new(&netifConfig);
  if (!ethNetif) {
    lastError_ = "ethernet_netif_create_failed";
    return false;
  }
  if (hostname && *hostname) esp_netif_set_hostname(ethNetif, hostname);

  ethGlue = esp_eth_new_netif_glue(ethHandle);
  if (!ethGlue || esp_netif_attach(ethNetif, ethGlue) != ESP_OK) {
    lastError_ = "ethernet_netif_attach_failed";
    return false;
  }
  if (esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                          &ethernetEvent, this,
                                          &ethEventInstance) != ESP_OK ||
      esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                          &ethernetIpEvent, this,
                                          &ipEventInstance) != ESP_OK ||
      esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_LOST_IP,
                                          &ethernetIpEvent, this,
                                          &ipLostEventInstance) != ESP_OK) {
    lastError_ = "ethernet_event_registration_failed";
    return false;
  }

  result = esp_eth_start(ethHandle);
  if (result != ESP_OK) {
    lastError_ = "w5500_not_detected:" + String(esp_err_to_name(result));
    return false;
  }
  initialized_ = true;
  return true;
}

void EthernetManager::loop() {
  if (!initialized_ || millis() - lastMaintenanceMs_ < 1000) return;
  lastMaintenanceMs_ = millis();
  // Events carry normal state changes. This periodic check also recovers the
  // IP state if an event was missed during early boot.
  if (ethNetif && esp_netif_is_netif_up(ethNetif)) {
    esp_netif_ip_info_t info = {};
    if (esp_netif_get_ip_info(ethNetif, &info) == ESP_OK && info.ip.addr) {
      gotIp_ = true;
      ipAddress_ = info.ip.addr;
    }
  }
}

IPAddress EthernetManager::localIP() const { return IPAddress(ipAddress_); }

void EthernetManager::onEthernetEvent(int32_t eventId) {
  switch (eventId) {
    case ETHERNET_EVENT_START:
      break;
    case ETHERNET_EVENT_CONNECTED:
      hardwareDetected_ = true;
      linkUp_ = true;
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      linkUp_ = false;
      gotIp_ = false;
      ipAddress_ = 0;
      break;
    case ETHERNET_EVENT_STOP:
      linkUp_ = false;
      gotIp_ = false;
      ipAddress_ = 0;
      break;
    default:
      break;
  }
}

void EthernetManager::onGotIp(uint32_t address) {
  hardwareDetected_ = true;
  linkUp_ = true;
  gotIp_ = address != 0;
  ipAddress_ = address;
}

void EthernetManager::onLostIp() {
  gotIp_ = false;
  ipAddress_ = 0;
}

#else

bool EthernetManager::begin(const char *) {
  lastError_ = "ethernet_disabled_in_build";
  return false;
}
void EthernetManager::loop() {}
IPAddress EthernetManager::localIP() const { return IPAddress(); }
void EthernetManager::onEthernetEvent(int32_t) {}
void EthernetManager::onGotIp(uint32_t) {}
void EthernetManager::onLostIp() {}

#endif
