#include <iostream>
#include <iomanip>
#include <include/cxxopts.hpp>
#if _WIN64 || _WIN32
#include <include/dirent.h>
#endif

#if !defined(_WIN64) && !defined(_WIN32)
#include <dirent.h>
#endif

#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <openssl/md5.h>
#include "dirshared.h"

using namespace std;

int loadConfig(cxxopts::Options &options, cxxopts::ParseResult& result, struct configdata& config)
{
    config.maxfilesize = result["max-size"].as<long long>();
    config.minfilesize = result["min-size"].as<long long>();
    config.maxdepth = result["max-depth"].as<int>();
    config.loadfile = result["load-file"].as<bool>();
    config.verbose = result["verbose"].as<bool>();
    config.inputfilestr = result["input"].as<string>();
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
    config.indexmap = new std::map<string, filedata>;
    config.loadmap = new std::map<string, filedata>;

    return 1;
}

int main(int argc, char *argv[]) {
    struct configdata config;

    std::cout << "DirIndexer v0.02 alpha" << endl;

    cxxopts::Options options("DirIndexer", "Index a file system.");
    options.add_options()
        ("d,debug", "Enable debugging (file as parm)", cxxopts::value<std::string>()->default_value("./debug.log"))
        ("r,root-dirs", "Root Directory(ies)", cxxopts::value<std::vector<std::string>>()->default_value("."))
        ("x,max-size", "Max Size file to index", cxxopts::value<long long>()->default_value("-1"))
        ("n,min-size", "Min Size file to index", cxxopts::value<long long>()->default_value("-1"))
        ("p,max-depth", "Max Depth to index", cxxopts::value<int>()->default_value("-1"))
        ("o,output", "Output filename", cxxopts::value<std::string>()->default_value("./output.txt"))
        ("c,case-insensitive", "Ignore case in exclude/include", cxxopts::value<bool>()->default_value("false"))
        ("i,input", "Input filename", cxxopts::value<std::string>()->default_value("./input.txt"))
        ("b,no-index", "Don't index, just read in existing index", cxxopts::value<bool>()->default_value("false"))
        ("l,load-file", "read existing index", cxxopts::value<bool>()->default_value("false"))
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ("t,include-type", "Include file extensions (iso,txt,pdf)", cxxopts::value<std::vector<std::string>>()->default_value(""))
        ("e, exclude-dir", "Exclude directories (dir1,dir2,dir3[?<level>])", cxxopts::value<std::vector<std::string>>()->default_value(""))
        ("h,help", "Help", cxxopts::value<bool>()->default_value("false"))
        ;
    auto result = options.parse(argc, argv);
    loadConfig(options, result, config);
    std::string rootdiropt;

    cout << "Output file = " << config.outputfilestr << endl;

    std::for_each(config.rootdirs.begin(), config.rootdirs.end(), [&config](string rootdiropt)
        {
            cout << "Rootdir = " << rootdiropt << endl;
            if ((rootdiropt.length() > 3) && ((rootdiropt.back() == '/') || (rootdiropt.back() == '\\')))
            {
                rootdiropt.pop_back();
            }
            const char* rootdirstr;
            rootdirstr = rootdiropt.c_str();
            char* rootdir;
            rootdir = (char*)rootdirstr;

            cout << "Rootdirstr = " << rootdirstr << ", rootdir = " << rootdir << endl;

            if (!(config.noindex))
            {
                if (!(config.out.is_open()))
                    config.out.open(config.outputfilestr, std::ios::out);
                getDirectory(rootdir, 0, config);
                cout << "Number of elements generated for (" << rootdir << ") : " << config.indexmap->size() << endl;
            }
        });

    if (config.debug.is_open())
    {
        config.debug.close();
    }
    if (config.out.is_open())
    {
        config.out.close();
    }

    config.indexmap->clear();
    if (config.loadfile)
    {
        cout << "Loading file " << config.inputfilestr << "..." << endl;
        loadTree(config.indexmap, config.inputfilestr, config);
        cout << "Loaded file (" << config.indexmap->size() << " items)" << endl;
    }

    config.indexmap->clear();
    delete (config.indexmap);
    config.indexmap = nullptr;
    return 0;
}