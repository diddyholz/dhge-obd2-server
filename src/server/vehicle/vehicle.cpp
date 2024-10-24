#include "vehicle.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <json.hpp>

namespace obd2_server {
    vehicle::vehicle() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) { }

    vehicle::vehicle(const std::string &definition_file)
        : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) {
        std::ifstream file(definition_file);

        if (!file.is_open()) {
            throw std::invalid_argument("Could not open file");
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(file);
        }
        catch (std::exception &e) {
            throw std::invalid_argument(e.what());
        }

        from_json(j, *this);
    }

    vehicle::vehicle(const std::string &make, const std::string &model)
        : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()), make(make), model(model) { }

    bool vehicle::operator==(const vehicle &v) const {
        return id == v.id;
    }

    void vehicle::add_request(const request &r) {
        requests.push_back(r);
    }

    void vehicle::remove_request(const request &r) {
        auto element = std::find(requests.begin(), requests.end(), r);
        
        requests.erase(element);
    }

    const UUIDv4::UUID &vehicle::get_id() const {
        return id;
    }

    const std::string &vehicle::get_make() const {
        return make;
    }

    const std::string &vehicle::get_model() const {
        return model;
    }

    request &vehicle::get_request(const UUIDv4::UUID &id) {
        for (auto &r : requests) {
            if (r.id == id) {
                return r;
            }
        }

        throw std::invalid_argument("Request not found");
    }

    const std::vector<request> &vehicle::get_requests() const {
        return requests;
    }

    void to_json(nlohmann::json& j, const vehicle& v) {
        j = nlohmann::json{
            {"id", v.id},
            {"make", v.make},
            {"model", v.model},
            {"requests", v.requests}
        };
    }

    void from_json(const nlohmann::json& j, vehicle& v) {
        v.id = j.at("id").template get<UUIDv4::UUID>();
        v.make = j.at("make").template get<std::string>();
        v.model = j.at("model").template get<std::string>();
        v.requests = j.at("requests").template get<std::vector<request>>();
    }
}
