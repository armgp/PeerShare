#ifndef CLIENT_FUNCTIONS
#define CLIENT_FUNCTIONS

#include <iostream>
#include <thread>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <sys/stat.h>
#include <limits.h>
#include <unordered_map>
#include <fcntl.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include <algorithm>

#define DESTDIR "/home/user/Documents/Coursework/AOS/Assg/Assg3_18-10-22/2022201024_a3/client"

using namespace std;

//structs
class Tracker{
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
struct Server{
    int domain;
    int type;
    int protocol;
    u_long interface;
    int port;
    int backlog;
    struct sockaddr_in address;
    int socket;
};
struct Client{
    int socket;
    int domain;
    int type;
    int protocol;
    int port;
    u_long interface;

    char *(*request)(struct Client *client, string serverIp, int port, string req, int resSize, int flag);
};

extern struct Tracker tracker;
extern string userid;
extern unordered_map<string, pair<vector<bool>, int>> fileToBitMap;
extern unordered_map<string, string> fileLocMap;                  // fileName -> filePath
extern unordered_map<string, pair<string, char>> downloadedFiles; // fileName*gid -> D/C
extern vector<string> createdDirectories;
extern unordered_map<string, string> dynamicDownloadsShaMap;

//utils
vector<string> divideStringByChar(string s, char c);
string convertBitMapToString(pair<vector<bool>, int> bitMap);
pair<vector<bool>, int> convertStringToBitMap(string str);
string getFileName(string filePath);
void getArgDetails(string &ip, int &port, string &trackerInfoDest, char *arg[]);
vector<Tracker> getTrackerDetails(string trackerInfoDest);
int createUserDirectory(string userId);
string getShaOfString(string value);
string getChunkWiseSha(string path);

//server
struct Server serverConstructor(int domain, int type, int protocol, u_long interface, int port, int backlog);
void server(int port);

//client
char *request(struct Client *client, string serverIp, int port, string req, int resSize, int flag);
struct Client clientConstructor(int domain, int type, int protocol, int port, u_long interface);
void getUsersBitMapThread(string peerIp, int peerPort, string fileName, string peerId, unordered_map<string, string> &userToBitMap, sem_t &mutex);
void downloadChunkFromPeer(string fileName, string peerIp, int peerPort, int chunkNo, int currChunkSize, int fd);
void client(string req, string ip, int port);

#endif
