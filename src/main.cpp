#include <iostream>

#include "server/server.h"

int main() {
    obd2_server::server s;
    s.start_server();
}
