#include "server.h"

namespace obd2_server {
    void server::setup_routes() {
        server_instance.Get(
            "/vehicles", 
            std::bind(
                &server::handle_get_vehicles, 
                this, 
                std::placeholders::_1, 
                std::placeholders::_2
            )
        );

        server_instance.Get(
            "/dashboards",
            std::bind(
                &server::handle_get_dashboards,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        server_instance.Get(
            "/data",
            std::bind(
                &server::handle_get_data,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        server_instance.Get(
            "/dtcs",
            std::bind(
                &server::handle_get_dtcs,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            ) 
        );
    }

    void server::handle_get_vehicles(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j = vehicles;

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_dashboards(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j = dashboards;

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_data(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j;
        std::vector<UUIDv4::UUID> ids;
        std::unordered_map<UUIDv4::UUID, float> data;
        auto first_it = req.params.find("id");

        if (first_it == req.params.end()) {
            j["error"] = "Missing id parameter";
            res.status = 400;
            res.set_content(j.dump(), "application/json");
            return;
        }
        
        ids = split_ids(first_it->second, ',');
        data = get_data_for_ids(ids);    
        j = data;

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_dtcs(const httplib::Request &req, httplib::Response &res) {
        auto dtcs = obd2.get_dtcs();   
        nlohmann::json j = dtcs;

        res.set_content(j.dump(), "application/json");
    }

    std::unordered_map<UUIDv4::UUID, float> server::get_data_for_ids(const std::vector<UUIDv4::UUID> &ids) {
        std::unordered_map<UUIDv4::UUID, float> data;

        for (const auto &id : ids) {
            if (!obd2.request_registered(id)) {
                obd2.register_request(get_request(id));
            }

            data[id] = obd2.get_request_val(id);
        }

        return data;
    }

    std::vector<UUIDv4::UUID> server::split_ids(const std::string &s, char delim) {
        std::vector<UUIDv4::UUID> elems;
        std::stringstream ss(s);
        std::string item;

        while (std::getline(ss, item, delim)) {
            elems.emplace_back(UUIDv4::UUID::fromStrFactory(item));
        }

        return elems;
    }
}
