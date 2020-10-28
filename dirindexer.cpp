#include <iostream>
#include <iomanip>
#include <include/cxxopts.hpp>
#if _WIN64 || _WIN32
#include <include/dirent.h>
#endif

#if !defined(_WIN64) && !defined(_WIN32)
#include <dirent.h>
#endif

#include <sstream>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <openssl/md5.h>

using namespace std;

struct filedata {
    string filename;
    string fullpath;
    long long filesize{};
    string md5;
};

struct configdata {
    std::fstream debug;
    std::vector<std::string>    excludedirs;
    std::vector<std::string>    includetypes;
    bool                        ignorecase;
};

std::fstream in;
std::fstream out;
std::stringstream filestream;

std::ostream& operator << (std::ostream& os, const filedata& fileobject)
{
    return os << fileobject.md5 << '|'
        << fileobject.filesize << '|'
        << fileobject.filename << "|"
        << fileobject.fullpath << '|'
        << '\n';
}

std::istream& operator >> (std::istream& os, filedata& fileobject)
{
    std::getline( os,  fileobject.fullpath);
    std::getline( os , fileobject.filename);
    string filesizestr;
    std::getline( os , filesizestr);
    if (!(filesizestr.empty()))
        fileobject.filesize = std::stoll(filesizestr);
    std::getline(os, fileobject.md5);
    return os;
}

