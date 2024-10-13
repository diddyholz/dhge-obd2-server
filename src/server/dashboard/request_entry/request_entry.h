#pragma once

#include <cstdint>
#include <uuid_v4.h>

namespace obd2_server {
    class request_entry {
        public:
            request_entry();
            request_entry(UUIDv4::UUID req_id, std::string display_type, 
                float min = std::numeric_limits<float>::quiet_NaN(), 
                float max = std::numeric_limits<float>::quiet_NaN());
        
            UUIDv4::UUID req_id;
            std::string display_type;

            // Request overwrites
            float min;
            float max;
    };
    
    void to_json(nlohmann::json& j, const request_entry& d);
    void from_json(const nlohmann::json& j, request_entry& d);
}
