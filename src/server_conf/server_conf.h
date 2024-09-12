#pragma once

#include <cstdint>
#include <string>
#include <json.hpp>

namespace obd2_server {
    class server {
        public:
            static const std::string DEFAULT_OBD2_CAN_DEVICE;
            static const uint32_t DEFAULT_OBD2_CAN_BITRATE;
            static const uint32_t DEFAULT_OBD2_REFRESH_MS;
            static const bool DEFAULT_OBD2_USE_PID_CHAINING;

            static const std::string DEFAULT_SERVER_ADDRESS;
            static const uint16_t DEFAULT_SERVER_PORT;

            server();
            server(std::string server_config);

        private:
            std::string obd2_can_device = DEFAULT_OBD2_CAN_DEVICE;
            uint32_t obd2_can_bitrate = DEFAULT_OBD2_CAN_BITRATE;
            uint32_t obd2_refresh_ms = DEFAULT_OBD2_REFRESH_MS;
            bool obd2_use_pid_chaining = DEFAULT_OBD2_USE_PID_CHAINING;

            std::string server_address = DEFAULT_SERVER_ADDRESS;
            uint16_t server_port = DEFAULT_SERVER_PORT;

            friend void to_json(nlohmann::json& j, const server& s);
            friend void from_json(const nlohmann::json& j, server& s);
    };

    void to_json(nlohmann::json& j, const server& s);
    void from_json(const nlohmann::json& j, server& s);
}