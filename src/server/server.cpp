#include "server.h"

#include <fstream>

namespace obd2_server {
    const std::string server::DEFAULT_OBD2_CAN_DEVICE = "can0";
    const uint32_t server::DEFAULT_OBD2_CAN_BITRATE = 500000;
    const uint32_t server::DEFAULT_OBD2_REFRESH_MS = 1000;
    const bool server::DEFAULT_OBD2_USE_PID_CHAINING = false;

    const std::string server::DEFAULT_SERVER_ADDRESS = "0.0.0.0";
    const uint16_t server::DEFAULT_SERVER_PORT = 80;

    server::server() { }

    server::server(std::string server_config) {
        std::ifstream file(server_config);

        if (!file.is_open()) {
            std::string error = strerror(errno);
            throw std::invalid_argument( + ": " + server_config);
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(file);
        }
        catch (std::exception &e) {
            throw std::invalid_argument(e.what());
        }

        from_json(j, *this);
    }

    void to_json(nlohmann::json& j, const server& s) {
        j = nlohmann::json{
            {"obd2_can_device", s.obd2_can_device},
            {"obd2_can_bitrate", s.obd2_can_bitrate},
            {"obd2_refresh_ms", s.obd2_refresh_ms},
            {"obd2_use_pid_chaining", s.obd2_use_pid_chaining},
            {"server_address", s.server_address},
            {"server_port", s.server_port}
        };
    }

    void from_json(const nlohmann::json& j, server& s) {
        s.obd2_can_device = j.at("obd2_can_device").template get<std::string>();
        s.obd2_can_bitrate = j.at("obd2_can_bitrate").template get<uint32_t>();
        s.obd2_refresh_ms = j.at("obd2_refresh_ms").template get<uint32_t>();
        s.obd2_use_pid_chaining = j.at("obd2_use_pid_chaining").template get<bool>();
        s.server_address = j.at("server_address").template get<std::string>();
        s.server_port = j.at("server_port").template get<uint16_t>();
    }
}
