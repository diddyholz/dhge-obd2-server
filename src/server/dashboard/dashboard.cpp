#include "dashboard.h"

#include <fstream>
#include <json.hpp>

#define CHECK_PATH() if (path.empty()) { throw std::runtime_error("Path is empty."); }

namespace obd2_server {
    dashboard::dashboard() : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()) {}

    dashboard::dashboard(const std::string &path) : path(path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }

        nlohmann::json j;

        try {
            j = nlohmann::json::parse(file);
        }
        catch (std::exception &e) {
            throw std::runtime_error("Could not parse JSON: " + std::string(e.what()));
        }

        from_json(j, *this);
    }

    dashboard::dashboard(const std::string &name, const std::string &path) 
        : id(UUIDv4::UUIDGenerator<std::mt19937>().getUUID()), name(name), path(path) { }

    bool dashboard::operator==(const dashboard &r) const {
        return id == r.id;
    }

    void dashboard::add_request(const request_entry &r) {
        requests.push_back(r);
    }

    void dashboard::save() const {
        CHECK_PATH();

        std::ofstream file(path, std::fstream::trunc);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }

        nlohmann::json j = *this;
        file << j.dump(4);
    }

    void dashboard::delete_file() {
        CHECK_PATH();
        
        std::remove(path.c_str());
        path.clear();
    }

    void dashboard::update(const nlohmann::json &j) {
        // Update name
        auto it_body = j.find("name");

        if (it_body != j.end()) {
            name = it_body->get<std::string>();
        }

        // Update requests
        it_body = j.find("requests");

        if (it_body != j.end()) {
            requests = it_body->get<std::list<request_entry>>();
        }
    }

    const UUIDv4::UUID &dashboard::get_id() const {
        return id;
    }

    const std::string &dashboard::get_name() const {
        return name;
    }

    const std::list<request_entry> &dashboard::get_requests() const {
        return requests;
    }

    void to_json(nlohmann::json& j, const dashboard& d) {
        j = nlohmann::json{
            {"id", d.id},
            {"name", d.name},
            {"requests", d.requests}
        };
    }

    void from_json(const nlohmann::json& j, dashboard& d) {
        d.id = UUIDv4::UUID::fromStrFactory(j.at("id").template get<std::string>());
        d.name = j.at("name").template get<std::string>();
        d.requests = j.at("requests").template get<std::list<request_entry>>();
    }
}