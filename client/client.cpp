/* includes */
#include "client_functions.h"

/* state */
struct Tracker tracker;
string userid = "";
unordered_map<string, pair<vector<bool>, int>> fileToBitMap;
unordered_map<string, string> fileLocMap;                  // fileName -> filePath
unordered_map<string, pair<string, char>> downloadedFiles; // fileName*gid -> D/C
vector<string> createdDirectories;
unordered_map<string, string> dynamicDownloadsShaMap;

/* main function */
int main(int n, char *argv[])
{
    if(n<3){
    	cout<<"ARGUMENTS MISSING\n";
    	return 0;
    }  

    string ip, trackerInfoDest;
    int port;
    getArgDetails(ip, port, trackerInfoDest, argv);
    vector<Tracker> trackers = getTrackerDetails(trackerInfoDest);
    tracker = trackers[0];

    // 127.0.0.3:2022
    std::cout << "[Tracker Used]:    TrackerId=> " << tracker.id << " | IP=> " << tracker.ip << " | PORT=> " << tracker.port << "\n";
    std::cout << "[Tracker 2]:    TrackerId=> " << trackers[1].id << " | IP=> " << trackers[1].ip << " | PORT=> " << trackers[1].port << "\n";
    std::cout << "[Peer Client]:    IP=> " << ip << " | PORT=> " << port << "\n";

    struct sockaddr_in servaddr;

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(tracker.ip.c_str());
    servaddr.sin_port = htons(tracker.port);

    thread serverThread(server, port);
    vector<thread> clientThreads;
    while (true)
    {
        int sockfd;
        string req;
        getline(cin, req);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            printf("socket creation for tracker live check failed...\n");
            exit(0);
        }
        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
        {
            cout << "SWITCHING TO SECOND TRACKER\n";
            tracker = trackers[1];
            servaddr.sin_addr.s_addr = inet_addr(tracker.ip.c_str());
            servaddr.sin_port = htons(tracker.port);
        }
        close(sockfd);

        if (req != "" && req != "\n" && req != " ")
            clientThreads.emplace_back(client, req, ip, port);
    }
    for (thread &t : clientThreads)
    {
        t.join();
    }
    serverThread.join();

    return 0;
}
