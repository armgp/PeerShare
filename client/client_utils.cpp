#include "client_functions.h"

char *request(struct Client *client, string serverIp, int port, string req, int resSize, int flag){

    char *res = (char *)malloc(resSize);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = client->domain;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    inet_pton(client->domain, serverIp.c_str(), &serverAddress.sin_addr);
    if (connect(client->socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        std::cout << "!! ERROR - CANNOT CONNECT SOCKET TO HOST !!\n";
        return res;
    }

    if (send(client->socket, req.c_str(), req.size(), 0) == -1){
        std::cout << "!! ERROR - SENDING FROM REQ BUFFER TO CLIENT SOCKET FAILED !!\n";
        return res;
    }

    if (flag == 0){
        if (read(client->socket, res, resSize) == -1){
            std::cout << "!! ERROR - READING FROM CLIENT SOCKET FILE DESCRIPTOR !!\n";
            return res;
        }

        return res;
    }

    // flag == 1 (compulsorily return resSize buffer)
    int totalSize = 0;
    char *res2 = (char *)malloc(resSize);
    int ind = 0;
    while (totalSize != resSize)
    {
        int currReadSize = read(client->socket, res, resSize);
        totalSize += currReadSize;
        for (int i = 0; (ind < ind + currReadSize && i < currReadSize); i++, ind++)
        {
            res2[ind] = res[i];
        }
        std::memset(res, 0, resSize);
    }
    return res2;
}

struct Client clientConstructor(int domain, int type, int protocol, int port, u_long interface)
{
    struct Client client;
    client.domain = domain;
    client.port = port;
    client.interface = interface;

    client.socket = socket(domain, type, protocol);
    client.request = request;
    return client;
}

void getUsersBitMapThread(string peerIp, int peerPort, string fileName, string peerId, unordered_map<string, string> &userToBitMap, sem_t &mutex)
{
    struct Client client2 = clientConstructor(AF_INET, SOCK_STREAM, 0, peerPort, INADDR_ANY);
    if (client2.socket == -1)
    {
        std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
        return;
    }
    string getFileBitMapReq = "getbitmap ";

    getFileBitMapReq += fileName;
    char *res2 = client2.request(&client2, peerIp, peerPort, getFileBitMapReq, 20000, 0);
    string stringBitMap(res2);

    sem_wait(&mutex);
    userToBitMap[peerId] = stringBitMap;
    sem_post(&mutex);

    std::cout << "<BITMAP RECEIVED FROM: >" << peerId << "\n";
    close(client2.socket);
}

void downloadChunkFromPeer(string fileName, string peerIp, int peerPort, int chunkNo, int currChunkSize, int fd)
{

    string downloadFileReq = "download ";
    downloadFileReq += fileName;
    downloadFileReq += " ";
    downloadFileReq += to_string(chunkNo);
    struct Client client3 = clientConstructor(AF_INET, SOCK_STREAM, 0, peerPort, INADDR_ANY);
    if (client3.socket == -1)
    {
        std::cout << "!! ERROR - SOCKET CREATION FAILED HERE@!!\n";
        return;
    }

    string status = "incomplete";
    int count = 0;
    while (status == "incomplete")
    {
        char *res3 = client3.request(&client3, peerIp, peerPort, downloadFileReq, currChunkSize, 1);
        string response3(res3);

        if (response3 == "<CHUNK DOWNLOAD FAILED>")
        {
            std::cout << "<FAILED TO DOWNLOAD> CHUNK - " << chunkNo << "\n";
            return;
        }

        string currChunkSha1 = getShaOfString(response3);
        string shaReceived = dynamicDownloadsShaMap[fileName].substr(chunkNo * 40, 40);

        if (currChunkSha1 == shaReceived)
        {
            pwrite(fd, res3, currChunkSize, chunkNo * 1024 * 512);
            status = "completed";
        }
        else
            count++;

        if (count >= 7)
        {
            status = "interrupted";
        }

        memset(res3, 0, currChunkSize);
    }
    close(client3.socket);

    if (status != "completed")
        std::cout << "<DOWNLOADED CHUNK NO: " << chunkNo << " INCOMPLETE>\n";
    fileToBitMap[fileName].first[chunkNo] = true;
}

void client(string req, string ip, int port){

    vector<string> command = divideStringByChar(req, ' ');

    // create_user <user_id> <password>
    if (command[0] == "create_user")
    {
        if (command.size() != 3)
        {
            std::cout << "Invalid number of arguments. Try => create_user <user_id> <password>\n";
            return;
        }
        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }

        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        std::cout << "**********[" << res << "]**********\n";
        string response(res);
        if (response == "<CREATED USER>")
        {
            if (createUserDirectory(command[1]) == 1)
            {
                std::cout << "**********[<ERROR WHILE CREATING CLIENT DIRECTORY>]**********\n";
            }
        }
        close(client.socket);
    }

    // login <user_id> <password>
    else if (command[0] == "login")
    {
        if (command.size() != 3)
        {
            std::cout << "Invalid number of arguments. Try => login <user_id> <password>\n";
            return;
        }

        req += " ";
        req += to_string(port);
        req += " ";
        req += ip;

        if (userid.size() == 0)
        {
            struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
            if (client.socket == -1)
            {
                std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
                return;
            }
            char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
            std::cout << "**********[" << res << "]**********\n";
            string response(res);
            if (response == "<LOGGED IN>")
            {
                userid = command[1];
            }
            close(client.socket);
        }
        else
        {
            std::cout << "**********[<ALREADY LOGGED IN AS>: " << userid << "]**********\n";
        }
    }

    // logout
    else if (command[0] == "logout")
    {

        if (command.size() != 1)
        {
            std::cout << "Invalid number of arguments. Try => logout\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND>]**********\n";
        }
        else
        {
            string ui = userid;
            struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
            if (client.socket == -1)
            {
                std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
                return;
            }
            req += " ";
            req += userid;
            char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);

            string response(res);
            if (response == "<LOGGED OUT>")
            {
                userid = "";
                std::cout << "**********[<" << ui << " LOGGED OUT>]**********\n";
            }
            else
            {
                std::cout << "**********[" << response << "]**********\n";
            }
            close(client.socket);
        }
    }

    // create_group <group_id>
    else if (command[0] == "create_group")
    {
        if (command.size() != 2)
        {
            std::cout << "Invalid number of arguments. Try => create_group <group_id>\n";
            return;
        }

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }

        if (userid.size())
        {
            req += " ";
            req += userid;
        }

        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        if (response == "<CREATED GROUP>")
        {
            std::cout << "**********[<GROUP " << command[1] << " CREATED>]**********\n";
        }
        else
        {
            std::cout << "**********[" << response << "]**********\n";
        }
        close(client.socket);
    }

    // list_groups
    else if (command[0] == "list_groups")
    {
        if (command.size() != 1)
        {
            std::cout << "Invalid number of arguments. Try => list_groups\n";
            return;
        }

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);

        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // join_group <group_id>
    else if (command[0] == "join_group")
    {
        if (command.size() != 2)
        {
            std::cout << "Invalid number of arguments. Try => join_group <group_id>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // list_requests <group_id>
    else if (command[0] == "list_requests")
    {
        if (command.size() != 2)
        {
            std::cout << "Invalid number of arguments. Try => list_requests <group_id>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // accept_request <group_id> <user_id>
    else if (command[0] == "accept_request")
    {
        if (command.size() != 3)
        {
            std::cout << "Invalid number of arguments. Try => accept_request <group_id> <user_id>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // leave_group <group_id>
    else if (command[0] == "leave_group")
    {
        if (command.size() != 2)
        {
            std::cout << "Invalid number of arguments. Try => leave_group <group_id>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // upload_file <file_path> <group_id>
    else if (command[0] == "upload_file")
    {
        if (command.size() != 3)
        {
            std::cout << "Invalid number of arguments. Try => upload_file <file_path> <group_id>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        char buf[PATH_MAX];
        char *result = realpath(command[1].c_str(), buf);
        if (!result)
        {
            std::cout << "**********[<FILE NOT FOUND>]**********\n";
            return;
        }
        string buffer(buf);
        string fileName = getFileName(buffer);
        string sha1 = getChunkWiseSha(buffer);
        int noOfChunks = (int)(sha1.size()) / 40;

        req = "";
        req += command[0];
        req += " ";
        req += buffer;
        req += " ";
        req += command[2];
        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        if (response == "<SHA1 ALREADY PRESENT>" || response == "<WAITING FOR SHA1 UPLOADS>")
        {
            std::cout << "**********[<UPLOADED>]**********\n";
        }

        if (response == "<WAITING FOR SHA1 UPLOADS>")
        {
            for (int i = 0; i < noOfChunks; i++)
            {
                string chunkSha = sha1.substr(i * 40, 40);
                string req1 = "uploadChunkSha ";
                req1 += fileName;
                req1 += " ";
                req1 += to_string(i);
                req1 += " ";
                req1 += to_string(noOfChunks);
                req1 += " ";
                req1 += chunkSha;
                struct Client client1 = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
                if (client1.socket == -1)
                {
                    std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
                    return;
                }
                char *res1 = client1.request(&client1, tracker.ip, tracker.port, req1, 20000, 0);
                string response1(res1);
                response = response1;
                close(client1.socket);
            }
        }

        if (response == "<SHA1 UPLOAD COMPLETED>" || response == "<SHA1 ALREADY PRESENT>")
        {
            cout << response << "\n";
            string fileName = getFileName(buffer);

            FILE *pFile;
            pFile = fopen(buf, "r");

            int sz = 0;
            if (pFile == NULL)
                perror("Error opening file");
            else
            {
                fseek(pFile, 0L, SEEK_END);
                sz = ftell(pFile);
            }
            fclose(pFile);

            int chunkSize = 512 * 1024;
            int noOfChunks = sz / chunkSize;
            int bytesLeft = sz - noOfChunks * chunkSize;
            // vector<pair<bool,int>> bitmap(noOfChunks, make_pair(1, chunkSize));

            int lastChunkSize = 512 * 1024;
            if (bytesLeft > 0)
            {
                lastChunkSize = bytesLeft;
                noOfChunks++;
            }
            vector<bool> bitMap(noOfChunks, true);
            fileToBitMap[fileName] = make_pair(bitMap, lastChunkSize);

            fileLocMap[fileName] = buffer;
        }

        close(client.socket);
    }

    // list_files <group_id>
    else if (command[0] == "list_files")
    {
        if (command.size() != 2)
        {
            std::cout << "Invalid number of arguments. Try => list_files <group_id>\n";
            return;
        }

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    // download_file <group_id> <file_name> <destination_path>
    else if (command[0] == "download_file")
    {
        if (command.size() != 4)
        {
            std::cout << "Invalid number of arguments. Try => download_file <group_id> <file_name> <destination_path>\n";
            return;
        }

        if (userid.size() == 0)
        {
            std::cout << "**********[<NO USER FOUND - LOGIN TO CONTINUE>]**********\n";
            return;
        }

        char buf[PATH_MAX];
        std::memset(buf, 0, PATH_MAX);
        char *result = realpath(command[3].c_str(), buf);

        if (!result)
        {
            std::cout << "**********[<DESTINATION NOT FOUND>]**********\n";
            return;
        }

        req += " ";
        req += userid;

        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }

        char *res = client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
        string response(res);
        std::memset(res, 0, 20000);

        if (response == "<ERROR1>")
        {
            std::cout << "**********[<GROUP DOESN'T EXIST>]**********\n";
        }
        else if (response == "<ERROR2>")
        {
            std::cout << "**********[<FILE DOESN'T EXIST IN THE GROUP>]**********\n";
        }
        else if (response == "<ERROR3>")
        {
            std::cout << "**********[<USER NOT A MEMBER OF THE GROUP>]**********\n";
        }
        else{

            std::cout << "**********[<USERS WITH FILE>: " << response << "]**********\n";

            vector<string> usersInfo = divideStringByChar(response, ' ');
            unordered_map<string, pair<string, int>> userDetails;
            for (string userInfo : usersInfo){
                vector<string> tokens = divideStringByChar(userInfo, '-');
                int port = atoi(tokens[2].substr(0, 4).c_str());
                userDetails[tokens[0]] = make_pair(tokens[1], port);
            }

            unordered_map<string, string> userToBitMap;
            string fileName = command[2];
            string destinationPath = command[3];
            vector<thread> bitMapThreads;

            // threads to get all the bitmaps from seeders
            sem_t mutex;
            sem_init(&mutex, 0, 1);
            for (auto us : userDetails){
                string peerId = us.first;
                string peerIp = us.second.first;
                int peerPort = us.second.second;
                bitMapThreads.push_back(thread(getUsersBitMapThread, peerIp, peerPort, fileName, peerId, ref(userToBitMap), ref(mutex)));
            }

            for (thread &t : bitMapThreads){
                t.join();
            }
            sem_destroy(&mutex);

            bitMapThreads.clear();
            std::cout << "<ALL BITMAPS RECEIVED>\n";

            unordered_map<string, pair<string, int>> userToStringBitMapAndLchunkSize;
            string bitMap = "";
            int lastChunkVal = 0;
            for (auto ub : userToBitMap){
                vector<string> vals = divideStringByChar(ub.second, ' ');
                userToStringBitMapAndLchunkSize[ub.first] = make_pair(vals[0], atoi(vals[1].c_str()));
                if (bitMap == "")
                    bitMap = vals[0];
                if (lastChunkVal == 0)
                    lastChunkVal = atoi(vals[1].c_str());
            }

            // get whole sha1 of file
            int noOfChunks = bitMap.size();
            string getShaReq = "getSha1 ";
            getShaReq += fileName;
            int noOfBytes = noOfChunks * 40;

            struct Client clientsh = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
            if (clientsh.socket == -1){
                std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
                return;
            }
            char *entireSha1 = clientsh.request(&clientsh, tracker.ip, tracker.port, getShaReq, noOfBytes, 1);
            string fullsha1(entireSha1);
            std::memset(entireSha1, 0, noOfBytes);
            dynamicDownloadsShaMap[fileName] = fullsha1;
            close(clientsh.socket);

            cout << dynamicDownloadsShaMap[fileName].size() << "\n";

            // chunkNo, noOfSeeders

            vector<pair<int, int>> chunkNoToNoOfSeeders(noOfChunks, make_pair(0, 0));
            unordered_map<int, vector<string>> chunkNoToSeeders;
            int firstChunkNo = -1;
            for (int i = 0; i < noOfChunks; i++)
            {
                chunkNoToNoOfSeeders[i].first = i;
                for (auto u : userToStringBitMapAndLchunkSize)
                {
                    chunkNoToSeeders[i].push_back(u.first);
                    string bm = u.second.first;
                    if (bm[i] == '1')
                    {
                        chunkNoToNoOfSeeders[i].second++;
                        if (firstChunkNo == -1)
                            firstChunkNo = i;
                        int udn = userDetails.size();
                        if (chunkNoToNoOfSeeders[i].second == udn)
                            firstChunkNo = i;
                    }
                }
            }
            int m = chunkNoToNoOfSeeders.size();
            pair<int, int> lastEle = chunkNoToNoOfSeeders[m - 1];
            pair<int, int> randomFirst = chunkNoToNoOfSeeders[firstChunkNo];
            chunkNoToNoOfSeeders[firstChunkNo] = lastEle;
            chunkNoToNoOfSeeders[m - 1] = randomFirst;
            chunkNoToNoOfSeeders.pop_back();

            std::sort(chunkNoToNoOfSeeders.begin(), chunkNoToNoOfSeeders.end(), [](pair<int, int> &a, pair<int, int> &b)
                      { return a.second > b.second; });
            // prioritizing random chunk first
            chunkNoToNoOfSeeders.push_back(randomFirst);

            destinationPath += "/";
            destinationPath += fileName;
            int fd = open(destinationPath.c_str(), O_CREAT | O_RDWR, 0666);

            string fileGrpKey = "";
            fileGrpKey += fileName;
            fileGrpKey += "*";
            fileGrpKey += command[1];

            // random chunk first and then rarest chunk first
            vector<thread> downloadChunkThreads;
            for (int i = m - 1; i >= 0; i--)
            {
                // setup file details for the first chunk download
                if (downloadedFiles.find(fileGrpKey) == downloadedFiles.end())
                {
                    downloadedFiles[fileGrpKey].first = command[1];
                    downloadedFiles[fileGrpKey].second = 'D';
                    char buf[PATH_MAX];
                    realpath(destinationPath.c_str(), buf);
                    fileLocMap[fileName] = string(buf);
                    vector<bool> bmap(noOfChunks, false);
                    fileToBitMap[fileName] = make_pair(bmap, lastChunkVal);
                }

                if (downloadedFiles[fileGrpKey].second == 'C')
                    break;

                pair<int, int> p = chunkNoToNoOfSeeders[i];
                int chunkNo = p.first;

                vector<string> seeders = chunkNoToSeeders[chunkNo];
                int max = seeders.size() - 1;
                int min = 0;
                int randNum = rand() % (max - min + 1) + min;
                string uid = seeders[randNum];
                string seederIp = userDetails[uid].first;
                int seederPort = userDetails[uid].second;
                int chunkSize = 512 * 1024;

                if (chunkNo == noOfChunks - 1)
                    chunkSize = lastChunkVal;

                downloadChunkThreads.emplace_back(downloadChunkFromPeer, fileName, seederIp, seederPort, chunkNo, chunkSize, fd);
                // downloadChunkFromPeer(fileName, seederIp, seederPort, chunkNo, chunkSize, fd);
                if (downloadChunkThreads.size() == 10)
                {
                    for (int i = 0; i < 10; i++)
                        downloadChunkThreads[i].join();
                    downloadChunkThreads.clear();
                }

                if (i == m - 1)
                {
                    // download_file <group_id> <file_name> <destination_path>
                    string req1 = "addSeeder ";
                    req1 += command[1];
                    req1 += " ";
                    req1 += command[2];
                    req1 += " ";
                    req1 += userid;
                    struct Client client1 = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
                    if (client1.socket == -1)
                    {
                        std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
                        return;
                    }

                    char *res1 = client1.request(&client1, tracker.ip, tracker.port, req1, 20000, 0);
                    string response1(res1);
                    std::memset(res1, 0, 20000);
                    close(client1.socket);
                }

                // usleep(1000000);
            }

            for (thread &t : downloadChunkThreads)
            {
                t.join();
            }
            close(fd);
            downloadChunkThreads.clear();

            std::cout << "FILE COMPLETELY DOWNLOADED SUCCESSFULLY\n";
            downloadedFiles[fileGrpKey].second = 'C';
            dynamicDownloadsShaMap.erase(dynamicDownloadsShaMap.find(fileName));
        }
        close(client.socket);
    }

    // show_downloads
    else if (command[0] == "show_downloads")
    {
        //[D] [grp_id] filename
        if (downloadedFiles.size() == 0)
        {
            cout << "<NO DOWNLOADS YET>\n";
        }
        else
        {
            for (auto m : downloadedFiles)
            {
                std::cout << "[" << m.second.second << "][" << m.second.first << "] " << divideStringByChar(m.first, '*')[0] << "\n";
            }
        }
    }

    // stop_share <group_id> <file_name>
    else if (command[0] == "stop_share")
    {
        string req1 = req + " ";
        req1 += userid;
        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        char *res = client.request(&client, tracker.ip, tracker.port, req1, 20000, 0);
        string response(res);
        std::cout << "**********[" << response << "]**********\n";
        close(client.socket);
    }

    else
    {
        struct Client client = clientConstructor(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);
        if (client.socket == -1)
        {
            std::cout << "!! ERROR - SOCKET CREATION FAILED !!\n";
            return;
        }
        client.request(&client, ip, client.port, req, 20000, 0);
        close(client.socket);
    }
}
