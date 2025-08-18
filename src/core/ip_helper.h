#pragma once

#include "core/constants.h"

namespace sim {
class IPHelper {
public:
  static std::string
  BuildBindAddress(const std::string &ip = constants::ANY_IP,
                   const std::string &port = constants::SCHEDULER_PORT) {
    return "tcp://" + ip + ":" + port;
  };

  static std::string
  BuildConnectAddress(const std::string &ip = constants::LOCALHOST_IP,
                      const std::string &port = constants::SCHEDULER_PORT) {
    return "tcp://" + ip + ":" + port;
  };
};
} // namespace sim