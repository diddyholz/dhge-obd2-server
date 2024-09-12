#pragma once

#include <cstdint>
#include <string>
#include <json.hpp>

#include "vehicle/vehicle.h"
#include "dashboard/dashboard.h"

namespace obd2_server {
    class server {
        public:
            static const std::string DEFAULT_OBD2_CAN_DEVICE;
            static const uint32_t DEFAULT_OBD2_CAN_BITRATE;
            static const uint32_t DEFAULT_OBD2_REFRESH_MS;
            static const bool DEFAULT_OBD2_USE_PID_CHAINING;

            static const std::string DEFAULT_SERVER_ADDRESS;
            static const uint16_t DEFAULT_SERVER_PORT;

            static const std::string DEFAULT_CONFIG_PATH;
            static const std::string DEFAULT_DASHBOARDS_PATH;
            static const std::string DEFAULT_VEHICLES_PATH;

            server();
            server(std::string server_config);

            void start_server();
            void stop_server();

            void set_obd2_can_device(const std::string &device);
            void set_obd2_can_bitrate(uint32_t bitrate);
            void set_obd2_refresh_ms(uint32_t refresh_ms);
            void set_obd2_use_pid_chaining(bool use_pid_chaining);
            void set_server_address(const std::string &address);
            void set_server_port(uint16_t port);
            void set_config_path(const std::string &path);
            void set_dashboards_path(const std::string &path);
            void set_vehicles_path(const std::string &path);

            const std::string &get_obd2_can_device() const;
            uint32_t get_obd2_can_bitrate() const;
            uint32_t get_obd2_refresh_ms() const;
            bool get_obd2_use_pid_chaining() const;
            const std::string &get_server_address() const;
            uint16_t get_server_port() const;
            const std::string &get_config_path() const;
            const std::string &get_dashboards_path() const;
            const std::string &get_vehicles_path() const;

        private:
            std::string obd2_can_device = DEFAULT_OBD2_CAN_DEVICE;
            uint32_t obd2_can_bitrate   = DEFAULT_OBD2_CAN_BITRATE;
            uint32_t obd2_refresh_ms    = DEFAULT_OBD2_REFRESH_MS;
            bool obd2_use_pid_chaining  = DEFAULT_OBD2_USE_PID_CHAINING;

            std::string server_address  = DEFAULT_SERVER_ADDRESS;
            uint16_t server_port        = DEFAULT_SERVER_PORT;

            std::string config_path     = DEFAULT_CONFIG_PATH;
            std::string dashboards_path = DEFAULT_DASHBOARDS_PATH;
            std::string vehicles_path   = DEFAULT_VEHICLES_PATH;

            std::vector<vehicle> vehicles;
            std::vector<dashboard> dashboards;

            bool load_server_config();
            uint32_t load_vehicles();
            uint32_t load_dashboards();

            friend void to_json(nlohmann::json& j, const server& s);
            friend void from_json(const nlohmann::json& j, server& s);
    };

    void to_json(nlohmann::json& j, const server& s);
    void from_json(const nlohmann::json& j, server& s);
}