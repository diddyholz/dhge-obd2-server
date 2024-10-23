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
            "/dashboards/:id",
            std::bind(
                &server::handle_get_dashboard_by_id,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        server_instance.Post(
            "/dashboards",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_post_dashboard,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        );

        server_instance.Put(
            "/dashboards/:id",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_put_dashboard,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        );

        server_instance.Delete(
            "/dashboards/:id",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_delete_dashboard,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
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

        server_instance.Delete(
            "/dtcs",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_delete_dtcs,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
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

        server_instance.Get(
            "/config",
            std::bind(
                &server::handle_get_config,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        server_instance.Put(
            "/config",
            httplib::Server::Handler(
                std::bind(
                    &server::handle_put_config,
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
            const std::vector<UUIDv4::UUID> &supported_requests = obd2->supported_requests(vehicle.second.get_requests());
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
            nlohmann::json dashboard_j;

            dashboard_j["id"] = dashboard.first;
            dashboard_j["name"] = dashboard.second.get_name();

            j.push_back(dashboard_j);
        }

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_get_dashboard_by_id(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json res_body;

        auto it_path = req.path_params.find("id");

        if (it_path == req.path_params.end()) {
            res_body["error"] = "Missing parameter 'id'";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        // Find specified dashboard
        UUIDv4::UUID id = UUIDv4::UUID::fromStrFactory(it_path->second);
        auto it_dashboard = dashboards.find(id);

        if (it_dashboard == dashboards.end()) {
            res_body["error"] = "Dashboard not found";
            res.status = 404;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        res_body = it_dashboard->second;
        res.set_content(res_body.dump(), "application/json");
    }

    void server::handle_post_dashboard(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json res_body;

        if (req.body.empty()) {
            res_body["error"] = "Missing request body";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        nlohmann::json dashboard_body = nlohmann::json::parse(req.body);
        auto body_it = dashboard_body.find("name");

        if (body_it == dashboard_body.end()) {
            res_body["error"] = "Missing parameter 'name'";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        const std::string name = body_it->get<std::string>();
        const std::string path = expand_path(dashboards_dir) + "/" + name + ".json";

        // Create and save new dashboard with specified name
        dashboard d(name, path);
        UUIDv4::UUID id = d.get_id();

        try {
            d.save();
        }
        catch (const std::exception &e) {
            res_body["error"] = e.what();
            res.status = 500;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        dashboards.try_emplace(id, std::move(d));

        res_body["id"] = id;
        res.set_content(res_body.dump(), "application/json");
    }

    void server::handle_put_dashboard(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json res_body;

        if (req.body.empty()) {
            res_body["error"] = "Missing request body";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        UUIDv4::UUID id;
        nlohmann::json update_body = nlohmann::json::parse(req.body);
        auto it_id = req.path_params.find("id");

        if (it_id == req.path_params.end()) {
            res_body["error"] = "Missing parameter 'id'";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        // Check if dashboard exists
        id = UUIDv4::UUID::fromStrFactory(it_id->second);
        auto it_dashboard = dashboards.find(id);

        if (it_dashboard == dashboards.end()) {
            res_body["error"] = "Dashboard not found";
            res.status = 404;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        dashboard &sel_dashboard = it_dashboard->second;

        try {
            sel_dashboard.update(update_body);
            sel_dashboard.save();
        }
        catch (const std::exception &e) {
            res_body["error"] = e.what();
            res.status = 500;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        res_body = sel_dashboard;
        res.set_content(res_body.dump(), "application/json");
    }

    void server::handle_delete_dashboard(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json res_body;
        UUIDv4::UUID id;

        // Get ID from path
        auto it_id = req.path_params.find("id");

        if (it_id == req.path_params.end()) {
            res_body["error"] = "Missing parameter 'id'";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        // Check if dashboard exists
        id = UUIDv4::UUID::fromStrFactory(it_id->second);
        auto it_dashboard = dashboards.find(id);

        if (it_dashboard == dashboards.end()) {
            res_body["error"] = "Dashboard not found";
            res.status = 404;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        // Delete dashboard
        dashboard &sel_dashboard = it_dashboard->second;

        sel_dashboard.delete_file();
        dashboards.erase(id);

        res.status = 204;
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
        auto dtcs = obd2->get_dtcs();   
        nlohmann::json j = dtcs;

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_delete_dtcs(const httplib::Request &req, httplib::Response &res) {
        obd2->clear_dtcs();
        res.status = 204;
    }

    void server::handle_get_log(const httplib::Request &req, httplib::Response &res) {
        std::string name = req.get_param_value("name");
        nlohmann::json j;

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
        nlohmann::json res_body;

        if (req.body.empty()) {
            res_body["error"] = "Missing request body";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        nlohmann::json req_body = nlohmann::json::parse(req.body);
        bool log_raw = false;
        auto body_it = req_body.find("stop");

        if (body_it != req_body.end()) {
            std::string name = body_it->template get<std::string>();
            stop_log(name);

            res.status = 204;
            return;
        }

        // Get dashboard to log
        body_it = req_body.find("dashboard");

        if (body_it == req_body.end()) {
            res_body["error"] = "Missing parameter 'dashboard'";
            res.status = 400;
            return res.set_content(res_body.dump(), "application/json");
        }

        UUIDv4::UUID dashboard_id = body_it->template get<UUIDv4::UUID>();

        // Check if dashboard exists
        auto it = dashboards.find(dashboard_id);

        if (it == dashboards.end()) {
            res_body["error"] = "Dashboard not found";
            res.status = 404;
            return res.set_content(res_body.dump(), "application/json");
        }

        // Check if should log raw
        body_it = req_body.find("raw");

        if (body_it != req_body.end()) {
            log_raw = body_it->get<bool>();
        }

        std::string log_name;
    
        try {
            log_name = create_log(dashboard_id, log_raw);
        }
        catch(const std::exception &e) {
            res_body["error"] = e.what();
            res.status = 500;
            return res.set_content(res_body.dump(), "application/json");
        }

        res_body["name"] = log_name;
        res.set_content(res_body.dump(), "application/json");
    }

    void server::handle_get_config(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json j;

        j["obd2_refresh_ms"] = get_obd2_refresh_ms();

        res.set_content(j.dump(), "application/json");
    }

    void server::handle_put_config(const httplib::Request &req, httplib::Response &res) {
        nlohmann::json res_body;

        if (req.body.empty()) {
            res_body["error"] = "Missing request body";
            res.status = 400;
            res.set_content(res_body.dump(), "application/json");
            return;
        }

        nlohmann::json config_body = nlohmann::json::parse(req.body);
        auto it = config_body.find("obd2_refresh_ms");

        if (it != config_body.end()) {
            set_obd2_refresh_ms(it->template get<uint32_t>());
        }

        save_server_config();

        res_body["obd2_refresh_ms"] = get_obd2_refresh_ms();
        res.set_content(res_body.dump(), "application/json");
    }

    std::string server::create_log(const UUIDv4::UUID &dashboard_id, bool log_raw) {
        auto it = dashboards.find(dashboard_id);

        if (it == dashboards.end()) {
            throw std::runtime_error("Dashboard not found [" + dashboard_id.str() + "]");
        }

        const dashboard &d = it->second;
        std::unordered_map<UUIDv4::UUID, std::string> requests;

        // Create map of request IDs to names (for log header)
        for (const request_entry &entry : d.get_requests()) {
            const auto &req = get_request(entry.req_id);
        
            // Use special format for raw logs (ECU:Service:PID)
            if (log_raw) {
                std::stringstream ss;

                ss << std::hex << std::setfill('0')
                   << std::setw(3) << req.ecu << ":" 
                   << std::setw(2) << static_cast<uint16_t>(req.service) << ":" 
                   << std::setw(2) << req.pid; 

                requests[entry.req_id] = ss.str();
            }
            else {
                requests[entry.req_id] = req.name;
            }
        }

        data_log log(requests, expand_path(logs_dir), log_raw);
        std::string log_name = log.get_name();
        logs.try_emplace(log_name, std::move(log));

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
            if (!obd2->request_registered(id)) {
                obd2->register_request(get_request(id));
            }

            data[id] = obd2->get_request_val(id);
        }

        return data;
    }

    std::unordered_map<UUIDv4::UUID, std::vector<uint8_t>> server::get_raw_data_for_ids(const std::vector<UUIDv4::UUID> &ids) {
        std::unordered_map<UUIDv4::UUID, std::vector<uint8_t>> data;

        for (const auto &id : ids) {
            if (!obd2->request_registered(id)) {
                obd2->register_request(get_request(id));
            }

            data.try_emplace(id, obd2->get_request_raw(id));
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
