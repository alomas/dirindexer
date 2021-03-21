#ifndef DIRINDEXER_DIRSHARED_H
#define DIRINDEXER_DIRSHARED_H

#endif //DIRINDEXER_DIRSHARED_H

#include <cstring>
#include <vector>
#include <include/cxxopts.hpp>

#if _WIN64 || _WIN32
#include <include/dirent.h>
#endif

#if !defined(_WIN64) && !defined(_WIN32)
#include <dirent.h>
#endif

#if _WIN64
#define stat _stat64
#elif !defined(__APPLE__)
#include <sys/stat.h>
    #define stat stat64
#endif
#if defined(__APPLE__)
#include <sys/stat.h>
#endif

using namespace std;

struct filedata {
    string filename;
    string fullpath;
    long long filesize{};
    string md5;
};

struct configdata {
    std::fstream                debug;
    std::fstream                matchfile;
    std::fstream                nomatchfile;
    std::fstream                sortedmatchfile;
    std::fstream                out; // Output file stream
    std::fstream                exif;
    std::vector<std::string>    rootdirs;
    std::vector<std::string>    excludedirs;
    std::vector<std::string>    includetypes;
    bool                        ignorecase = false;
    bool                        showfilename = false;
    bool                        quotenames = true;
    long long                   maxfilesize = -1;
    long long                   minfilesize = -1;
    int                         maxdepth = -1;
    std::multimap<std::string, filedata>* indexmap = nullptr;
    std::multimap<std::string, filedata>* loadsrcmap = nullptr;
    std::multimap<std::string, filedata>* loaddstmap = nullptr;
    std::string                 inputfilestr;
    std::string                 srcinputfilestr;
    std::string                 dstinputfilestr;
    std::string                 matchfilestr;
    std::string                 nomatchfilestr;
    std::string                 sortedmatchfilestr;
    bool                        usesrcinputfile = false;
    bool                        loadfile = false;
    bool                        noindex = false;
    std::string                 outputfilestr;
    std::string                 debugfilestr;
    bool                        usestat = false;
    bool                        verbose;
};

string getMD5(const char *fullpath, struct configdata &config);
string getMD5FromString(const char *body, int length, struct configdata &config);
long long getFileSize(const string& fullpath);
int loadTree(std::multimap<string, filedata>* filemap, const string& filename, configdata &config);
int loadTreebyMD5(std::multimap<string, filedata>* filemap, const string& filename, configdata &config);
int getDirectory(const char *rootdir, int depth, struct configdata &config);
bool depthSkipDir(const std::string& str, bool skipdir, int depth, struct dirent* entry);
bool isFile(dirent* entry, struct filedata* fileobject, struct configdata &config);
bool isDir(dirent* entry, struct filedata* fileobject, struct configdata &config);
bool extSkipFile(struct configdata &config, const std::string& str, bool skipfile, bool &alreadymatched, struct dirent* entry);
std::ostream& operator << (std::ostream& os, const filedata& fileobject);