#pragma once

#include <atomic>
#include <obd2.h>
#include <unordered_map>
#include <uuid_v4.h>
#include <vector>

#include "../vehicle/vehicle.h"

namespace obd2_server {
    class obd2_bridge {
        public:
            obd2_bridge();
            obd2_bridge(const std::string &device, bool skip_can_setup, bool enable_bitrate_discovery, 
                uint32_t bitrate, uint32_t refresh_ms = 1000, bool enable_pid_chaining = false);
            obd2_bridge(const obd2_bridge &o) = delete;
            obd2_bridge(obd2_bridge &&o) = delete;
            ~obd2_bridge();

            obd2_bridge &operator=(const obd2_bridge &o) = delete;
            obd2_bridge &operator=(obd2_bridge &&o) = delete;

            void connection_loop();

            bool register_request(const obd2_server::request &request);
            bool unregister_request(const UUIDv4::UUID &id);
            void clear_requests();
            bool request_registered(const UUIDv4::UUID &id);
            float get_request_val(const UUIDv4::UUID &id);
            const std::vector<uint8_t> &get_request_raw(const UUIDv4::UUID &id);
            std::vector<UUIDv4::UUID> supported_requests(const std::vector<obd2_server::request> &requests);

            std::unordered_map<std::string, std::vector<obd2::dtc>> get_dtcs(); // ECU name => DTCs
            void clear_dtcs();

            void set_obd2_refresh_cb(const std::function<void()> &cb);
            void set_can_bitrate(uint32_t bitrate);
            void set_can_refresh_ms(uint32_t refresh_ms);
            void set_bitrate_discovery(bool enable);

            bool get_is_connected() const;
            bool get_bitrate_discovery() const;
            uint32_t get_can_bitrate() const;
            uint32_t get_can_refresh_ms() const;

        private:
            static const std::chrono::milliseconds CONNECTION_CHECK_INTERVAL;
            static const uint32_t BITRATES[];

            obd2::obd2 instance;
            std::unordered_map<UUIDv4::UUID, obd2::request> requests;
            uint32_t can_bitrate;
            std::string can_device;
            bool skip_can_setup;
            bool enable_bitrate_discovery;

            std::thread connection_thread;
            std::atomic<bool> connection_thread_running = false;
            std::atomic<bool> is_connected = false;

            void set_next_bitrate();
            void setup_can_device();
	    void shutdown_can_device();
    };
}

namespace obd2 {
    void to_json(nlohmann::json& j, const dtc& d);
}
