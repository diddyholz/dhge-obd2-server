#include <request_entry.h>

namespace obd2_server {
    request_entry::request_entry() {}

    request_entry::request_entry(UUIDv4::UUID req_id, std::string display_type, float min_val, float max_val) 
        : req_id(req_id), display_type(display_type), min_val(min_val), max_val(max_val) {}

    void to_json(nlohmann::json& j, const request_entry& d) {
        j = nlohmann::json{
            {"req_id", d.req_id.str()},
            {"display_type", d.display_type},
            {"min_val", d.min_val},
            {"max_val", d.max_val}
        };
    }

    void from_json(const nlohmann::json& j, request_entry& d) {
        d.req_id = j.at("req_id").get<UUIDv4::UUID>();
        d.display_type = j.at("display_type").get<std::string>();
        d.min_val = j.at("min_val").get<float>();
        d.max_val = j.at("max_val").get<float>();
    }
}