#include "data_log.h"

#include <fstream>
#include <filesystem>

namespace obd2_server {
    data_log::data_log() { }

    data_log::data_log(const std::string &name, const std::string &directory) 
        : name(name), directory(directory) {
        // Strip raw prefix from name 
        if (name.find_first_of(RAW_LOG_PREFIX) == 0) {
            raw_log = true;
            this->name = name.substr(RAW_LOG_PREFIX.size());
        }

        file_size = read_file_size();
    }

    data_log::data_log(const std::unordered_map<UUIDv4::UUID, std::string> &requests, 
        const std::string &directory, bool raw_log) 
        : directory(directory), raw_log(raw_log) {
        std::vector<std::string> header(requests.size());
        std::string filename;
        size_t i = 0;

        for (const auto &req : requests) {
            this->requests.push_back(req.first);
            header[i] = req.second;
            col_indices[req.first] = i;

            i++;
        }

        name = generate_name();

        // Use prefix for raw logging
        if (raw_log) {
            filename = directory + "/raw_" + name + ".csv";
        }
        else {
            filename = directory + "/" + name + ".csv";
        }

        logger = csv_logger(header, filename, raw_log);
    }

    void data_log::stop_logging() {
        if (!logger.get_is_active()) {
            return;
        }
        
        // Clear active logger and update file size
        logger = csv_logger();
        file_size = read_file_size();
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

    void data_log::add_data_raw(const std::unordered_map<UUIDv4::UUID, std::vector<uint8_t>> &data) {
        if (!logger.get_is_active()) {
            return;
        }

        std::vector<std::vector<uint8_t>> row(requests.size());

        for (const auto &d : data) {
            UUIDv4::UUID req_id = d.first;
            auto col_it = col_indices.find(req_id);

            // If the request is not in the log, skip it
            if (col_it == col_indices.end()) {
                continue;
            }

            row[col_it->second] = d.second;
        }

        logger.write_row_raw(row);
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

    bool data_log::get_is_raw() const {
        return raw_log;
    }

    const std::vector<UUIDv4::UUID> &data_log::get_request_ids() const {
        return requests;
    }

    size_t data_log::read_file_size() const {
        std::string filename;

        if (raw_log) {
            filename = directory + "/raw_" + name + ".csv";
        }
        else {
            filename = directory + "/" + name + ".csv";
        }
        
        try {
            return std::filesystem::file_size(filename);
        }
        catch (const std::exception &e) {
            throw std::runtime_error(e.what());
        }
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
            {"raw_log", log.get_is_raw()},
            {"size", log.get_file_size()}
        };
    }
}
