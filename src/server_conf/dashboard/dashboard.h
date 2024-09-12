#pragma once

#include <cstdint>
#include <string>
#include <uuid_v4.h>
#include <json.hpp>

namespace obd2_server {
    class dashboard {
        public:    
            UUIDv4::UUID id;
            std::string name;
            std::vector<UUIDv4::UUID> requests;

            dashboard();

            bool operator==(const dashboard &r) const;
    };
    
    void to_json(nlohmann::json& j, const dashboard& d);
    void from_json(const nlohmann::json& j, dashboard& d);
}