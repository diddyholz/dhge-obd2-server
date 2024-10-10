#include "csv_logger.h"

#include <chrono>
#include <fstream>

namespace obd2_server {
    csv_logger::csv_logger() {}

    csv_logger::csv_logger(const std::vector<std::string> &header, bool use_bytes) : 
        csv_logger(
            header, 
            "obd2_log_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + ".csv", 
            use_bytes
        ) {}

    csv_logger::csv_logger(const std::vector<std::string> &header, const std::string &filename, bool use_bytes) 
        : start_time(std::chrono::system_clock::now()) {
        file.open(filename);

        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file " + filename);
        }
        
        write_header(header, use_bytes);
    }

    void csv_logger::write_row(const std::vector<float> &data) {
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        file << get_time_string(timestamp);

        for (float d : data) {
            file  << "," << d;
        }

        file << std::endl;
    }

    void csv_logger::write_row_raw(const std::vector<std::vector<uint8_t>> &data) {
        auto time_since_start = std::chrono::system_clock::now() - start_time;
        uint32_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_start).count();

        // Print miliseconds since start as bytes
        file << std::hex << std::setw(2) << std::setfill('0') 
            << (timestamp & 0xFF)
            << ((timestamp >> 8) & 0xFF)
            << ((timestamp >> 16) & 0xFF)
            << ((timestamp >> 24) & 0xFF);

        for (const auto &col : data) {
            for (uint8_t byte : col) {
                file << "," << static_cast<int>(byte);
            }
        }

        file << std::endl;
    }

    bool csv_logger::get_is_active() const {
        return file.is_open();
    }

    void csv_logger::write_header(const std::vector<std::string> &header, bool use_bytes) {
        const size_t header_count = header.size();

        if (use_bytes) {
            file << "00:00:00"; // See simulator configuration
        } else {
            file << "timestamp";
        }

        for (const auto &col : header) {
            file << "," << col;
        }

        file << std::endl;
    }
    
    std::string csv_logger::get_time_string(uint64_t timestamp) const {
        std::time_t time = timestamp / 1000;
        std::tm *tm = std::localtime(&time);

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", tm);
        std::string time_string = buffer;

        // Add milliseconds
        time_string += "." + std::to_string(timestamp % 1000);

        return time_string;
    }
}
