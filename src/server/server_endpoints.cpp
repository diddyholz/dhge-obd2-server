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

        server_instance.Get(
            "/logs",
            std::bind(
                &server::handle_get_log,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        server_instance.Post(
            "/logs",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_post_log,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        );
    }

    void server::handle_get_vehicles(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j = nlohmann::json::array();
        
        // Go through all vehicles in map and add them to the JSON array
        for (const auto &vehicle : vehicles) {
            const std::vector<UUIDv4::UUID> &supported_requests = obd2.supported_requests(vehicle.second.get_requests());
            nlohmann::json vehicle_j = vehicle.second;
            vehicle_j["supported_requests"] = supported_requests;
            
            j.push_back(vehicle_j);
        }

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_dashboards(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j = nlohmann::json::array();

        // Go through all dashboards in map and add them to the JSON array
        for (const auto &dashboard : dashboards) {
            nlohmann::json dashboard_j = dashboard.second;
            j.push_back(dashboard_j);
        }

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_data(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j;
        std::vector<UUIDv4::UUID> ids;
        std::unordered_map<UUIDv4::UUID, float> data;
        auto first_it = req.params.find("id");

        if (first_it == req.params.end()) {
            j["error"] = "Missing parameter 'id'";
            res.status = 400;
            res.set_content(j.dump(), "application/json");
            return;
        }
        
        ids = split_ids(first_it->second, ',');
        data = get_data_for_ids(ids);    
        to_json(j, data);

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_dtcs(const httplib::Request &req, httplib::Response &res) {
        auto dtcs = obd2.get_dtcs();   
        nlohmann::json j = dtcs;

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_log(const httplib::Request &req, httplib::Response &res) {
        std::string name = req.get_param_value("name");
        nlohmann::json j;

        // TODO: Not working

        // Return all logs (without data) if no name is provided
        if (name.empty()) {
            j = nlohmann::json::array();

            // Go through all logs in map and add them to the JSON array
            for (const auto &log : logs) {
                nlohmann::json log_j = log.second;
                j.push_back(log_j);
            }

            res.set_content(j.dump(), "application/json");
            return;            
        }

        // Check if log exists
        if (logs.find(name) == logs.end()) {
            j["error"] = "Log not found";
            res.status = 404;
            res.set_content(j.dump(), "application/json");
            return;
        }

        // Return log data
        try {
            j = logs[name];
            j["data"] = logs[name].get_csv_string();
        }
        catch (const std::exception &e) {
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
            return;
        }

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_post_log(const httplib::Request &req, httplib::Response &res) {
        if (req.has_param("stop")) {
            std::string name = req.get_param_value("stop");
            stop_log(name);

            res.status = 204;
            return;
        }

        // Get dashboard to log
        std::string dashboard_id_str = req.get_param_value("dashboard");
        nlohmann::json j;
        bool log_raw = false;

        if (req.has_param("raw")) {
            log_raw = req.get_param_value("raw") == "true";
        }

        if (dashboard_id_str.empty()) {
            j["error"] = "Missing parameter 'dashboard'";
            res.status = 400;
            return res.set_content(j.dump(), "application/json");
        }

        UUIDv4::UUID dashboard_id = UUIDv4::UUID::fromStrFactory(dashboard_id_str);
        auto it = dashboards.find(dashboard_id);

        if (it == dashboards.end()) {
            j["error"] = "Dashboard not found";
            res.status = 404;
            return res.set_content(j.dump(), "application/json");
        }

        std::string log_name;
    
        try {
            log_name = create_log(dashboard_id, log_raw);
        }
        catch(const std::exception &e) {
            j["error"] = e.what();
            res.status = 500;
            return res.set_content(j.dump(), "application/json");
        }

        j["name"] = log_name;

        res.set_content(j.dump(), "application/json");
    }

    std::string server::create_log(const UUIDv4::UUID &dashboard_id, bool log_raw) {
        auto it = dashboards.find(dashboard_id);

        if (it == dashboards.end()) {
            throw std::runtime_error("Dashboard not found [" + dashboard_id.str() + "]");
        }

        const dashboard &d = it->second;
        std::unordered_map<UUIDv4::UUID, std::string> requests;

        for (const auto &req_id : d.requests) {
            requests[req_id] = get_request(req_id).name;
        }

        data_log log(requests, expand_path(logs_dir), log_raw);
        std::string log_name = log.get_name();
        logs[log_name] = std::move(log);

        return log_name;
    }

    void server::stop_log(const std::string &name) {
        auto it = logs.find(name);

        if (it == logs.end()) {
            return;
        }
        
        it->second.stop_logging();
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

    std::unordered_map<UUIDv4::UUID, std::reference_wrapper<const std::vector<uint8_t>>> server::get_raw_data_for_ids(const std::vector<UUIDv4::UUID> &ids) {
        std::unordered_map<UUIDv4::UUID, std::reference_wrapper<const std::vector<uint8_t>>> data;

        for (const auto &id : ids) {
            if (!obd2.request_registered(id)) {
                obd2.register_request(get_request(id));
            }

            data[id] = obd2.get_request_raw(id);
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

    void to_json(nlohmann::json& j, const std::unordered_map<UUIDv4::UUID, float>& d) {
        j = nlohmann::json();

        for (const auto &pair : d) {
            j[pair.first.str()] = pair.second;
        }
    }
}
