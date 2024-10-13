#pragma once

#include <cstdint>
#include <uuid_v4.h>

namespace obd2_server {
    class request_entry {
        public:
            request_entry();
            request_entry(UUIDv4::UUID req_id, std::string display_type, float min_val, float max_val);
        
            UUIDv4::UUID req_id;
            std::string display_type;

            // Request overwrites
            float min_val;
            float max_val;
    };
    
    void to_json(nlohmann::json& j, const request_entry& d);
    void from_json(const nlohmann::json& j, request_entry& d);
}
