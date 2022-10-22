#include "tracker_functions.h"

string getFileName(string filePath){
    int n = filePath.size();
    for(int i=n-1; i>=0; i--){
        if(filePath[i] == '/'){
            return filePath.substr(i+1);
        }
    }

    return filePath;
}

bool doesFileExist(string filePath){
    ifstream test(filePath); 
    return test.good();
}

Tracker getTrackerDetails(string trackerInfoDest, int trackerNo){
    string jsonfile = "traker_info.json";
    ifstream trackerInfoFile;
    trackerInfoFile.open(trackerInfoDest);
    ofstream trackerInfoJson;
    trackerInfoJson.open("traker_info.json");
    string content = "";

    int i = 0;
    for(i=0; !trackerInfoFile.eof(); i++) content += trackerInfoFile.get();
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
    string id_s = fastWriter.write(jsonData["trackers"][trackerNo-1]["id"]);
    string ip = fastWriter.write(jsonData["trackers"][trackerNo-1]["ip"]);
    ip.pop_back();
    ip.pop_back();
    ip = ip.substr(1);
    string port_s = fastWriter.write(jsonData["trackers"][trackerNo-1]["port"]);
    int id = atoi(id_s.c_str());
    int port = atoi(port_s.c_str());

    Tracker tracker(id, ip, port);
    remove(jsonfile.c_str());
    return tracker;
}

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
