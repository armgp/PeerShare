#include "tracker_functions.h"

struct Server serverConstructor(int domain, int type, int protocol, u_long interface, int port, int backlog){

    struct Server server;

    server.domain = domain;
    server.type = type;
    server.protocol = protocol;
    server.interface = interface;
    server.port = port;
    server.backlog = backlog;
    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain, type, protocol);

    if (server.socket < 0)
    {
        perror("Failed to connect to socket\n");
        exit(1);
    }

    if ((bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address))) < 0)
    {
        perror("Failed to bind socket\n");
        exit(1);
    }

    if ((listen(server.socket, server.backlog)) < 0)
    {
        perror("Failed to start listening\n");
        exit(1);
    }

    return server;
}

void server(int port, string ip){
    cout << "[Tracker Server]:    PORT=> " << port << "\n";
    struct Server server = serverConstructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 20);

    struct sockaddr *address;
    socklen_t addressLen;
    bool status = true;

    // take care of joining threads.
    vector<thread> clientReqThreads;

    while (status)
    {
        int newSocketFd = accept(server.socket, address, &addressLen);
        struct ThreadParams params;
        params.newSocketFd = newSocketFd;
        params.server = server;
        params.status = &status;
        clientReqThreads.emplace_back(processClientRequest, params);
    }

    for (thread &t : clientReqThreads)
    {
        if (t.joinable())
            t.join();
    }

    clientReqThreads.clear();
}
