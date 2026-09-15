#pragma once

// Keep the existing synchronous clients and protocol libraries. Their receive
// wait loops repeatedly call available(); service the meter on that same task.
// This does not make DNS, TCP connect or TLS handshakes asynchronous.
template <typename Base, void (*Service)()>
class ServicedNetworkClient : public Base {
 public:
  using Base::read;
  int available() override {
    Service();
    return Base::available();
  }
  int read() override {
    Service();
    return Base::read();
  }
  int read(uint8_t *buffer, size_t length) override {
    Service();
    return Base::read(buffer, length);
  }
};