std::istream& operator >> (std::stringstream& os, filedata& fileobject)
{
    std::getline(os, fileobject.fullpath);
    std::getline(os, fileobject.filename);
    string filesizestr;
    std::getline(os, filesizestr);
    if (!(filesizestr.empty()))
        fileobject.filesize = std::stoll(filesizestr);
    std::getline(os, fileobject.md5);
    return os;
}
string getMD5(const char *fullpath, struct configdata &config)
{
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[8192];
    int i;
    unsigned char checksum[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen ((const char *) fullpath, "rb");

    if (inFile == nullptr)
    {
        return string("00000000000000000000000000000000");
    }
    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 8192, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (checksum,&mdContext);
    stringstream oss;
    for(i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        printf("%02x", checksum[i]);
        oss << std::right << setw(2) << std::setfill('0') << std::hex << (short)checksum[i];
        string thestring = oss.str();
    }
    printf (" %s\n", fullpath);
    (config.debug) << oss.str() << " " << fullpath << endl;
    fclose (inFile);
    string md5;
    md5 = oss.str();
    return md5;
}

int loadTree(std::map<string, filedata>* filemap, long long maxfilesize, long long minfilesize, string filename)
{
    filedata rec;
    string filesizestr;
    long counter = 0;

    std::ifstream inputfile(filename);
    
    while (std::getline(inputfile, rec.md5, '|') &&
        std::getline(inputfile, filesizestr, '|') &&
        std::getline(inputfile, rec.filename, '|') &&
        std::getline(inputfile, rec.fullpath, '|') &&
        inputfile >> std::ws)
    {
        counter++;
        if (counter % 100000 == 0)
            cout << setfill('0') << setw(7) << counter << ".\t" << rec.fullpath << endl;
        if (!(filesizestr.empty()))
            rec.filesize = std::stoll(filesizestr);
        filemap->insert(std::make_pair(rec.fullpath, rec));
    }

    inputfile.close();
    return 0;

}

#if _WIN64
#define stat _stat64
#else
#include <sys/stat.h>
#define stat stat64
#endif

unsigned long getFileSize(const string& fullpath)
{
    struct stat filestat;
    int result;
    std::ifstream file(fullpath);
    file.seekg(0, file.end);
    unsigned long length = file.tellg();
    file.close();
    result = stat(fullpath.c_str(), &filestat);
    return filestat.st_size;
}

bool isFile(dirent* entry)
{
    #if _WIN64 || _WIN32
    if ( (entry->d_type & DT_REG) == DT_REG)
    {
        return true;
    }
    else
    {
        return false;
    }

    #endif

    #if !defined(_WIN64) && !defined(_WIN32)
    if (((entry->d_type & DT_REG) == DT_REG) && (entry->d_type & DT_LNK) != DT_LNK)
    {
        return true;
    }
    else
    {
        return false;
    }
    cout << "Not windows" << endl;
    #endif
    
    return true;
}

int getDirectory(const char *rootdir, int depth, std::map<string, filedata> *filemap, long long maxfilesize, long long minfilesize, std::vector<std::string> exclude, struct configdata &config)
{
    struct dirent *entry;
    DIR *dir;
    dir = opendir(rootdir);
    depth++;
    if (dir == nullptr)
    {
        string errmsg = "Error opening";
        errmsg += rootdir;
        perror(errmsg.c_str());
    }
    else
    {
        while ((entry = readdir(dir)))
        {
            filedata fileobject;
            fileobject.filename = entry->d_name;
            fileobject.fullpath = rootdir;
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                if ((entry->d_type & DT_DIR) == DT_DIR)
                {
                    ostringstream oss;
                    
                    if ((strlen(rootdir) == 3) && (rootdir[2] == '\\'))
                    {
                        oss << rootdir[0] << rootdir[1] << "/";
                    }
                    else
                    {
                        oss << rootdir;
                        if ((strcmp(rootdir, "/") != 0) && (strcmp(rootdir, "\\") != 0))
                            oss << "/";
                    }
                    oss << entry->d_name;
                    bool skipdir = false;
                    string temppath;
                    
                    std::for_each(exclude.begin(), exclude.end(), [entry, &skipdir, depth](const string str)
                        {
                            string depthstr, excludestr;
                            size_t nPos;
                            bool anydepth;
                            int excludedepth;
                            nPos = str.find("?", 0);
                            if (nPos == -1)
                            {
                                anydepth = true;
                                excludestr = str;
                            }
                            else
                            {
                                anydepth = false;
                                excludestr = str.substr(0, nPos);
                                depthstr = str.substr(nPos + 1, string::npos);
                                excludedepth = stoi(depthstr);
                            }

                            bool depthtest = false;
                            bool comparetest = false;
                            if ((anydepth == true) || (excludedepth == depth))
                                depthtest = true;
                            if ((excludestr.compare(entry->d_name) == 0))
                                comparetest = true;
                            if ( depthtest && comparetest)
                            {
                                skipdir = true;
                            }
                            else
                                int x = 8;
                        });

                    if (!skipdir)
                    {
                        getDirectory(oss.str().c_str(), depth, filemap, maxfilesize, minfilesize, exclude, config);
                    }
                }
                else
                    if (isFile(entry))
                    {
                        bool skipfile;
                        if (config.includetypes.empty())
                            skipfile = false;
                        else
                            skipfile = true;
                        bool alreadymatched = false;
                        std::for_each(config.includetypes.begin(), config.includetypes.end(), [&config, &alreadymatched, entry, &skipfile](const string str)
                            {
                                string fileext, includeextupper, fileextupper;
                                string filename = entry->d_name;
                                size_t found = filename.find_last_of('.');
                                fileext = filename.substr(found + 1);
                                fileextupper = fileext;
                                includeextupper = str;
                                std::transform(fileextupper.begin(), fileextupper.end(), fileextupper.begin(),
                                    [](unsigned char c) { return std::toupper(c); });
                                std::transform(includeextupper.begin(), includeextupper.end(), includeextupper.begin(),
                                    [](unsigned char c) { return std::toupper(c); });
                                if (
                                    (config.ignorecase && (includeextupper.compare(fileextupper) == 0))
                                 || (!(config.ignorecase) && (str.compare(fileext) == 0)))
                                {
                                    skipfile = false;
                                    alreadymatched = true;
                                }
                                else
                                {
                                    if (!(alreadymatched))
                                        skipfile = true;
                                }

                            });
                        if (!skipfile)
                        {
                            ostringstream oss;
                            if ((strlen(rootdir) == 3) && (rootdir[2] == '\\'))
                            {
                                oss << rootdir[0] << rootdir[1] << "/";
                            }
                            else
                            {
                                oss << rootdir;
                                if ((strcmp(rootdir, "/") != 0) && (strcmp(rootdir, "\\") != 0))
                                    oss << "/";
                            }
                            oss << entry->d_name;
                            fileobject.fullpath = oss.str();
                            fileobject.filesize = getFileSize(fileobject.fullpath);
                            if (
                                ((maxfilesize < 0) ||
                                    (maxfilesize > -1 && fileobject.filesize < maxfilesize)) &&
                                ((minfilesize < 0) || (minfilesize > -1 && fileobject.filesize > minfilesize))
                                )
                            {
                                fileobject.md5 = string(getMD5(fileobject.fullpath.c_str(), config));
                                filemap->insert(std::make_pair(oss.str(), fileobject));
                                if (out.is_open())
                                    out << fileobject;
                            }
                        }
                    }
            }
        }
        closedir(dir);
    }
    return 0;
}


