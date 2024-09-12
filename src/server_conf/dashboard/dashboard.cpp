#include "dashboard.h"

namespace obd2_server {
    dashboard::dashboard() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) {}

    bool dashboard::operator==(const dashboard &r) const {
        return id == r.id;
    }

    void to_json(nlohmann::json& j, const dashboard& d) {
        j = nlohmann::json{
            {"id", d.id.str()},
            {"name", d.name},
            {"requests", d.requests}
        };
    }

    void from_json(const nlohmann::json& j, dashboard& d) {
        d.id = UUIDv4::UUID::fromStrFactory(j.at("id").template get<std::string>());
        d.name = j.at("name").template get<std::string>();

        for (const auto &r : j.at("requests")) {
            UUIDv4::UUID req_id = UUIDv4::UUID::fromStrFactory(r.template get<std::string>());
            d.requests.emplace_back(req_id);
        }
    }
}