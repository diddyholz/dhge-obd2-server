#include "request.h"

namespace obd2_server {
    request::request() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) { }

    bool request::operator==(const request &r) const {
        return id == r.id;
    }
    
    void to_json(nlohmann::json& j, const request& r) {
        j = nlohmann::json{
            {"id", r.id.str()}, 
            {"name", r.name},
            {"description", r.description},
            {"category", r.category},
            {"ecu", r.ecu}, 
            {"service", r.service},
            {"pid", r.pid},
            {"formula", r.formula},
            {"unit", r.unit},
            {"min", r.min},
            {"max", r.max}
        };
    }

    void from_json(const nlohmann::json& j, request& r) {
        r.id = UUIDv4::UUID::fromStrFactory(j.at("id").template get<std::string>());
        r.name = j.at("name").template get<std::string>();
        r.description = j.at("description").template get<std::string>();
        r.category = j.at("category").template get<std::string>();
        r.ecu = j.at("ecu").template get<uint32_t>();
        r.service = j.at("service").template get<uint8_t>();
        r.pid = j.at("pid").template get<uint16_t>();
        r.formula = j.at("formula").template get<std::string>();
        r.unit = j.at("unit").template get<std::string>();
        r.min = j.at("min").template get<float>();
        r.max = j.at("max").template get<float>();
    }
}