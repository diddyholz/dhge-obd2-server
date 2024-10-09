#include <iostream>
#include <thread>

#include "server/server.h"

int main() {
    try {
        obd2_server::server s;
        s.start_server();
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
