#include "server.h"

#include <fstream>
#include <iostream>

namespace obd2_server {
    const std::string server::DEFAULT_OBD2_CAN_DEVICE = "can0";
    const uint32_t server::DEFAULT_OBD2_CAN_BITRATE = 500000;
    const uint32_t server::DEFAULT_OBD2_REFRESH_MS = 1000;
    const bool server::DEFAULT_OBD2_USE_PID_CHAINING = false;

    const std::string server::DEFAULT_SERVER_ADDRESS = "0.0.0.0";
    const uint16_t server::DEFAULT_SERVER_PORT = 38380;

    const std::string server::DEFAULT_CONFIG_PATH = "config.json";
    const std::string server::DEFAULT_DASHBOARDS_PATH = "dashboards";
    const std::string server::DEFAULT_VEHICLES_PATH = "vehicles";

    server::server() : server(DEFAULT_CONFIG_PATH) { }

    server::server(std::string server_config) : config_path(server_config) {
        std::cout << "Loading server configuration from " << config_path << std::endl;

        if (!load_server_config()) {
            std::cout << "Could not load config file, using defaults" << std::endl;
        }

        std::cout << "Loaded " << load_vehicles() << " vehicle definitions" << std::endl;
        std::cout << "Loaded " << load_dashboards() << " dashboards" << std::endl;

        setup_routes();
    }

    void server::start_server() {
        if (server_instance.is_running()) {
            std::cout << "Server already running" << std::endl;
            return;
        }

        std::cout << "Starting server..." << std::endl;
        
        server_thread = std::thread(
            &httplib::Server::listen, 
            &server_instance, 
            server_address.c_str(), 
            server_port
        );

        std::cout << "Server started at " << server_address << ":" << server_port << std::endl;
    }

    void server::stop_server() {
        std::cout << "Stopping server..." << std::endl;
        server_instance.stop();
    }

    void server::set_obd2_can_device(const std::string &device) {
        obd2_can_device = device;
    }

    void server::set_obd2_can_bitrate(uint32_t bitrate) {
        obd2_can_bitrate = bitrate;
    }

    void server::set_obd2_refresh_ms(uint32_t refresh_ms) {
        obd2_refresh_ms = refresh_ms;
    }

    void server::set_obd2_use_pid_chaining(bool use_pid_chaining) {
        obd2_use_pid_chaining = use_pid_chaining;
    }

    void server::set_server_address(const std::string &address) {
        server_address = address;
    }

    void server::set_server_port(uint16_t port) {
        server_port = port;
    }

    void server::set_config_path(const std::string &path) {
        config_path = path;
    }

    void server::set_dashboards_path(const std::string &path) {
        dashboards_path = path;
    }

    void server::set_vehicles_path(const std::string &path) {
        vehicles_path = path;
    }

    const std::string &server::get_obd2_can_device() const {
        return obd2_can_device;
    }

    uint32_t server::get_obd2_can_bitrate() const {
        return obd2_can_bitrate;
    }

    uint32_t server::get_obd2_refresh_ms() const {
        return obd2_refresh_ms;
    }

    bool server::get_obd2_use_pid_chaining() const {
        return obd2_use_pid_chaining;
    }

    const std::string &server::get_server_address() const {
        return server_address;
    }

    uint16_t server::get_server_port() const {
        return server_port;
    }

    const std::string &server::get_config_path() const {
        return config_path;
    }

    const std::string &server::get_dashboards_path() const {
        return dashboards_path;
    }

    const std::string &server::get_vehicles_path() const {
        return vehicles_path;
    }

    request &server::get_request(const UUIDv4::UUID &id) {
        UUIDv4::UUID vehicle_id = request_vehicle_map.at(id);
        auto it = vehicles.find(vehicle_id);

        if (it == vehicles.end()) {
            throw std::runtime_error("Vehicle not found");
        }

        return it->second.get_request(id);
    }

    void to_json(nlohmann::json& j, const server& s) {
        j = nlohmann::json{
            {"obd2_can_device", s.obd2_can_device},
            {"obd2_can_bitrate", s.obd2_can_bitrate},
            {"obd2_refresh_ms", s.obd2_refresh_ms},
            {"obd2_use_pid_chaining", s.obd2_use_pid_chaining},
            {"server_address", s.server_address},
            {"server_port", s.server_port},
            {"config_path", s.config_path},
            {"dashboards_path", s.dashboards_path},
            {"vehicles_path", s.vehicles_path}
        };
    }

    void from_json(const nlohmann::json& j, server& s) {
        s.obd2_can_device = j.at("obd2_can_device").template get<std::string>();
        s.obd2_can_bitrate = j.at("obd2_can_bitrate").template get<uint32_t>();
        s.obd2_refresh_ms = j.at("obd2_refresh_ms").template get<uint32_t>();
        s.obd2_use_pid_chaining = j.at("obd2_use_pid_chaining").template get<bool>();
        s.server_address = j.at("server_address").template get<std::string>();
        s.server_port = j.at("server_port").template get<uint16_t>();
        s.config_path = j.at("config_path").template get<std::string>();
        s.dashboards_path = j.at("dashboards_path").template get<std::string>();
        s.vehicles_path = j.at("vehicles_path").template get<std::string>();
    }
}
