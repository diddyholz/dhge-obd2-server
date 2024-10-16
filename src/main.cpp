#include <cstdlib>
#include <iostream>
#include <thread>

#include "server/server.h"

std::unique_ptr<obd2_server::server> active_server;

void handle_int(int signal);

int main() {
    std::signal(SIGINT, handle_int);

    try {
        active_server = std::make_unique<obd2_server::server>();
        active_server->start_server();
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void handle_int(int signal) {
    if (active_server) {
        active_server->stop_server();
    }    
}
