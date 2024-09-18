#include "obd2_bridge.h"

#include <iostream>

namespace obd2_server {
    void obd2_bridge::setup_can_device() {
        // Make sure the can device string is safe to use as a shell parameter
        for (char c : can_device) {
            if (!isalnum(c)) {
                throw std::invalid_argument("Could not setup CAN device: Invalid device name");
            }
        }

        std::string command = "sudo ip link set " + can_device + " type can bitrate " + std::to_string(can_bitrate);

        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Could not set up CAN device: Command failed, please retry as root");
        }
    }
}
