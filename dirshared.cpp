#include <cstring>
#include <openssl/md5.h>
#include <map>
#include <iterator>
#include <fstream>
#include <iomanip>
#include "dirshared.h"
#include <vector>
#include <include/cxxopts.hpp>

#if _WIN64 || _WIN32
#include <include/dirent.h>
#endif

using namespace std;

string getMD5(const char *fullpath, struct configdata &config)
{
    MD5_CTX mdContext;
    size_t bytes;
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

bool isFile(dirent* entry)
{
#if _WIN64 || _WIN32
    if ( (entry->d_type & DT_REG) == DT_REG)
        return true;
    else
        return false;
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
#endif
}
bool depthSkipDir(const std::string& str, bool skipdir, int depth, struct dirent* entry)
{
    string depthstr, excludestr;
    size_t nPos;
    bool anydepth;
    int excludedepth;
    nPos = str.find('?', 0);
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
    if (anydepth || (excludedepth == depth))
        depthtest = true;
    if (excludestr == entry->d_name)
        comparetest = true;
    if (depthtest && comparetest)
    {
        skipdir = true;
    }

    return skipdir;
}

int getDirectory(const char *rootdir, int depth, struct configdata &config)
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

                    std::for_each(config.excludedirs.begin(), config.excludedirs.end(), [entry, &skipdir, depth](const string& str)
                    {
                        skipdir = depthSkipDir(str, skipdir, depth, entry);
                    });

                    if (!skipdir)
                    {
                        if ((config.maxdepth == -1) || (depth < config.maxdepth))
                            getDirectory(oss.str().c_str(), depth, config);
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
                    std::for_each(config.includetypes.begin(), config.includetypes.end(), [&config, &alreadymatched, entry, &skipfile](const string &str)
                    {
                        skipfile = extSkipFile(config, str, skipfile, alreadymatched, entry);
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
                                ((config.maxfilesize < 0) ||
                                 (config.maxfilesize > -1 && fileobject.filesize < config.maxfilesize)) &&
                                ((config.minfilesize < 0) || (config.minfilesize > -1 && fileobject.filesize > config.minfilesize))
                                )
                        {
                            fileobject.md5 = string(getMD5(fileobject.fullpath.c_str(), config));
                            config.filemap->insert(std::make_pair(oss.str(), fileobject));
                            if (config.out.is_open())
                                config.out << fileobject;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    return 0;
}


int loadConfig(cxxopts::Options &options, cxxopts::ParseResult& result, struct configdata& config)
{
    config.maxfilesize = result["max-size"].as<long long>();
    config.minfilesize = result["min-size"].as<long long>();
    config.maxdepth = result["max-depth"].as<int>();
    config.loadfile = result["load-file"].as<bool>();
    config.inputfile = result["input"].as<string>();
    config.noindex = result["no-index"].as<bool>();
    config.excludedirs = result["exclude-dir"].as<std::vector<std::string>>();
    config.rootdirs = result["root-dirs"].as<std::vector<std::string>>();
    config.includetypes = result["include-type"].as<std::vector<std::string>>();
    config.ignorecase = result["case-insensitive"].as<bool>();
    config.outputfilestr = result["output"].as<std::string>();
    config.debugfilestr = result["debug"].as<std::string>();
    config.debug.open(config.debugfilestr, std::ios::out);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    config.filemap = new std::map<string, filedata>;

    return 1;
}

long long getFileSize(const string& fullpath)
{
    struct stat filestat{};
    int result;

    result = stat(fullpath.c_str(), &filestat);
    if (result == 0)
        return filestat.st_size;
    else
        return 0;
}


int loadTree(std::map<string, filedata>* filemap, const string& filename)
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
        if (counter % 1000 == 0)
            cout << setfill('0') << setw(7) << counter << ".\t" << rec.fullpath << endl;
        if (!(filesizestr.empty()))
            rec.filesize = std::stoll(filesizestr);
        filemap->insert(std::make_pair(rec.fullpath, rec));
    }

    inputfile.close();
    return 0;

}


bool extSkipFile(struct configdata &config, const std::string& str, bool skipfile, bool &alreadymatched, struct dirent* entry)
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
            (config.ignorecase && (includeextupper == fileextupper))
            || (!(config.ignorecase) && (str == fileext)))
    {
        skipfile = false;
        alreadymatched = true;
    }
    else
    {
        if (!(alreadymatched))
            skipfile = true;
    }
    return skipfile;

}