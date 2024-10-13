#pragma once

#include <unordered_map>
#include <vector>
#include <uuid_v4.h>
#include <obd2.h>
#include "../vehicle/vehicle.h"

namespace obd2_server {
    class obd2_bridge {
        public:
            obd2_bridge();
            obd2_bridge(const std::string &device, bool skip_can_setup, uint32_t bitrate, uint32_t refresh_ms = 1000, bool enable_pid_chaining = false);

            bool register_request(const obd2_server::request &request);
            bool unregister_request(const UUIDv4::UUID &id);
            void clear_requests();
            bool request_registered(const UUIDv4::UUID &id);
            float get_request_val(const UUIDv4::UUID &id);
            const std::vector<uint8_t> &get_request_raw(const UUIDv4::UUID &id);
            std::unordered_map<std::string, std::vector<obd2::dtc>> get_dtcs(); // ECU name => DTCs
            void clear_dtcs();

            std::vector<UUIDv4::UUID> supported_requests(const std::vector<obd2_server::request> &requests);

        private:
            obd2::obd2 instance;
            std::unordered_map<UUIDv4::UUID, obd2::request> requests;
            uint32_t can_bitrate;
            std::string can_device;
            
            void setup_can_device();
    };
}

namespace obd2 {
    void to_json(nlohmann::json& j, const dtc& d);
}
