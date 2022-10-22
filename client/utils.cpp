#include "client_functions.h"

vector<string> divideStringByChar(string s, char c){
    int n = s.size();
    int st = 0;
    vector<string> res;
    for(int i=0; i<n; i++){
        if(s[i]==c){
            res.push_back(s.substr(st, (i-st)));
            st = i+1;
        }
    }
    res.push_back(s.substr(st));
    return res;
}

string convertBitMapToString(pair<vector<bool>, int> bitMap){
    vector<bool> bits = bitMap.first;
    int lastChunkSize = bitMap.second;
    string res="";
    for(bool b : bits){
        if(b){
            res+="1";
        }else{
            res+="0";
        }
    }
    res+=" ";
    res+=to_string(lastChunkSize);
    return res;
}

pair<vector<bool>, int> convertStringToBitMap(string str){
    vector<bool> v;
    int lastChunkSize;
    vector<string> infos = divideStringByChar(str, ' ');
    lastChunkSize = atoi(infos[1].c_str());
    for(char c : infos[0]){
        if(c == '1'){
            v.push_back(true);
        }else if(c == '0'){
            v.push_back(false);
        }
    }
    return make_pair(v, lastChunkSize);
}

string getFileName(string filePath){
    int n = filePath.size();
    for(int i=n-1; i>=0; i--){
        if(filePath[i] == '/'){
            return filePath.substr(i+1);
        }
    }

    return filePath;
}

void getArgDetails(string& ip, int& port, string& trackerInfoDest, char* arg[]){
    string ipAndPort = arg[1];
    trackerInfoDest = arg[2];
    int n = ipAndPort.size();
    for(int i=0; i<n; i++){
        if(ipAndPort[i] == ':'){
            ip = ipAndPort.substr(0, i);
            port = atoi(ipAndPort.substr(i+1).c_str());
            return;
        }
    }
}

vector<Tracker> getTrackerDetails(string trackerInfoDest){
    string jsonfile = "traker_info.json";
    ifstream trackerInfoFile;
    trackerInfoFile.open(trackerInfoDest);
    ofstream trackerInfoJson;
    trackerInfoJson.open("traker_info.json");
    string content = "";

    int i = 0;
    for(i=0; !trackerInfoFile.eof(); i++) 
    content += trackerInfoFile.get();
    trackerInfoFile.close();
    content.erase(content.end()-1);  
    i--;

    trackerInfoJson << content;
    trackerInfoJson.close();

    ifstream jsonFile;
    jsonFile.open(jsonfile);
    Json::Value jsonData;
    Json::Reader reader;
    Json::FastWriter fastWriter; //converts Json::Value to string

    reader.parse(jsonFile, jsonData);
    
    string id_s1 = fastWriter.write(jsonData["trackers"][0]["id"]);
    string id_s2 = fastWriter.write(jsonData["trackers"][1]["id"]);

    string ip1 = fastWriter.write(jsonData["trackers"][0]["ip"]);
    ip1.pop_back();
    ip1.pop_back();
    ip1 = ip1.substr(1);
    string ip2 = fastWriter.write(jsonData["trackers"][1]["ip"]);
    ip2.pop_back();
    ip2.pop_back();
    ip2 = ip2.substr(1);

    string port_s1 = fastWriter.write(jsonData["trackers"][0]["port"]);
    int id1 = atoi(id_s1.c_str());
    int port1 = atoi(port_s1.c_str());
    string port_s2 = fastWriter.write(jsonData["trackers"][1]["port"]);
    int id2 = atoi(id_s2.c_str());
    int port2 = atoi(port_s2.c_str());

    vector<Tracker> res;
    Tracker tracker1(id1, ip1, port1);
    Tracker tracker2(id2, ip2, port2);
    res.push_back(tracker1);
    res.push_back(tracker2);

    remove(jsonfile.c_str());
    return res;
}

int createUserDirectory(string userId){
    string destinationDirectory = DESTDIR;
    string path=destinationDirectory+"/"+userId;
    mode_t perms = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    int d = mkdir(path.c_str(), perms);
    if (d==-1){
        return 1;
    }
    return 0;
}

string getShaOfString(string value){
    int i = 0;
    unsigned char temp[SHA_DIGEST_LENGTH];
    char buf[SHA_DIGEST_LENGTH*2];

    memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
    memset(temp, 0x0, SHA_DIGEST_LENGTH);

    SHA1((unsigned char *)value.c_str(), value.size(), temp);

    for (i=0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf((char*)&(buf[i*2]), "%02x", temp[i]);
    }

    string res(buf);
    return res;
}

string getChunkWiseSha(string path){
    char buffer[524288];
    int readByte;
    string res = "";

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return "-1";
    else{
        while ((readByte = read(fd, buffer, 524288)) > 0){
            string buf(buffer);
            res+=getShaOfString(buf);
            memset(buffer, 0, 524288);
        }
    }

    return res;
}
