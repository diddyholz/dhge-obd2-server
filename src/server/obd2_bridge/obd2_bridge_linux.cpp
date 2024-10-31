#include "obd2_bridge.h"

#include <iostream>

namespace obd2_server {
    void obd2_bridge::setup_can_device() {
        if (skip_can_setup) {
            return;
        }

    // Before setting up can device, shut it down first, to change attributes
    shutdown_can_device();

        // Make sure the can device string is safe to use as a shell parameter
        for (char c : can_device) {
            if (!isalnum(c)) {
                throw std::invalid_argument("Could not setup CAN device: Invalid device name");
            }
        }

        std::string command = "ip link set " + can_device + " up type can bitrate " 
            + std::to_string(can_bitrate) + " loopback off > /dev/null 2> /dev/null";

        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Could not set up CAN device: Command failed, please retry as root");
        }
    }

    void obd2_bridge::shutdown_can_device() {
        if (skip_can_setup) {
        return;
    }

    // Make sure the can device string is safe to use as a shell parameter
    for (char c : can_device) {
        if (!isalnum(c)) {
                throw std::invalid_argument("Could not shutdown CAN device: Invalid device name");
            }
    }

    std::string command = "ip link set " + can_device + " down";

    if (system(command.c_str()) != 0) {
        throw std::runtime_error("Could not shut down CAN device: Command failed, please retry as root");
    }
    }
}
