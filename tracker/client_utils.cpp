#include "tracker_functions.h"

char* request(struct Client *client, string serverIp, int port, string req, int resSize, int flag){

        char* res = (char*)malloc(resSize);

        struct sockaddr_in serverAddress;
        serverAddress.sin_family = client->domain;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        inet_pton(client->domain, serverIp.c_str(), &serverAddress.sin_addr);
        if(connect(client->socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
            if(req[0] == 't' && req[1] == '_') std::cout<<"TRACKER2 NOT ONLINE\n";
            else std::cout<<"!! ERROR - CANNOT CONNECT SOCKET TO HOST !!\n";
            return res;
        }

        if(send(client->socket, req.c_str(), req.size(), 0) == -1){
            std::cout<<"!! ERROR - SENDING FROM REQ BUFFER TO CLIENT SOCKET FAILED !!\n";
            return res;
        }

        if(flag == 0){
            if(read(client->socket, res, resSize) == -1){
                std::cout<<"!! ERROR - READING FROM CLIENT SOCKET FILE DESCRIPTOR !!\n";
                return res;
            }

            return res;
        }

        //flag == 1 (compulsorily return resSize buffer)
        int totalSize = 0;
        char* res2 = (char*)malloc(resSize);
        int ind = 0;
        while(totalSize != resSize){
            int currReadSize = read(client->socket, res, resSize);
            totalSize+=currReadSize;
            for(int i=0; (ind<ind+currReadSize && i<currReadSize); i++,ind++){
                res2[ind] = res[i];
            }
            std::memset(res, 0, resSize);
        }
        return res2;
}

struct Client clientConstructor(int domain, int type, int protocol, int port, u_long interface){
    struct Client client;
    client.domain = domain;
    client.port = port;
    client.interface = interface;

    client.socket = socket(domain, type, protocol);
    client.request = request;
    return client;
}

void client(string req, string ip, int port) {
    struct Client client = clientConstructor(AF_INET,  SOCK_STREAM, 0, port, INADDR_ANY);
    if(client.socket == -1){
        cout<<"!! ERROR - SOCKET CREATION FAILED !!\n";
        return;
    }
    client.request(&client, tracker.ip, tracker.port, req, 20000, 0);
    close(client.socket);
}

void syncCommand(string request){
    string tRequest = "t_"+request;
    struct Client client = clientConstructor(AF_INET,  SOCK_STREAM, 0, tracker2.port, INADDR_ANY);
    if(client.socket == -1){
        std::cout<<"!! ERROR - SOCKET CREATION FAILED !!\n";
        return;
    }
    char* res = client.request(&client, tracker2.ip, tracker2.port, tRequest, 20000, 0);
    string response(res);
    cout<<response<<"\n";
    close(client.socket);
}

void processClientRequest(struct ThreadParams params){

        int newSocketFd = params.newSocketFd;
        struct Server server = params.server;
        bool* status = params.status;

        if(!(*status)) return;

        char req[1000];
        memset(req, 0, 1000);
        read(newSocketFd, req, 1000);
        string request(req);

        if(request == "quit"){
            close(server.socket);
            *status = false;
        }

        vector<string> command = divideStringByChar(request, ' ');

        // create_user <user_id> <password> 
        if(command[0] == "create_user"){

            if(command.size() != 3){
                cout<<"<INVALID NO OF ARGS>\n";
                send(newSocketFd, "<INVALID NO OF ARGS>", 21, 0);
            }

            else{
                string userId = command[1];
                string password = command[2];
     
                if(UsersMap.find(userId) == UsersMap.end()){
                    User user(userId, password);
                    UsersMap[userId] = user;
                    send(newSocketFd, "<CREATED USER>", 15, 0);
                    cout<<"<CREATED> User:"<<userId<<"\n";

                    if(trackerNo == 1){
                        syncCommand(request);
                    }
                }
                else{
                    send(newSocketFd, "<CREATE USER FAILED - USERID ALREADY EXISTS>", 45, 0);
                    cout<<"<CREATE USER FAILED>: User-Id already exists!\n";
                }
            }
        }

        // t_create_user <user_id> <password> 
        else if(command[0] == "t_create_user"){
            if(trackerNo == 2){
                string userId = command[1];
                string password = command[2];
                User user(userId, password);
                UsersMap[userId] = user;
                cout<<"<CREATED> User:"<<userId<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        // login <user_id> <password> <peer_port> <peer_ip>
        else if(command[0] == "login"){
            string userId = command[1];
            string password = command[2];
            string peerPort = command[3];
            string peerIp = command[4];

            if(UsersMap.find(userId) != UsersMap.end() && 
               UsersMap[userId].live == false && 
               UsersMap[userId].password == password){

                UsersMap[userId].live = true;
                UsersMap[userId].ip = peerIp;
                UsersMap[userId].port = stoi(peerPort);
                cout<<"<LOGGED IN AS>: "<<userId<<"\n";
                send(newSocketFd, "<LOGGED IN>", 12, 0);

                if(trackerNo == 1){
                    syncCommand(request);
                }
            }
            
            else if(UsersMap.find(userId) != UsersMap.end() && UsersMap[userId].live == true){
                cout<<"<USER ALREADY LOGGED ON ANOTHER SYSTEM>\n";
                send(newSocketFd, "<ALREADY LOGGED IN ANOTHER SYSTEM>", 35, 0);
            }

            else{
                cout<<"<INVALID CREDENTIALS>\n";
                send(newSocketFd, "<INVALID CREDENTIALS>", 22, 0);
            }
        }

        //t_login <user_id> <password> <peer_port> <peer_ip>
        else if(command[0] == "t_login"){
            if(trackerNo == 2){
                string userId = command[1];
                string password = command[2];
                string peerPort = command[3];
                string peerIp = command[4];
                UsersMap[userId].live = true;
                UsersMap[userId].ip = peerIp;
                UsersMap[userId].port = stoi(peerPort);
                cout<<"<LOGGED IN AS>: "<<userId<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //logout <user_id>
        else if(command[0] == "logout"){
            string userid = command[1];
            if(UsersMap.find(userid) == UsersMap.end()){
                cout<<"<USER NOT LOGGED IN>\n";
                send(newSocketFd, "<USER NOT LOGGED IN>", 21, 0);
            }
            
            else{
                UsersMap[userid].live = false;
                UsersMap[userid].ip = "";
                UsersMap[userid].port = 0;
                cout<<"<"<<userid<<" LOGGED OUT>\n";
                send(newSocketFd, "<LOGGED OUT>", 21, 0);
                if(trackerNo == 1){
                    syncCommand(request);
                }
            }
        }

        //t_logout <userid>
        else if(command[0] == "t_logout"){
            if(trackerNo == 2){
                string userid = command[1];
                UsersMap[userid].live = false;
                UsersMap[userid].ip = "";
                UsersMap[userid].port = 0;
                cout<<"<"<<userid<<" LOGGED OUT>\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //create_group <group_id> <user_id>
        else if(command[0] == "create_group"){
            if(command.size() != 3){
                cout<<"<LOGIN TO CREATE A GROUP>\n";
                send(newSocketFd, "<LOGIN TO CREATE A GROUP>", 26, 0);
            }else{
                string groupid = command[1];
                string userid = command[2];
                if(Groups.find(groupid) == Groups.end()){
                    Group newGroup;
                    newGroup.userIds.insert(userid);
                    newGroup.adminUserId = userid;
                    Groups[groupid] = newGroup;
                    send(newSocketFd, "<CREATED GROUP>", 16, 0);
                    cout<<"<CREATED> Group: "<<groupid<<" - ADMIN: "<<userid<<"\n";
                    UsersMap[userid].addToAdminedGroups(groupid);

                    if(trackerNo == 1){
                        syncCommand(request);
                    }
                }
                else{
                    send(newSocketFd, "<CREATE GROUP FAILED - GROUPID ALREADY EXISTS>", 47, 0);
                    cout<<"<CREATE GROUP FAILED>: Group-Id already exists!\n";
                }
            }
        }

        //t_create_group <group_id> <user_id>
        else if(command[0] == "t_create_group"){
            if(trackerNo == 2){
                string groupid = command[1];
                string userid = command[2];
                Group newGroup;
                newGroup.userIds.insert(userid);
                newGroup.adminUserId = userid;
                Groups[groupid] = newGroup;
                UsersMap[userid].addToAdminedGroups(groupid);
                cout<<"<CREATED> Group: "<<groupid<<" - ADMIN: "<<userid<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //list_groups
        else if(command[0] == "list_groups"){
            string response;
            if(Groups.size() == 0) response = "";
            else response = "\n\n";

            int i = 1;
            for(auto group : Groups){
                response+=to_string(i++);
                response+=". ";
                response+=group.first;
                response+="\n";
            }
            
            if(response.size() == 0){
                send(newSocketFd, "<NONE>", 18, 0);
                cout<<"<NO GROUPS EXIST>\n";
            }
            else {
                response+="\n";
                send(newSocketFd, response.c_str(), response.size(), 0);
            }
        }

        //join_group <group_id> <user_id>
        else if(command[0] == "join_group"){
            if(command.size() != 3){
                cout<<"<LOGIN TO CREATE A GROUP>\n";
                send(newSocketFd, "<LOGIN TO CREATE A GROUP>", 26, 0);
            }else{
                string groupId = command[1];
                string userId = command[2];
                if(Groups.find(groupId) == Groups.end()){
                    send(newSocketFd, "<GROUP DOESNT EXIST>", 21, 0);
                    cout<<"<ERROR> Group: "<<groupId<<" - DOESN'T EXIST\n";
                }

                else if(UsersMap.find(userId) == UsersMap.end()){
                    send(newSocketFd, "<USER DOESNT EXIST>", 20, 0);
                    cout<<"<ERROR> USER: "<<userId<<" - DOESN'T EXIST\n";
                }

                else if(Groups[groupId].userIds.find(userId) != Groups[groupId].userIds.end()){
                    send(newSocketFd, "<USER ALREADY A PART OF THE GROUP>", 35, 0);
                    cout<<"<USER ALREADY A PART OF THE GROUP>\n";
                }
                
                else{
                    auto pos = Groups[groupId].pendingRequests.find(userId);

                    if(pos != Groups[groupId].pendingRequests.end()){
                        send(newSocketFd, "<REQUEST ALREADY PENDING>", 26, 0);
                        cout<<"<REQUEST ALREADY PENDING>\n";
                    }

                    else {
                        Groups[groupId].pendingRequests.insert(userId);
                        send(newSocketFd, "<REQUEST SEND>", 15, 0);
                        cout<<"<REQUEST SENT>\n";

                        if(trackerNo == 1){
                            syncCommand(request);
                        }
                    }
                }
            }
        }

        //t_join_group <group_id> <user_id>
        else if(command[0] == "t_join_group"){
            if(trackerNo == 2){
                string groupId = command[1];
                string userId = command[2];
                Groups[groupId].pendingRequests.insert(userId);
                cout<<"<REQUEST SENT>\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //leave_group <group_id> <user_id>
        else if(command[0] == "leave_group"){
            if(command.size() != 3){
                cout<<"<YOU MUST BE LOGGED IN FOR EXECUTING THIS COMMAND>\n";
                send(newSocketFd, "<YOU MUST BE LOGGED IN FOR EXECUTING THIS COMMAND>", 51, 0);
            }else{
                string groupId = command[1];
                string userId = command[2];
                if(Groups.find(groupId) == Groups.end()){
                    send(newSocketFd, "<GROUP DOESNT EXIST>", 21, 0);
                    cout<<"<ERROR> Group: "<<groupId<<" - DOESN'T EXIST\n";
                }

                else if(UsersMap.find(userId) == UsersMap.end()){
                    send(newSocketFd, "<USER DOESNT EXIST>", 20, 0);
                    cout<<"<ERROR> USER: "<<userId<<" - DOESN'T EXIST\n";
                }
                
                else{
                    auto pos = Groups[groupId].userIds.find(userId);
                    if(pos == Groups[groupId].userIds.end()){
                        send(newSocketFd, "<USER IS NOT A PART OF THIS GROUP>", 35, 0);
                        cout<<"<ERROR> USER: "<<userId<<" - NOT IN THE GROUP\n";
                    }else{
                        if(Groups[groupId].adminUserId == userId){
                            send(newSocketFd, "<ADMIN CANNOT LEAVE>", 21, 0);
                            cout<<"<ERROR> ADMIN: "<<userId<<" - CANNOT LEAVE\n";
                        }
                        else{
                            Groups[groupId].userIds.erase(pos);
                            send(newSocketFd, "<LEFT GROUP>", 35, 0);
                            cout<<"<LEFT> USER: "<<userId<<" - LEFT THE GROUP\n";

                            if(trackerNo == 1){
                                syncCommand(request);
                            }
                        }
                        
                    }
                }
            }
        }

        //t_leave_group <group_id> <user_id>
        else if(command[0] == "t_leave_group"){
            if(trackerNo == 2){
                string groupId = command[1];
                string userId = command[2];
                auto pos = Groups[groupId].userIds.find(userId);
                Groups[groupId].userIds.erase(pos);
                cout<<"<LEFT> USER: "<<userId<<" - LEFT THE GROUP\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //list_requests <group_id> <user_id>
        else if(command[0] == "list_requests"){
            if(command.size() != 3){
                cout<<"<LOGIN TO CREATE A GROUP>\n";
                send(newSocketFd, "<LOGIN TO CREATE A GROUP>", 26, 0);
            }else{
                string groupId = command[1];
                string userId = command[2];
                User user = UsersMap[userId];

                if(Groups.find(groupId) == Groups.end()){
                    cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                    send(newSocketFd, "<GROUP DOESN'T EXIST>", 22, 0);
                }
                else if( Groups[groupId].adminUserId != userId){
                    cout<<"<ERROR>: ONLY ADMIN OF THE GROUP CAN LIST REQUESTS\n";
                    send(newSocketFd, "<ONLY ADMIN OF THIS GROUP CAN SEE PENDING REQUESTS>", 52, 0);
                }else{
                    if(Groups[groupId].pendingRequests.size() == 0){
                        cout<<"<NO PENDING REQUESTS>\n";
                        send(newSocketFd, "<NONE>", 7, 0);
                    }else{
                        string response = "\n\n";
                        int i = 1;
                        for(string uid : Groups[groupId].pendingRequests){
                            response+=to_string(i++);
                            response+=". ";
                            response+=uid;
                            response+="\n";
                        }
                        response+="\n";
                        send(newSocketFd, response.c_str(), response.size(), 0);
                    }
                }
            }
        }

        //accept_request <group_id> <user_id> <admin_user_id>
        else if(command[0] == "accept_request"){
            if(command.size() != 4){
                cout<<"<LOGIN TO SENT ACCEPT REQUEST>\n";
                send(newSocketFd, "<LOGIN TO SENT ACCEPT REQUEST>", 31, 0);
            }

            else{
                string groupId = command[1];
                string userId = command[2];
                string admin = command[3];

                if(Groups.find(groupId) == Groups.end()){
                    cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                    send(newSocketFd, "<GROUP DOESN'T EXIST>", 22, 0);
                }

                else if(UsersMap.find(userId) == UsersMap.end()){
                    cout<<"<ERROR>: USER "<<userId<<" DOESN'T EXIST\n";
                    send(newSocketFd, "<USER DOESN'T EXIST>", 21, 0);
                }

                else {
                    User adminUser = UsersMap[admin];
                    if(Groups[groupId].adminUserId != admin){
                        cout<<"<ERROR>: "<<admin<<" IS NOT THE ADMIN\n";
                        send(newSocketFd, "<CURRENT SESSION DOESN'T HAVE AUTHORIZATION OVER THIS GROUP>", 61, 0);
                    }else{
                       
                        auto pos = Groups[groupId].pendingRequests.find(userId);
                    
                        if(pos != Groups[groupId].pendingRequests.end()){
                            Groups[groupId].userIds.insert(userId);
                            Groups[groupId].pendingRequests.erase(pos);
                            cout<<"<ACCEPTED>: "<<userId<<"\n";
                            send(newSocketFd, "<USER IS ACCEPTED TO THE GROUP>", 32, 0);

                            if(trackerNo == 1){
                                syncCommand(request);
                            }
                        }

                        else{
                            cout<<"<ERROR>: NO PENDING REQUESTS OF "<<userId<<" FOUND\n";
                            send(newSocketFd, "<NO PENDING REQUESTS FOUND>", 28, 0);
                        }
                    }
                }
            }
        }

        //t_accept_request <group_id> <user_id> <admin_user_id>
        else if(command[0] == "t_accept_request"){
            if(trackerNo == 2){
                string groupId = command[1];
                string userId = command[2];
                string admin = command[3];
                auto pos = Groups[groupId].pendingRequests.find(userId);
                Groups[groupId].userIds.insert(userId);
                Groups[groupId].pendingRequests.erase(pos);
                cout<<"<ACCEPTED>: "<<userId<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //upload_file <file_path> <group_id> <user_id> 
        else if(command[0] == "upload_file"){
            if(command.size() != 4){
                cout<<"<LOGIN TO UPLOAD FILES>\n";
                send(newSocketFd, "<LOGIN TO UPLOAD FILES>", 24, 0);
            }

            else{
                string filePath = command[1];
                string groupId = command[2];
                string userId = command[3];

                if(Groups.find(groupId) == Groups.end()){
                    cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                    send(newSocketFd, "<GROUP DOESN'T EXIST>", 22, 0);
                }

                else if(UsersMap.find(userId) == UsersMap.end()){
                    cout<<"<ERROR>: USER "<<userId<<" DOESN'T EXIST\n";
                    send(newSocketFd, "<USER DOESN'T EXIST>", 21, 0);
                }

                else{
                    string fileName = getFileName(filePath);
                    if(Groups[groupId].userIds.find(userId) == Groups[groupId].userIds.end()){
                        cout<<"<ERROR>: USER "<<userId<<" IS NOT A PART OF THE GROUP "<<groupId<<"\n";
                        send(newSocketFd, "<USER NOT PART OF THE GROUP>", 29, 0);
                    }

                    else if(fileNameToSha1Map.find(fileName) != fileNameToSha1Map.end()){
                        Groups[groupId].shareableFiles[fileName].insert(userId);
                        UsersMap[userId].files.insert(fileName);
                        cout<<"<UPLOADED>\n";
                        send(newSocketFd, "<SHA1 ALREADY PRESENT>", 23, 0);

                        if(trackerNo == 1){
                            string req = "1"+request;
                            syncCommand(req);
                        }
                    }

                    else{
                        //filename-> <vector>userids
                        Groups[groupId].shareableFiles[fileName].insert(userId);
                        fileNameToSha1Map[fileName] = "";
                        UsersMap[userId].files.insert(fileName);

                        cout<<"<WAITING FOR SHA1 UPLOADS>\n";
                        send(newSocketFd, "<WAITING FOR SHA1 UPLOADS>", 27, 0);

                        if(trackerNo == 1){
                            string req = "2"+request;
                            syncCommand(req);
                        }
                    }
                }
            }

        }

        //t_1upload_file <file_path> <group_id> <user_id> 
        else if(command[0] == "t_1upload_file"){
            if(trackerNo == 2){
                string filePath = command[1];
                string groupId = command[2];
                string userId = command[3];
                string fileName = getFileName(filePath);
                Groups[groupId].shareableFiles[fileName].insert(userId);
                UsersMap[userId].files.insert(fileName);
                cout<<"<UPLOADED>\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //t_2upload_file <file_path> <group_id> <user_id> 
        else if(command[0] == "t_2upload_file"){
            if(trackerNo == 2){
                string filePath = command[1];
                string groupId = command[2];
                string userId = command[3];
                string fileName = getFileName(filePath);
                Groups[groupId].shareableFiles[fileName].insert(userId);
                fileNameToSha1Map[fileName] = "";
                UsersMap[userId].files.insert(fileName);
                cout<<"<WAITING FOR SHA1 UPLOADS>\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //uploadChunkSha <filename> <chunkNo> <noOfChunks> <chunksha1>
        else if(command[0] == "uploadChunkSha"){
            string fileName = command[1];
            string chunkNo = command[2];
            int cno = atoi(chunkNo.c_str());
            int noOfChunks = atoi(command[3].c_str());
            string chunkSha1 = command[4];
            fileNameToSha1Map[fileName]+=chunkSha1;
            if(cno == noOfChunks-1){
                send(newSocketFd, "<SHA1 UPLOAD COMPLETED>", 24, 0);
            }
            else send(newSocketFd, "<NEXT>", 7, 0);

            if(trackerNo == 1){
                syncCommand(request);
            }
        }

        //t_uploadChunkSha <filename> <chunkNo> <noOfChunks> <chunksha1>
        else if(command[0] == "t_uploadChunkSha"){
            if(trackerNo == 2){
                string fileName = command[1];
                string chunkNo = command[2];
                string chunkSha1 = command[4];
                fileNameToSha1Map[fileName]+=chunkSha1;
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //list_files <group_id>
        else if(command[0] == "list_files"){
            string groupId = command[1];
            if(Groups.find(groupId) == Groups.end()){
                cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                send(newSocketFd, "<GROUP DOESN'T EXIST>", 22, 0);
            }else{
                if(Groups[groupId].shareableFiles.size() == 0){
                    cout<<"<NO SHARABLE FILES>\n";
                    send(newSocketFd, "<NONE>", 7, 0);
                }
                
                else{
                    string response = "\n\n";
                    int i = 1;
                    for(auto file : Groups[groupId].shareableFiles){
                        response+=to_string(i++);
                        response+=". ";
                        response+=file.first;
                        response+="\n";
                    }
                    response+="\n";
                    send(newSocketFd, response.c_str(), response.size(), 0);
                }
            }
        }

        //download_file <group_id> <file_name> <destination_path> <user_id>
        else if(command[0] == "download_file"){
            if(command.size() != 5){
                cout<<"<LOGIN TO UPLOAD FILES>\n";
                send(newSocketFd, "<LOGIN TO UPLOAD FILES>", 24, 0);
            }
            else{

                string groupId = command[1];
                string fileName = command[2];
                string destinationPath = command[3];
                string userId = command[4];

                if(Groups.find(groupId) == Groups.end()){
                    cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                    send(newSocketFd, "<ERROR1>", 9, 0);
                }

                else if(Groups[groupId].shareableFiles.find(fileName) == Groups[groupId].shareableFiles.end()){
                    cout<<"<ERROR>: FILE DOESN'T EXIST IN THE GROUP\n";
                    send(newSocketFd, "<ERROR2>", 9, 0);
                }

                else if(Groups[groupId].userIds.find(userId) == Groups[groupId].userIds.end()){
                    cout<<"<ERROR>: USER NOT A MEMBER OF THE GROUP\n";
                    send(newSocketFd, "<ERROR3>", 9, 0);
                }

                else{
                    set<string> userIds = (Groups[groupId].shareableFiles)[fileName];
                    string res = "";
                    for(string uid : userIds){
                        //uid-ip-port
                        if(UsersMap[uid].live){
                            res+=uid;
                            res+="-";
                            res+=UsersMap[uid].ip;
                            res+="-";
                            res+=to_string(UsersMap[uid].port);
                            res+=" ";
                        }
                    }
                    res.pop_back();
                    write(newSocketFd, res.c_str(), strlen(res.c_str()));
                    cout<<"<SEND>: META DATA SUCCESFULLY FORWARDED\n";
                }

            }
        }
        
        //getSha1 <fileName> 
        else if(command[0] == "getSha1"){
            string fileName = command[1];
            string sha1 = fileNameToSha1Map[fileName];
            int sz = sha1.size();
            // send(newSocketFd, sha1.c_str(), sha1.size(), 0);
            
            int sendBytes = 0;
            int totalBytesSent = 0;
            while(totalBytesSent!=sz){
                cout<<totalBytesSent<<" out of "<<sz<<"\n";
                sendBytes = send(newSocketFd, sha1.c_str()+totalBytesSent, sha1.size(), 0);
                totalBytesSent+=sendBytes;
            }
        }
        
        //getSha1 <fileName> <chunkNo> <sha1>
        else if(command[0] == "checkSha1chunk"){
            string fileName = command[1];
            int chunkNo = atoi(command[2].c_str());
            string sha1Recv = command[3];
            cout<<chunkNo<<" ";
            string sha1Data = fileNameToSha1Map[fileName].substr(chunkNo*40, 40);
            cout<<sha1Data<<"\n";
         
            if(sha1Data == sha1Recv){
                write(newSocketFd, "SHA1 VERIFIED", 14);
                return;
            }

            else{
                cout<<sha1Recv<<"\n"<<sha1Data<<"\n"<<"corrupted\n";
                write(newSocketFd, "CORRUPETD", 10);
            }
        }

        //addSeeder <group_id> <fileName> <user_id>
        else if(command[0] == "addSeeder"){
            string gid = command[1];
            string fileName = command[2];
            string uid = command[3];
            Groups[gid].shareableFiles[fileName].insert(uid);
            cout<<"<NEW SEEDER ADDED>: FileName: "<<fileName<<"\n";
            send(newSocketFd, "<USER ADDED AS SEEDER>", 23, 0);

            if(trackerNo == 1){
                syncCommand(request);
            }
        }

        //t_addSeeder <group_id> <fileName> <user_id>
        else if(command[0] == "t_addSeeder"){
            if(trackerNo == 2){
                string gid = command[1];
                string fileName = command[2];
                string uid = command[3];
                Groups[gid].shareableFiles[fileName].insert(uid);
                cout<<"<NEW SEEDER ADDED>: FileName: "<<fileName<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }

        //stop_share <group_id> <file_name> <uid>
        else if(command[0] == "stop_share"){
            string gid = command[1];
            string fileName = command[2];
            string uid = command[3];

            if(Groups.find(gid) == Groups.end()){
                cout<<"<ERROR>: GROUP DOESN'T EXIST\n";
                send(newSocketFd, "<GROUP DOESN'T EXIST>", 22, 0);
            }
            
            else if(Groups[gid].shareableFiles.find(fileName) == Groups[gid].shareableFiles.end()){
                cout<<"<ERROR>: FILE DOESN'T EXIST\n";
                send(newSocketFd, "<FILE DOESN'T EXIST>", 21, 0);
            }
            
            else{
                set<string>::iterator it = Groups[gid].shareableFiles[fileName].find(uid);
                if(it == Groups[gid].shareableFiles[fileName].end()){
                    send(newSocketFd, "<USER IS NOT SHARING THIS FILE>", 23, 0);
                }
                else{
                    Groups[gid].shareableFiles[fileName].erase(it);
                    if(Groups[gid].shareableFiles[fileName].size() == 0){
                        Groups[gid].shareableFiles.erase(Groups[gid].shareableFiles.find(fileName));
                    }
                    cout<<"<"<<uid<<" STOPPED SHARING>: FILENAME: "<<fileName<<" IN "<<"GROUP: "<<gid<<"\n";
                    send(newSocketFd, "<STOPPED SHARING>", 23, 0);

                    if(trackerNo == 1){
                        syncCommand(request);
                    }
                }
            }

        }

        //t_stop_share <group_id> <file_name> <uid>
        else if(command[0] == "t_stop_share"){
            if(trackerNo == 2){
                string gid = command[1];
                string fileName = command[2];
                string uid = command[3];
                set<string>::iterator it = Groups[gid].shareableFiles[fileName].find(uid);
                Groups[gid].shareableFiles[fileName].erase(it);
                if(Groups[gid].shareableFiles[fileName].size() == 0){
                    Groups[gid].shareableFiles.erase(Groups[gid].shareableFiles.find(fileName));
                }
                cout<<"<"<<uid<<" STOPPED SHARING>: FILENAME: "<<fileName<<" IN "<<"GROUP: "<<gid<<"\n";
                send(newSocketFd, "<TRACKER2 SYNCED>", 18, 0);
            }
        }
        
        close(newSocketFd);
}
