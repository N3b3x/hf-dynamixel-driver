#include "dynamixel_types.hpp"

#ifndef WINDECLSPEC
#define WINDECLSPEC
#endif

#include "packet_handler.h"

namespace dynamixel {

DriverError MapCommCode(int comm_code) noexcept {
    switch (comm_code) {
        case COMM_SUCCESS:       return DriverError::None;
        case COMM_PORT_BUSY:     return DriverError::Busy;
        case COMM_TX_FAIL:       return DriverError::SerialError;
        case COMM_RX_FAIL:       return DriverError::SerialError;
        case COMM_TX_ERROR:      return DriverError::InvalidParameter;
        case COMM_RX_TIMEOUT:    return DriverError::Timeout;
        case COMM_RX_CORRUPT:    return DriverError::ChecksumError;
        case COMM_NOT_AVAILABLE: return DriverError::UnsupportedRequest;
        default:                 return DriverError::MalformedResponse;
    }
}

}  // namespace dynamixel
