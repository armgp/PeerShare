#include "client_functions.h"

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
void server(int port){
    std::cout << "[Peer Server]:    PORT=> " << port << "\n";
    struct Server server = serverConstructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 20);

    struct sockaddr *address = (struct sockaddr *)&server.address;
    socklen_t addressLen = (socklen_t)sizeof(server.address);
    while (true)
    {
        int newSocketFd = accept(server.socket, address, &addressLen);
        char req[1000];
        std::memset(req, 0, 1000);
        read(newSocketFd, req, 1000);
        string request(req);

        vector<string> commands = divideStringByChar(request, ' ');
        if (commands[0] == "getbitmap")
        {
            string fileName = commands[1];
            string filePath = fileLocMap[fileName];

            pair<vector<bool>, int> bitMap = fileToBitMap[fileName];
            string stringBitmap = convertBitMapToString(bitMap);

            std::cout << "<BITMAP SEND>\n";
            send(newSocketFd, stringBitmap.c_str(), stringBitmap.size(), 0);
        }

        if (commands[0] == "download")
        {
            string fileName = commands[1];
            int positionOfChunk = stoi(commands[2]);
            string fileLoc = fileLocMap[fileName];
            int totalNoOfChunks = fileToBitMap[fileName].first.size();
            int chunkSize = 524288;
            if (positionOfChunk == totalNoOfChunks - 1)
            {
                chunkSize = fileToBitMap[fileName].second;
            }

            ifstream file;
            file.open(fileLoc, ios::binary);
            long offset = positionOfChunk * 524288;
            file.seekg(offset, file.beg);
            char buffer[chunkSize];
            file.read(buffer, chunkSize);
            file.close();

            send(newSocketFd, buffer, chunkSize, 0);
            std::memset(buffer, 0, chunkSize);
            std::cout << "<CHUNK " << positionOfChunk << " SEND>\n";
        }

        else
        {
            printf("INVALID COMMAND: %s\n", req);
        }
        close(newSocketFd);
    }
}
