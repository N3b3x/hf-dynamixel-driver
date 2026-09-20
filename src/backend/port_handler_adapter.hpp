/**
 * @file port_handler_adapter.hpp
 * @brief Private SDK PortHandler that drives an HF TransportView.
 *
 * Not a public header. Do not include from inc/.
 */
#pragma once

#ifndef WINDECLSPEC
#define WINDECLSPEC
#endif

#include "dynamixel_transport.hpp"

#include "port_handler.h"

#include <algorithm>
#include <cstring>

namespace dynamixel {
namespace backend {

class PortHandlerAdapter : public ::dynamixel::PortHandler {
public:
    explicit PortHandlerAdapter(TransportView transport) noexcept
        : transport_(transport) {
        is_using_ = false;
        std::strncpy(port_name_, "hf-transport", sizeof(port_name_) - 1);
        port_name_[sizeof(port_name_) - 1] = '\0';
    }

    void set_config(const TransportConfig& cfg) noexcept {
        config_ = cfg;
        baud_rate_ = static_cast<int>(cfg.baud_rate);
        UpdateByteTime();
    }

    const TransportConfig& config() const noexcept { return config_; }
    TransportView transport() const noexcept { return transport_; }
    bool last_echo_ok() const noexcept { return last_echo_ok_; }

    bool openPort() override {
        const DriverError err = transport_.open(config_);
        opened_ = (err == DriverError::None);
        return opened_;
    }

    void closePort() override {
        transport_.close();
        opened_ = false;
    }

    void clearPort() override { transport_.discard_stale_input(); }

    void setPortName(const char* port_name) override {
        if (port_name == nullptr) {
            return;
        }
        std::strncpy(port_name_, port_name, sizeof(port_name_) - 1);
        port_name_[sizeof(port_name_) - 1] = '\0';
    }

    char* getPortName() override { return port_name_; }

    bool setBaudRate(const int baudrate) override {
        if (is_using_) {
            return false;
        }
        if (baudrate <= 0) {
            return false;
        }
        const DriverError err = transport_.set_baud_rate(static_cast<uint32_t>(baudrate));
        if (err != DriverError::None) {
            return false;
        }
        baud_rate_ = baudrate;
        config_.baud_rate = static_cast<uint32_t>(baudrate);
        UpdateByteTime();
        return true;
    }

    int getBaudRate() override { return baud_rate_; }

    int getBytesAvailable() override {
        // Prompt, non-blocking peek via a zero-copy read of 0 is not available.
        // Report "maybe data" as 1 so rxPacket calls readPort.
        return 1;
    }

    int readPort(uint8_t* packet, int length) override {
        if (packet == nullptr || length <= 0) {
            return 0;
        }
        const TransportIo io = transport_.read_some(packet, static_cast<std::size_t>(length));
        if (io.error == DriverError::SerialError) {
            return -1;
        }
        return static_cast<int>(io.accepted);
    }

    int writePort(uint8_t* packet, int length) override {
        if (packet == nullptr || length <= 0) {
            return 0;
        }
        last_echo_ok_ = true;
        std::size_t written = 0;
        while (written < static_cast<std::size_t>(length)) {
            const TransportIo io = transport_.write_some(
                packet + written, static_cast<std::size_t>(length) - written);
            if (io.error != DriverError::None) {
                return -1;
            }
            if (io.accepted == 0) {
                break;
            }
            written += io.accepted;
        }
        if (written != static_cast<std::size_t>(length)) {
            return static_cast<int>(written);
        }
        if (transport_.finish_transmit() != DriverError::None) {
            return -1;
        }
        if (config_.echo_policy == EchoPolicy::ExpectedLocalEcho) {
            if (!ConsumeEcho(packet, static_cast<std::size_t>(length))) {
                last_echo_ok_ = false;
                return -1;
            }
        }
        return static_cast<int>(written);
    }

    void setPacketTimeout(uint16_t packet_length) override {
        const double msec =
            (tx_time_per_byte_ms_ * static_cast<double>(packet_length)) +
            (static_cast<double>(config_.latency_ms) * 2.0) + 2.0;
        setPacketTimeout(msec);
    }

    void setPacketTimeout(double msec) override {
        start_us_ = transport_.now_us();
        timeout_us_ = static_cast<uint64_t>(msec * 1000.0);
        if (timeout_us_ == 0) {
            timeout_us_ = 1;
        }
        timeout_armed_ = true;
    }

    bool isPacketTimeout() override {
        if (!timeout_armed_) {
            return true;
        }
        const uint64_t now = transport_.now_us();
        const uint64_t elapsed = (now >= start_us_) ? (now - start_us_) : 0;
        if (elapsed >= timeout_us_) {
            timeout_armed_ = false;
            return true;
        }
        transport_.yield();
        return false;
    }

private:
    void UpdateByteTime() noexcept {
        const double baud = (baud_rate_ > 0) ? static_cast<double>(baud_rate_) : 57600.0;
        tx_time_per_byte_ms_ = (1000.0 / baud) * 10.0;
    }

    bool ConsumeEcho(const uint8_t* expected, std::size_t length) noexcept {
        uint8_t scratch[64];
        std::size_t matched = 0;
        const uint64_t deadline = transport_.now_us() + 20 * 1000;
        while (matched < length) {
            const std::size_t chunk = std::min(length - matched, sizeof(scratch));
            const TransportIo io = transport_.read_some(scratch, chunk);
            if (io.error == DriverError::SerialError) {
                return false;
            }
            if (io.accepted == 0) {
                if (transport_.now_us() >= deadline) {
                    return false;
                }
                transport_.yield();
                continue;
            }
            for (std::size_t i = 0; i < io.accepted; ++i) {
                if (scratch[i] != expected[matched + i]) {
                    return false;
                }
            }
            matched += io.accepted;
        }
        return true;
    }

    TransportView transport_{};
    TransportConfig config_{};
    char port_name_[32]{};
    int baud_rate_{57600};
    double tx_time_per_byte_ms_{0.1736};
    uint64_t start_us_{0};
    uint64_t timeout_us_{0};
    bool timeout_armed_{false};
    bool opened_{false};
    bool last_echo_ok_{true};
};

}  // namespace backend
}  // namespace dynamixel
