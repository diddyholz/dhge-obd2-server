#include <iostream>
#include <thread>

#include "server/server.h"

int main() {
    obd2_server::server s;
    s.start_server();

    while (true)
    {
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
