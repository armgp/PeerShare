#include "tracker_functions.h"

/* state */
class Tracker tracker, tracker2;
int trackerNo;
unordered_map<string, string> fileNameToSha1Map; //fileName-> sha1
unordered_map<string, string> sharedFileDets;
unordered_map <string, User> UsersMap;
unordered_map <string, Group> Groups; //groupId, Group

/* main function */
int main(int n, char* argv[]){
    string trackerInfoDest = argv[1];
    trackerNo = atoi(argv[2]);
    tracker = getTrackerDetails(trackerInfoDest, trackerNo);
    tracker2 = getTrackerDetails(trackerInfoDest, 2);
    cout<<"[Tracker Client]:    TrackerId=> "<<tracker.id<<" | IP=> "<<tracker.ip<<" | PORT=> "<<tracker.port<<"\n";

    // Logger::Info("%d", 3);
    thread serverThread(server, tracker.port, tracker.ip);

    while(true){
        string req;
        getline(cin, req);
        if(req != "") {
            client(req, tracker.ip, tracker.port);
            if(req=="quit") exit(0);
        }
    }

    serverThread.join();

    return 0;
}