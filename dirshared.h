#ifndef DIRINDEXER_DIRSHARED_H
#define DIRINDEXER_DIRSHARED_H

#endif //DIRINDEXER_DIRSHARED_H

#include <cstring>
#include <vector>
using namespace std;

struct filedata {
    string filename;
    string fullpath;
    long long filesize{};
    string md5;
};

struct configdata {
    std::fstream                debug;
    std::fstream                out; // Output file stream
    std::vector<std::string>    rootdirs;
    std::vector<std::string>    excludedirs;
    std::vector<std::string>    includetypes;
    bool                        ignorecase = false;
    long long                   maxfilesize = -1;
    long long                   minfilesize = -1;
    int                         maxdepth = -1;
    std::map<std::string, filedata>* filemap = nullptr;
    std::string                 inputfile;
    bool                        loadfile = false;
    bool                        noindex = false;
    std::string                 outputfilestr;
    std::string                 debugfilestr;
};

string getMD5(const char *fullpath, struct configdata &config);