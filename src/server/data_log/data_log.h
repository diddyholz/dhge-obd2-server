#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <json.hpp>
#include <uuid_v4.h>

#include "csv_logger/csv_logger.h"

namespace obd2_server {
    class data_log {
        public:
            data_log();
            data_log(const std::string &name, const std::string &directory);
            data_log(const std::unordered_map<UUIDv4::UUID, std::string> &requests, 
                const std::string &directory,
                bool raw_log = false);

            void add_data(const std::unordered_map<UUIDv4::UUID, float> &data);
            void add_data_raw(const std::unordered_map<UUIDv4::UUID, std::vector<uint8_t>> &data);
            void stop_logging();

            const std::string &get_name() const;
            const std::string &get_csv_string();
            size_t get_file_size() const;
            bool get_is_logging() const;
            bool get_is_raw() const;
            const std::vector<UUIDv4::UUID> &get_request_ids() const;

        private:
            const std::string RAW_LOG_PREFIX = "raw_";

            std::vector<UUIDv4::UUID> requests;
            std::unordered_map<UUIDv4::UUID, size_t> col_indices;

            std::string name;
            std::string directory;
            std::string csv_string;
            size_t file_size = 0;
            csv_logger logger;

            bool raw_log = false;

            size_t read_file_size() const;
            std::string generate_name() const;
    };

    void to_json(nlohmann::json &j, const data_log &log);
}