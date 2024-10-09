#include "data_log.h"

#include <fstream>
#include <filesystem>

namespace obd2_server {
    data_log::data_log() { }

    data_log::data_log(const std::string &name, const std::string &directory) 
        : name(name), directory(directory) { 
        try {
            file_size = std::filesystem::file_size(directory + "/" + name + ".csv");
        } 
        catch (const std::exception &e) {
            throw std::runtime_error(e.what());
        }
    }

    data_log::data_log(const std::unordered_map<UUIDv4::UUID, std::string> &requests, 
        const std::string &directory) : directory(directory) {
        std::vector<std::string> header(requests.size());
        size_t i = 0;

        for (const auto &req : requests) {
            this->requests.push_back(req.first);
            header.push_back(req.second);
            col_indices[req.first] = i;

            i++;
        }

        name = generate_name();
        logger = csv_logger(header, directory + "/" + name + ".csv");
    }

    void data_log::add_data(const std::unordered_map<UUIDv4::UUID, float> &data) {
        if (!logger.get_is_active()) {
            return;
        }

        std::vector<float> row(requests.size());

        for (const auto &d : data) {
            UUIDv4::UUID req_id = d.first;
            auto col_it = col_indices.find(req_id);

            // If the request is not in the log, skip it
            if (col_it == col_indices.end()) {
                continue;
            }

            row[col_it->second] = d.second;
        }

        logger.write_row(row);
    }

    const std::string &data_log::get_name() const {
        return name;
    }

    const std::string &data_log::get_csv_string() {
        // When not currently logging, return the cached CSV string
        if (!logger.get_is_active() && !csv_string.empty()) {
            return csv_string;
        }
        
        std::ifstream csv_file(directory + "/" + name + ".csv");

        if (!csv_file.is_open()) {
            throw std::system_error(std::error_code(errno, std::generic_category()));
        }

        std::stringstream ss;
        ss << csv_file.rdbuf();

        csv_string = ss.str();

        return csv_string;
    }

    size_t data_log::get_file_size() const {
        return file_size;
    }

    bool data_log::get_is_logging() const {
        return logger.get_is_active();
    }

    const std::vector<UUIDv4::UUID> &data_log::get_request_ids() const {
        return requests;
    }

    std::string data_log::generate_name() const {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d%H%M%S");
        return oss.str();
    }

    void to_json(nlohmann::json &j, const data_log &log) {
        j = nlohmann::json{
            {"name", log.get_name()},
            {"is_logging", log.get_is_logging()},
            {"size", log.get_file_size()}
        };
    }
}
