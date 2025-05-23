#pragma once

#include <cstdint>
#include <vector>
#include <uuid_v4.h>
#include "request/request.h"

namespace obd2_server {
    class vehicle {
        public:
            vehicle();
            vehicle(const std::string &definition_file);
            vehicle(const std::string &make, const std::string &model);

            bool operator==(const vehicle &v) const;

            void add_request(const request &r);
            void remove_request(const request &r);

            const UUIDv4::UUID &get_id() const;
            const std::string &get_make() const;
            const std::string &get_model() const;
            request &get_request(const UUIDv4::UUID &id);
            const std::vector<request> &get_requests() const;

        private:
            UUIDv4::UUID id;

            std::string make;
            std::string model;
            std::vector<request> requests;

            friend void to_json(nlohmann::json& j, const vehicle& v);
            friend void from_json(const nlohmann::json& j, vehicle& v);
    };
            
    void to_json(nlohmann::json& j, const vehicle& v);
    void from_json(const nlohmann::json& j, vehicle& v);
};
