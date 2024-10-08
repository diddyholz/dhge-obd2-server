#include "server.h"

#include <fstream>

namespace obd2_server {
    bool server::load_server_config() {
        std::ifstream file(config_path);

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
        std::filesystem::path path(vehicles_path);

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
        std::filesystem::path path(dashboards_path);

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
                dashboards.try_emplace(d.id, d);
            }
            catch (std::exception &e) {
                std::cerr << "Could not load vehicle from " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }

        return dashboards.size();
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
}
