#include "dashboard.h"

#include <fstream>
#include <json.hpp>

namespace obd2_server {
    dashboard::dashboard() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) {}

    dashboard::dashboard(const std::string &path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }

        nlohmann::json j;

        try {
            j = nlohmann::json::parse(file);
        }
        catch (std::exception &e) {
            throw std::runtime_error("Could not parse JSON: " + std::string(e.what()));
        }

        from_json(j, *this);
    }

    bool dashboard::operator==(const dashboard &r) const {
        return id == r.id;
    }

    void to_json(nlohmann::json& j, const dashboard& d) {
        j = nlohmann::json{
            {"id", d.id},
            {"name", d.name},
            {"requests", d.requests}
        };
    }

    void from_json(const nlohmann::json& j, dashboard& d) {
        d.id = UUIDv4::UUID::fromStrFactory(j.at("id").template get<std::string>());
        d.name = j.at("name").template get<std::string>();
        d.requests = j.at("requests").template get<std::list<UUIDv4::UUID>>();
    }
}