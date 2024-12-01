#include "obd2_bridge.h"

namespace obd2_server {
    const std::chrono::milliseconds obd2_bridge::CONNECTION_CHECK_INTERVAL = std::chrono::milliseconds(5000);
    
    // Typical CAN bus bitrates for obd2
    const uint32_t obd2_bridge::BITRATES[] = { 
        500000, // 500kHz
        250000, // 250kHz
        125000, // 125kHz
    };

    obd2_bridge::obd2_bridge() { }

    obd2_bridge::obd2_bridge(const std::string &device, bool skip_can_setup, bool enable_bitrate_discovery,
        uint32_t bitrate, uint32_t refresh_ms, bool enable_pid_chaining)
        : can_bitrate(bitrate), can_device(device), skip_can_setup(skip_can_setup), enable_bitrate_discovery(enable_bitrate_discovery) { 

        setup_can_device();

        // Only after the device is set up we can start the obd2 instance
        instance = obd2::obd2(device.c_str(), refresh_ms, enable_pid_chaining);
        instance.set_refreshed_cb(std::bind(&obd2_bridge::handle_obd2_refreshed, this));

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
            while (!(is_connected = instance.is_connection_active())
                && connection_thread_running) {
                if (!enable_bitrate_discovery) {
                    std::cout << "No connection active" << std::endl;
                    continue;
                }

                try {
                    std::cout << "No connection active. Trying next bitrate..." << std::endl;
                    set_next_bitrate();
                }
                catch (const std::exception &e) {
                    std::cerr << "Error changing bitrate: " << e.what() << std::endl;
                }
            }

            std::cout << "Connection active" << std::endl;

            const auto start = std::chrono::steady_clock::now();
            const auto end = start + CONNECTION_CHECK_INTERVAL;
            const auto sleep_time = std::chrono::milliseconds(1000);

            while (connection_thread_running && std::chrono::steady_clock::now() < end) {
                std::this_thread::sleep_for(sleep_time);
            }
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

    void obd2_bridge::await_new_data() {
        while (!has_new_data) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        has_new_data = false;
    }

    std::unordered_map<std::string, std::vector<obd2::dtc>> obd2_bridge::get_dtcs() {
        std::unordered_map<std::string, std::vector<obd2::dtc>> dtcs;

        for (const auto &ecu : instance.get_ecus()) {
            std::vector<obd2::dtc> ecu_dtcs = instance.get_dtcs(ecu.get().get_id());
            std::string ecu_name = ecu.get().get_name();

            // If the ecu did not provide a name, use the id
            if (ecu_name.empty()) {
                std::stringstream ss;
                ss << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << ecu.get().get_id();
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
        std::lock_guard<std::mutex> refreshed_cb_lock(refreshed_cb_mutex);
        refreshed_cb = cb;
    }

    void obd2_bridge::set_can_bitrate(uint32_t bitrate) {
        can_bitrate = bitrate;

        setup_can_device();
    }

    void obd2_bridge::set_can_refresh_ms(uint32_t refresh_ms) {
        instance.set_refresh_ms(refresh_ms);
    }

    void obd2_bridge::set_bitrate_discovery(bool enable) {
        enable_bitrate_discovery = enable;
    }
    
    bool obd2_bridge::get_is_connected() const {
        return is_connected;
    }

    uint32_t obd2_bridge::get_can_bitrate() const {
        return can_bitrate;
    }

    uint32_t obd2_bridge::get_can_refresh_ms() const {
        return instance.get_refresh_ms();
    }

    bool obd2_bridge::get_bitrate_discovery() const {
        return enable_bitrate_discovery;
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
        set_can_bitrate(BITRATES[bitrate_index]);
    }

    void obd2_bridge::handle_obd2_refreshed() {
        has_new_data = true;
        
        std::lock_guard<std::mutex> refreshed_cb_lock(refreshed_cb_mutex);

        if (refreshed_cb) {
            refreshed_cb();
        }
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
