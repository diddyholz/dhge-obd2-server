#include "server.h"

#include <fstream>

namespace obd2_server {
    const std::string server::DEFAULT_OBD2_CAN_DEVICE   = "can0";
    const uint32_t server::DEFAULT_OBD2_CAN_BITRATE     = 500000;
    const uint32_t server::DEFAULT_OBD2_REFRESH_MS      = 1000;
    const bool server::DEFAULT_OBD2_USE_PID_CHAINING    = false;
    const bool server::DEFAULT_OBD2_SKIP_CAN_SETUP      = false;

    const std::string server::DEFAULT_SERVER_ADDRESS    = "0.0.0.0";
    const uint16_t server::DEFAULT_SERVER_PORT          = 38380;

    const std::string server::DEFAULT_CONFIG_PATH       = "$HOME/.config/obd2-server/config.json";
    const std::string server::DEFAULT_DASHBOARDS_DIR    = "$HOME/.config/obd2-server/dashboards";
    const std::string server::DEFAULT_VEHICLES_DIR      = "$HOME/.config/obd2-server/vehicles";
    const std::string server::DEFAULT_LOGS_DIR          = "$HOME/.config/obd2-server/logs";

    bool server::load_server_config() {
        std::ifstream file(expand_path(config_path));

        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;

        try {
            j = nlohmann::json::parse(file);
        }
        catch (std::exception &e) {
            return false;
        }

        from_json(j, *this);

        return true;
    }

    uint32_t server::load_vehicles() {
        std::filesystem::path path(expand_path(vehicles_dir));

        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            return 0;
        }

        vehicles.clear();

        // Iterate through all files in the vehicles directory and load .json files
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (entry.path().extension() != ".json") {
                continue;
            }

            try {
                vehicle v(entry.path().string());
                vehicles.try_emplace(v.get_id(), v);
            }
            catch (std::exception &e) {
                std::cerr << "Could not load vehicle from " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }

        create_req_vehicle_map();

        return vehicles.size();
    }

    uint32_t server::load_dashboards() {
        std::filesystem::path path(expand_path(dashboards_dir));

        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            return 0;
        }

        dashboards.clear();

        // Iterate through all files in the dashboard directory and load .json files
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (entry.path().extension() != ".json") {
                continue;
            }

            try {
                dashboard d(entry.path().string());
                dashboards.try_emplace(d.get_id(), std::move(d));
            }
            catch (std::exception &e) {
                std::cerr << "Could not load dashboard from " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }

        return dashboards.size();
    }

    uint32_t server::load_logs() {
        std::filesystem::path path(expand_path(logs_dir));

        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            return 0;
        }

        logs.clear();

        // Iterate through all files in the logs directory and add loggers
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (entry.path().extension() != ".csv") {
                continue;
            }
            
            std::string name = entry.path().filename().replace_extension("").string();
            logs.try_emplace(name, name, path.string());
        }

        return logs.size();
    }

    void server::create_req_vehicle_map() {
        request_vehicle_map.clear();

        for (auto &v : vehicles) {
            const auto &requests = v.second.get_requests();

            for (const auto &r : requests) {
                request_vehicle_map[r.id] = v.second.get_id();
            }
        }
    }

    void server::save_server_config() {
        nlohmann::json j;

        to_json(j, *this);
        make_directories();

        std::ofstream file(expand_path(config_path));
        file << j.dump(4);
    }

    void server::make_directories() {
        // Strip filename from config path
        size_t last_slash = config_path.find_last_of("/\\");
        std::string config_dir = config_path.substr(0, last_slash);

        std::filesystem::create_directories(expand_path(config_dir));
        std::filesystem::create_directories(expand_path(dashboards_dir));
        std::filesystem::create_directories(expand_path(vehicles_dir));
        std::filesystem::create_directories(expand_path(logs_dir));
    }

    std::string server::expand_path(const std::string &path) {
        std::string expanded = path;

        size_t opening_pos = 0;
        size_t closing_pos = 0;

        // Expand environment variables
        while ((opening_pos = expanded.find('$')) != std::string::npos) {
            closing_pos = expanded.find('/', opening_pos + 1);

            if (closing_pos == std::string::npos) {
                break;
            }

            std::string env = expanded.substr(opening_pos + 1, closing_pos - opening_pos - 1);
            std::string value = std::getenv(env.c_str());

            if (!value.empty()) {
                expanded.replace(opening_pos, closing_pos - opening_pos, value);
            }
        }

        return expanded;
    }

    void to_json(nlohmann::json& j, const server& s) {
        j = nlohmann::json{
            {"obd2_can_device", s.obd2_can_device},
            {"obd2_can_bitrate", s.obd2_can_bitrate},
            {"obd2_refresh_ms", s.obd2_refresh_ms},
            {"obd2_use_pid_chaining", s.obd2_use_pid_chaining},
            {"obd2_skip_can_setup", s.obd2_skip_can_setup},
            {"server_address", s.server_address},
            {"server_port", s.server_port},
            {"config_path", s.config_path},
            {"dashboards_dir", s.dashboards_dir},
            {"vehicles_dir", s.vehicles_dir},
            {"logs_dir", s.logs_dir}
        };
    }

    void from_json(const nlohmann::json& j, server& s) {
        s.obd2_can_device = j.at("obd2_can_device").template get<std::string>();
        s.obd2_can_bitrate = j.at("obd2_can_bitrate").template get<uint32_t>();
        s.obd2_refresh_ms = j.at("obd2_refresh_ms").template get<uint32_t>();
        s.obd2_use_pid_chaining = j.at("obd2_use_pid_chaining").template get<bool>();
        s.obd2_skip_can_setup = j.at("obd2_skip_can_setup").template get<bool>();
        s.server_address = j.at("server_address").template get<std::string>();
        s.server_port = j.at("server_port").template get<uint16_t>();
        s.config_path = j.at("config_path").template get<std::string>();
        s.dashboards_dir = j.at("dashboards_dir").template get<std::string>();
        s.vehicles_dir = j.at("vehicles_dir").template get<std::string>();
        s.logs_dir = j.at("logs_dir").template get<std::string>();
    }
}
