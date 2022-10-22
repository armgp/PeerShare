#ifndef TRACKER_FUNCTIONS
#define TRACKER_FUNCTIONS

#include <iostream>
#include <thread>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <set>
#include <fstream>
#include <semaphore.h>

using namespace std;

// classes
class Tracker
{
public:
    int id;
    string ip;
    int port;
    Tracker() {}
    Tracker(int _id, string _ip, int _port)
    {
        id = _id;
        ip = _ip;
        port = _port;
    }
};
class User
{
public:
    string uid;
    string password;
    string ip;
    int port;
    bool live = false;
    unordered_map<string, int> joinedGroups;
    unordered_map<string, int> adminedGroups;
    set<string> files; // filename -> SharedFile

    User()
    {
    }

    User(string _uid, string _password)
    {
        uid = _uid;
        password = _password;
    }

    int joinGroup(string groupId)
    {
        if (joinedGroups.find(groupId) != joinedGroups.end())
            return -1;
        joinedGroups[groupId] = 1;
        return 0;
    }

    int addToAdminedGroups(string groupId)
    {
        if (adminedGroups.find(groupId) != adminedGroups.end())
            return -1;
        adminedGroups[groupId] = 1;
        return 0;
    }
};
class Group
{
public:
    string adminUserId;
    set<string> userIds;
    set<string> pendingRequests;                       // userids
    unordered_map<string, set<string>> shareableFiles; // filename-> <vector>userids
};

// structs
struct Server
{
    int domain;
    int type;
    int protocol;
    u_long interface;
    int port;
    int backlog;
    struct sockaddr_in address;
    int socket;
};
struct Client
{
    int socket;
    int domain;
    int type;
    int protocol;
    int port;
    u_long interface;

    char *(*request)(struct Client *client, string serverIp, int port, string req, int resSize, int flag);
};
struct ThreadParams
{
    int newSocketFd;
    struct Server server;
    bool *status;
};

extern int trackerNo;
extern struct Tracker tracker, tracker2;
extern unordered_map<string, string> fileNameToSha1Map; // fileName-> sha1
extern unordered_map<string, string> sharedFileDets;
extern unordered_map<string, User> UsersMap;
extern unordered_map<string, Group> Groups; // groupId, Group

// utils
string getFileName(string filePath);
bool doesFileExist(string filePath);
Tracker getTrackerDetails(string trackerInfoDest, int trackerNo);
vector<string> divideStringByChar(string s, char c);

// server
struct Server serverConstructor(int domain, int type, int protocol, u_long interface, int port, int backlog);
void server(int port, string ip);

// client
char *request(struct Client *client, string serverIp, int port, string req, int resSize, int flag);
struct Client clientConstructor(int domain, int type, int protocol, int port, u_long interface);
void client(string req, string ip, int port);
void syncCommand(string request);
void processClientRequest(struct ThreadParams params);

#endif
