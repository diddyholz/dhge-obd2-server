#include "obd2_bridge.h"

namespace obd2_server {
    obd2_bridge::obd2_bridge() { }

    obd2_bridge::obd2_bridge(const std::string &device, uint32_t bitrate, uint32_t refresh_ms, bool enable_pid_chaining)
        : instance(obd2::obd2(device.c_str(), refresh_ms, enable_pid_chaining)), can_bitrate(bitrate), can_device(device) { 
        setup_can_device();
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

    std::vector<UUIDv4::UUID> obd2_bridge::supported_requests(const std::vector<obd2_server::request> &requests) {
        std::vector<UUIDv4::UUID> supported;

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
