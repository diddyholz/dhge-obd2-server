#pragma once

#include <cstdint>
#include <fstream>
#include <json.hpp>
#include <list>
#include <string>
#include <uuid_v4.h>

#include "request_entry/request_entry.h"

namespace obd2_server {
    class dashboard {
        public:    
            dashboard();
            dashboard(const std::string &path);
            dashboard(const std::string &name, const std::string &path);

            bool operator==(const dashboard &r) const;

            void add_request(const request_entry &r);
            void update(const nlohmann::json &j);
            void save() const;
            void delete_file();

            const UUIDv4::UUID &get_id() const;
            const std::string &get_name() const;
            const std::list<request_entry> &get_requests() const;
        
        private:
            UUIDv4::UUID id;
            std::string name;
            std::list<request_entry> requests;

            std::string path;

            friend void to_json(nlohmann::json& j, const dashboard& d);
            friend void from_json(const nlohmann::json& j, dashboard& d);
    };
    
    void to_json(nlohmann::json& j, const dashboard& d);
    void from_json(const nlohmann::json& j, dashboard& d);
}