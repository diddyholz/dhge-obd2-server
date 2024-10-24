#include "server.h"

#include <fstream>

namespace obd2_server {
    const std::string server::DEFAULT_OBD2_CAN_DEVICE   = "can0";
    const uint32_t server::DEFAULT_OBD2_CAN_BITRATE     = 500000;
    const uint32_t server::DEFAULT_OBD2_REFRESH_MS      = 1000;
    const bool server::DEFAULT_OBD2_USE_PID_CHAINING    = false;
    const bool server::DEFAULT_OBD2_SKIP_CAN_SETUP      = false;
    const bool server::DEFAULT_OBD2_BITRATE_DISCOVERY   = true;

    const std::string server::DEFAULT_SERVER_ADDRESS    = "0.0.0.0";
    const uint16_t server::DEFAULT_SERVER_PORT          = 38380;

    const std::string server::DEFAULT_CONFIG_PATH       = "$HOME/.config/obd2-server/config.json";
    const std::string server::DEFAULT_DASHBOARDS_DIR    = "$HOME/.config/obd2-server/dashboards";
    const std::string server::DEFAULT_VEHICLES_DIR      = "$HOME/.config/obd2-server/vehicles";
    const std::string server::DEFAULT_LOGS_DIR          = "$HOME/.config/obd2-server/logs";

    bool server::load_server_config() {
        std::ifstream file(get_config_path());

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
        std::filesystem::path path(get_vehicles_dir());

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
        std::filesystem::path path(get_dashboards_dir());

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
        std::filesystem::path path(get_logs_dir());

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

        std::ofstream file(get_config_path(), std::fstream::trunc);
        file << j.dump(4);
    }

    void server::make_directories() {
        // Strip filename from config path
        size_t last_slash = config_path.find_last_of("/\\");
        std::string config_dir = config_path.substr(0, last_slash);

        std::filesystem::create_directories(expand_path(config_dir));
        std::filesystem::create_directories(get_dashboards_dir());
        std::filesystem::create_directories(get_vehicles_dir());
        std::filesystem::create_directories(get_logs_dir());
    }

    std::string server::expand_path(const std::string &path) const {
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
            {"obd2_can_device", s.get_obd2_can_device()},
            {"obd2_can_bitrate", s.get_obd2_can_bitrate()},
            {"obd2_refresh_ms", s.get_obd2_refresh_ms()},
            {"obd2_bitrate_discovery", s.get_obd2_bitrate_discovery()},
            // {"obd2_use_pid_chaining", s.get_obd2_use_pid_chaining()},
            {"obd2_skip_can_setup", s.get_obd2_skip_can_setup()},
            {"server_address", s.get_server_address()},
            {"server_port", s.get_server_port()},
            {"config_path", s.config_path},
            {"dashboards_dir", s.dashboards_dir},
            {"vehicles_dir", s.vehicles_dir},
            {"logs_dir", s.logs_dir}
        };
    }

    void from_json(const nlohmann::json& j, server& s) {
        auto json_it = j.find("obd2_can_device");

        if (json_it != j.end()) {
            s.set_obd2_can_device(json_it->template get<std::string>());
        }

        if ((json_it = j.find("obd2_can_bitrate")) != j.end()) {
            s.set_obd2_can_bitrate(json_it->template get<uint32_t>());
        }

        if ((json_it = j.find("obd2_refresh_ms")) != j.end()) {
            s.set_obd2_refresh_ms(json_it->template get<uint32_t>());
        }

        if ((json_it = j.find("obd2_bitrate_discovery")) != j.end()) {
            s.set_obd2_bitrate_discovery(json_it->template get<bool>());
        }

        // if ((json_it = j.find("obd2_use_pid_chaining")) != j.end()) {
        //     s.set_obd2_use_pid_chaining(json_it->template get<bool>());
        // }

        if ((json_it = j.find("obd2_skip_can_setup")) != j.end()) {
            s.set_obd2_skip_can_setup(json_it->template get<bool>());
        }

        if ((json_it = j.find("server_address")) != j.end()) {
            s.set_server_address(json_it->template get<std::string>());
        }

        if ((json_it = j.find("server_port")) != j.end()) {
            s.set_server_port(json_it->template get<uint16_t>());
        }

        if ((json_it = j.find("config_path")) != j.end()) {
            s.set_config_path(json_it->template get<std::string>());
        }

        if ((json_it = j.find("dashboards_dir")) != j.end()) {
            s.set_dashboards_dir(json_it->template get<std::string>());
        }

        if ((json_it = j.find("vehicles_dir")) != j.end()) {
            s.set_vehicles_dir(json_it->template get<std::string>());
        }

        if ((json_it = j.find("logs_dir")) != j.end()) {
            s.set_logs_dir(json_it->template get<std::string>());
        }
    }
}