int main(int argc, char *argv[]) {
    char *rootdir;
    std::map<std::string, filedata> *filemap;
    filemap = new std::map<string, filedata>;
    int opt;
    bool noindex, loadfile;
    long long maxfilesize, minfilesize;
    std::vector<std::string>    excludedirs;
    string inputfile;
    struct configdata config;

    std::cout << "DirIndexer v0.02 alpha" << endl;

    cxxopts::Options options("DirIndexer", "Index a file system.");
    options.add_options()
        ("d,debug", "Enable debugging (file as parm)", cxxopts::value<std::string>()->default_value("./debug.log"))
        ("r,root-dir", "Root Directory", cxxopts::value<std::string>()->default_value("."))
        ("x,max-size", "Max Size file to index", cxxopts::value<long long>()->default_value("-1"))
        ("n,min-size", "Min Size file to index", cxxopts::value<long long>()->default_value("-1"))
        ("o,output", "Output filename", cxxopts::value<std::string>()->default_value("./output.txt"))
        ("c,case-insensitive", "Ignore case in exclude/include", cxxopts::value<bool>()->default_value("false"))
        ("i,input", "Input filename", cxxopts::value<std::string>()->default_value("./input.txt"))
        ("b,no-index", "Don't index, just read in existing index", cxxopts::value<bool>()->default_value("false"))
        ("l,load-file", "read existing index", cxxopts::value<bool>()->default_value("false"))
        ("t,include-type", "Include file extensions (iso,txt,pdf)", cxxopts::value<std::vector<std::string>>()->default_value(""))
        ("e, exclude-dir", "Exclude directories (dir1,dir2,dir3[?<level>])", cxxopts::value<std::vector<std::string>>()->default_value(""))
        ("h,help", "Help", cxxopts::value<bool>()->default_value("false"))
        ;
    auto result = options.parse(argc, argv);
    maxfilesize = result["max-size"].as<long long>();
    minfilesize = result["min-size"].as<long long>();
    loadfile = result["load-file"].as<bool>();
    inputfile = result["input"].as<string>();
    noindex = result["no-index"].as<bool>();
    excludedirs = result["exclude-dir"].as<std::vector<std::string>>();
    config.includetypes = result["include-type"].as<std::vector<std::string>>();
    config.ignorecase = result["case-insensitive"].as<bool>();
    std::string debugfilestr, outputfilestr, rootdiropt;
  
    outputfilestr = result["output"].as<std::string>();
    debugfilestr = result["debug"].as<std::string>();
    config.debug.open(debugfilestr, std::ios::out);
    cout << "Output file = " << outputfilestr << endl;
    const char* rootdirstr;

    rootdiropt = result["root-dir"].as<std::string>();
    if ((rootdiropt.length() > 3) && ((rootdiropt.back() == '/') || (rootdiropt.back() == '\\')))
    {
        rootdiropt.pop_back();
    }
    rootdirstr = rootdiropt.c_str();
    rootdir = (char*)rootdirstr;

    cout << "Rootdirstr = " << rootdirstr << ", rootdir = " << rootdir << endl;
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (!noindex)
    {
        out.open(outputfilestr, std::ios::out);
        getDirectory(rootdir, 0, filemap, maxfilesize, minfilesize, excludedirs, config);
        cout << "Number of elements generated for (" << rootdir << ") : " << filemap->size() << endl;
    }

    if (config.debug.is_open())
    {
        config.debug.close();
    }
    if (out.is_open())
    {
        out.close();
    }
    in.open(outputfilestr, std::ios::in );

    std::vector<filedata> data;

    filemap->clear();
    if (loadfile)
    {
        cout << "Loading file " << inputfile << "..." << endl;
        loadTree(filemap, maxfilesize, minfilesize, inputfile);
        cout << "Loaded file.";
    }

    filemap->clear();
    delete (filemap);
    filemap = nullptr;
    return 0;
}