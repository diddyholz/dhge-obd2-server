#include "obd2_bridge.h"

#include <iostream>
#include <fstream>

namespace obd2_server {
    int32_t execute_command(std::string command, std::string &output);

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
            + std::to_string(can_bitrate) + " loopback off";
        std::string output;

        if (execute_command(command, output) != 0) {
            throw std::runtime_error("Could not set up CAN device: " + output);
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
        std::string output;

        if (execute_command(command, output) != 0) {
            throw std::runtime_error("Could not shut down CAN device: " + output);
        }
    }

    int32_t execute_command(std::string command, std::string &output) {
        uint32_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::string tmp_file = std::filesystem::temp_directory_path().string() 
            + "/obd2_tmp_" + std::to_string(timestamp);

        command += " > " + tmp_file + " 2>&1";

        int32_t ret = system(command.c_str());

        std::ifstream file(tmp_file);
        std::stringstream buffer;
        buffer << file.rdbuf();

        output = buffer.str();

        return ret;
    }
}
