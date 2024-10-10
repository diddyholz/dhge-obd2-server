#pragma once

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

namespace obd2_server {
    class csv_logger {
        public:
            csv_logger();
            csv_logger(const std::vector<std::string> &header, bool use_bytes = false);
            csv_logger(const std::vector<std::string> &header, const std::string &filename,
                bool use_bytes = false);

            void write_row(const std::vector<float> &data);
            void write_row_raw(const std::vector<std::vector<uint8_t>> &data);
            bool get_is_active() const;
            
        private:
            std::ofstream file;
            std::chrono::time_point<std::chrono::system_clock> start_time;

            void write_header(const std::vector<std::string> &header, bool use_bytes);
            std::string get_time_string(uint64_t timestamp) const;
    };
}
