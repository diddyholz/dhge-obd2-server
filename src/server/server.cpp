#include "server.h"

#include <fstream>
#include <iostream>

namespace obd2_server {
    server::server() : server(DEFAULT_CONFIG_PATH) { }

    server::server(std::string server_config) : config_path(server_config) {
        make_directories();

        std::cout << "Loading server configuration from " << get_config_path() << std::endl;

        if (!load_server_config()) {
            std::cout << "Could not load config file, using defaults" << std::endl;
            save_server_config();
            std::cout << "Saved server configuration to " << get_config_path() << std::endl;
        }
        
        uint32_t loaded_vehicles = load_vehicles();
        uint32_t loaded_dashboards = load_dashboards();
        uint32_t loaded_logs = load_logs();

        std::cout << "Loaded " << loaded_vehicles << " vehicle definitions" << std::endl;
        std::cout << "Loaded " << loaded_dashboards << " dashboards" << std::endl;
        std::cout << "Loaded " << loaded_logs << " logs" << std::endl;

        setup_routes();

        // Initialize obd2 bridge
        obd2 = std::make_unique<obd2_bridge>(obd2_can_device, obd2_skip_can_setup, 
            obd2_bitrate_discovery, obd2_can_bitrate, obd2_refresh_ms, obd2_use_pid_chaining);
        obd2->set_obd2_refresh_cb(std::bind(&server::handle_obd2_refresh, this));
    }

    server::~server() {
        stop_server();
        save_server_config();
    }

    void server::start_server() {
        if (server_instance.is_running()) {
            std::cout << "Server already running" << std::endl;
            return;
        }

        std::cout << "Starting server at " << server_address << ":" << server_port << std::endl;
        
        server_listen();
    }

    void server::stop_server() {
        if (!server_instance.is_running()) {
            return;
        }

        std::cout << "Stopping server..." << std::endl;
        server_instance.stop();
    }

    void server::server_listen() {
        server_instance.listen(server_address, server_port);
    }

    void server::set_obd2_can_device(const std::string &device) {
        obd2_can_device = device;
    }

    void server::set_obd2_can_bitrate(uint32_t bitrate) {
        obd2_can_bitrate = bitrate;

        if (obd2) {
            obd2->set_can_bitrate(bitrate);
        }
    }

    void server::set_obd2_refresh_ms(uint32_t refresh_ms) {
        obd2_refresh_ms = refresh_ms;

        if (obd2) {
            obd2->set_can_refresh_ms(refresh_ms);
        }
    }

    void server::set_obd2_use_pid_chaining(bool use_pid_chaining) {
        obd2_use_pid_chaining = use_pid_chaining;
    }

    void server::set_obd2_bitrate_discovery(bool bitrate_discovery) {
        obd2_bitrate_discovery = bitrate_discovery;

        if (obd2) {
            obd2->set_bitrate_discovery(bitrate_discovery);
        }
    }

    void server::set_obd2_skip_can_setup(bool skip_can_setup) {
        obd2_skip_can_setup = skip_can_setup;
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

    void server::set_dashboards_dir(const std::string &path) {
        dashboards_dir = path;
    }

    void server::set_vehicles_dir(const std::string &path) {
        vehicles_dir = path;
    }

    void server::set_logs_dir(const std::string &path) {
        logs_dir = path;
    }

    const std::string &server::get_obd2_can_device() const {
        return obd2_can_device;
    }

    uint32_t server::get_obd2_can_bitrate() const {
        if (obd2) {
            return obd2->get_can_bitrate();
        }
        
        return obd2_can_bitrate;
    }

    uint32_t server::get_obd2_refresh_ms() const {
        if (obd2) {
            return obd2->get_can_refresh_ms();
        }

        return obd2_refresh_ms;
    }

    bool server::get_obd2_use_pid_chaining() const {
        return obd2_use_pid_chaining;
    }

    bool server::get_obd2_bitrate_discovery() const {
        if (obd2) {
            return obd2->get_bitrate_discovery();
        }

        return obd2_bitrate_discovery;
    }

    bool server::get_obd2_skip_can_setup() const {
        return obd2_skip_can_setup;
    }

    const std::string &server::get_server_address() const {
        return server_address;
    }

    uint16_t server::get_server_port() const {
        return server_port;
    }

    std::string server::get_config_path() const {
        return expand_path(config_path);
    }

    std::string server::get_dashboards_dir() const {
        return expand_path(dashboards_dir);
    }

    std::string server::get_vehicles_dir() const {
        return expand_path(vehicles_dir);
    }

    std::string server::get_logs_dir() const {
        return expand_path(logs_dir);
    }

    void server::handle_obd2_refresh() {
        process_logs();
    }

    void server::process_logs() {
        for (auto &log : logs) {
            if (!log.second.get_is_logging()) {
                continue;
            }

            if (log.second.get_is_raw()) {
                auto data = get_raw_data_for_ids(log.second.get_request_ids());
                log.second.add_data_raw(data);
            }
            else {
                std::unordered_map<UUIDv4::UUID, float> data = get_data_for_ids(log.second.get_request_ids());
                log.second.add_data(data);
            }
        }
    }

    request &server::get_request(const UUIDv4::UUID &id) {
        UUIDv4::UUID vehicle_id = request_vehicle_map.at(id);
        auto it = vehicles.find(vehicle_id);

        if (it == vehicles.end()) {
            throw std::runtime_error("Vehicle not found");
        }

        return it->second.get_request(id);
    }
}
