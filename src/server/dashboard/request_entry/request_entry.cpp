#include "request_entry.h"

namespace obd2_server {
    request_entry::request_entry() {}

    request_entry::request_entry(UUIDv4::UUID req_id, std::string display_type, float min, float max) 
        : req_id(req_id), display_type(display_type), min(min), max(max) {}

    void to_json(nlohmann::json& j, const request_entry& d) {
        j = nlohmann::json{
            {"req_id", d.req_id},
        };

        // "display_type", "min" and "max" are optional
        if (!d.display_type.empty()) {
            j["display_type"] = d.display_type;
        }

        if (!std::isnan(d.min)) {
            j["min"] = d.min;
        }

        if (!std::isnan(d.max)) {
            j["max"] = d.max;
        }
    }

    void from_json(const nlohmann::json& j, request_entry& d) {
        d.req_id = j.at("req_id").get<UUIDv4::UUID>();
        
        // "display_type", "min" and "max" are optional
        auto it = j.find("display_type");

        if (it != j.end() && !it->is_null()) {
            d.display_type = it->get<std::string>();
        }
        else {
            d.display_type = "";
        }
        
        it = j.find("min");

        // Check if min is null
        if (it != j.end() && !it->is_null()) {
            d.min = it->get<float>();
        }
        else {
            d.min = std::numeric_limits<float>::quiet_NaN();
        }

        it = j.find("max");

        // Check if max is null
        if (it != j.end() && !it->is_null()) {
            d.max = it->get<float>();
        }
        else {
            d.max = std::numeric_limits<float>::quiet_NaN();
        }
    }
}