#include "obd2_bridge.h"

namespace obd2_server {
    const std::chrono::milliseconds obd2_bridge::CONNECTION_CHECK_INTERVAL = std::chrono::milliseconds(30000);
    
    // Typical CAN bus bitrates for obd2
    const uint32_t obd2_bridge::BITRATES[] = { 
        500000, // 500kHz
        250000, // 250kHz
        125000, // 125kHz
    };

    obd2_bridge::obd2_bridge() { }

    obd2_bridge::obd2_bridge(const std::string &device, bool skip_can_setup, uint32_t bitrate, uint32_t refresh_ms, bool enable_pid_chaining)
        : can_bitrate(bitrate), can_device(device) { 

        if (!skip_can_setup) {
            setup_can_device();
        }

        // Only after the device is set up we can start the obd2 instance
        instance = obd2::obd2(device.c_str(), refresh_ms, enable_pid_chaining);

        // Also start connection loop
        connection_thread_running = true;
        connection_thread = std::thread(&obd2_bridge::connection_loop, this);
    }

    obd2_bridge::~obd2_bridge() {
        if (connection_thread_running) {
            connection_thread_running = false;
            connection_thread.join();
        }
    }

    void obd2_bridge::connection_loop() {
        while (connection_thread_running) {
            // Cycle bitrates if connection is not active
            while (!(is_connected = instance.is_connection_active()) && connection_thread_running) {
                set_next_bitrate();

                try {
                    setup_can_device();
                }
                catch (const std::exception &e) {
                    std::cerr << "Error changing bitrate: " << e.what() << std::endl;
                }
            }

            std::this_thread::sleep_for(CONNECTION_CHECK_INTERVAL);
        }
    }

    bool obd2_bridge::register_request(const obd2_server::request &request) {
        if (requests.find(request.id) != requests.end()) {
            return false;
        }

        requests.try_emplace(request.id, request.ecu, request.service, request.pid, instance, request.formula, true);
        return true;
    }

    bool obd2_bridge::unregister_request(const UUIDv4::UUID &id) {
        if (requests.find(id) == requests.end()) {
            return false;
        }
        
        requests.erase(id);
        return true;
    }

    void obd2_bridge::clear_requests() {
        requests.clear();
    }

    bool obd2_bridge::request_registered(const UUIDv4::UUID &id) {
        return requests.find(id) != requests.end();
    }

    float obd2_bridge::get_request_val(const UUIDv4::UUID &id) {
        if (requests.find(id) == requests.end()) {
            return std::numeric_limits<float>::quiet_NaN();
        }

        return requests[id].get_value();
    }

    const std::vector<uint8_t> &obd2_bridge::get_request_raw(const UUIDv4::UUID &id) {
        if (requests.find(id) == requests.end()) {
            static const std::vector<uint8_t> empty;
            return empty;
        }

        return requests[id].get_raw();
    }

    std::vector<UUIDv4::UUID> obd2_bridge::supported_requests(const std::vector<obd2_server::request> &requests) {
        std::vector<UUIDv4::UUID> supported;

        // When no connection has been established, return empty list
        if (!is_connected) {
            return supported;
        }

        for (const auto &r : requests) {
            if (instance.pid_supported(r.ecu, r.service, r.pid)) {
                supported.push_back(r.id);
            }
        }

        return supported;
    }

    std::unordered_map<std::string, std::vector<obd2::dtc>> obd2_bridge::get_dtcs() {
        std::unordered_map<std::string, std::vector<obd2::dtc>> dtcs;

        for (const auto &ecu : instance.get_ecus()) {
            std::vector<obd2::dtc> ecu_dtcs = instance.get_dtcs(ecu.get().get_id());
            std::string ecu_name = ecu.get().get_name();

            // If the ecu did not provide a name, use the id
            if (ecu_name.empty()) {
                std::stringstream ss;
                ss << std::hex << std::setw(3) << std::setfill('0') << ecu.get().get_id();
                ecu_name = ss.str();
            }

            dtcs[ecu_name] = ecu_dtcs;
        }

        return dtcs;
    }

    void obd2_bridge::clear_dtcs() {
        for (const auto &ecu : instance.get_ecus()) {
            instance.clear_dtcs(ecu.get().get_id());
        }
    }

    void obd2_bridge::set_obd2_refresh_cb(const std::function<void()> &cb) {
        instance.set_refreshed_cb(cb);
    }
    
    bool obd2_bridge::get_is_connected() const {
        return is_connected;
    }

    void obd2_bridge::set_next_bitrate() {
        size_t bitrate_index = 0;
        size_t bitrate_count = sizeof(BITRATES) / sizeof(BITRATES[0]);

        for (size_t i = 0; i < bitrate_count; i++) {
            bitrate_index++;

            if (BITRATES[i] == can_bitrate) {
                break;
            }
        }

        bitrate_index = bitrate_index % bitrate_count;
        can_bitrate = BITRATES[bitrate_index];
    }
}

namespace obd2 {
    void to_json(nlohmann::json& j, const dtc& d) {
        j = nlohmann::json();
        
        j["code"] = d.str();

        std::stringstream ss;
        ss << d.get_status();
        j["status"] = ss.str();
    }
}
