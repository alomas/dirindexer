#include "findbymd5.h"
#include <iostream>
#include <include/cxxopts.hpp>
#if _WIN64 || _WIN32
#include <include/dirent.h>
#endif

#if !defined(_WIN64) && !defined(_WIN32)
#include <dirent.h>
#endif

#include <map>
#include <iterator>
#include <fstream>
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
    config.srcinputfilestr = result["src-input"].as<string>();
    config.dstinputfilestr = result["dst-input"].as<string>();
    if (result.count("dst-input"))
    {
        std::cout << "Using dst-input file " << config.dstinputfilestr << " (not indexing local folder)" << endl;
        config.usedstinputfile = true;
    }
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
    config.loadsrcmap = new std::map<string, filedata>;
    config.loaddstmap = new std::map<string, filedata>;

    return 1;
}

int main(int argc, char *argv[]) {
    struct configdata config;
    int matchedfiles = 0;
    int missingfiles = 0;

    std::cout << "FindByMd5 v0.01 alpha" << endl;

    cxxopts::Options options("FindByMd5", "Find a file by md5 checksum.");
    options.add_options()
            ("d,debug", "Enable debugging (file as parm)", cxxopts::value<std::string>()->default_value("./debug.log"))
            ("r,root-dirs", "Root Directory(ies)", cxxopts::value<std::vector<std::string>>()->default_value("."))
            ("x,max-size", "Max Size file to index", cxxopts::value<long long>()->default_value("-1"))
            ("n,min-size", "Min Size file to index", cxxopts::value<long long>()->default_value("-1"))
            ("p,max-depth", "Max Depth to index", cxxopts::value<int>()->default_value("-1"))
            ("o,output", "Output filename", cxxopts::value<std::string>()->default_value("./output.txt"))
            ("c,case-insensitive", "Ignore case in exclude/include", cxxopts::value<bool>()->default_value("false"))
            ("i,input", "Input filename", cxxopts::value<std::string>()->default_value("./input.txt"))
            ("dst-input", "Destination Input filename (This contains your source-of-truth hashes you want to search)", cxxopts::value<std::string>()->default_value("./dst-input.txt"))
            ("src-input", "Source Input filename (This is list of hashes you want to find in the destination input file)", cxxopts::value<std::string>()->default_value("./src-input.txt"))
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

    cout << "Input file = " << config.inputfilestr << endl;
    cout << "Output file = " << config.outputfilestr << endl;
    config.loadsrcmap->clear();
    if (config.loadfile)
    {
        cout << "Loading src file " << config.srcinputfilestr << "..." << endl;
        loadTreebyMD5(config.loadsrcmap, config.srcinputfilestr, config);
        cout << "Loaded file (" << config.loadsrcmap->size() << " items)" << endl;
        cout << "Loading dst file " << config.dstinputfilestr << "..." << endl;
        loadTreebyMD5(config.loaddstmap, config.dstinputfilestr, config);
        cout << "Loaded file (" << config.loaddstmap->size() << " items)" << endl;
    }

    std::for_each(config.rootdirs.begin(), config.rootdirs.end(), [&config, &missingfiles, &matchedfiles](string rootdiropt)
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
            map<string, filedata> *indexmap;
            map<string, filedata> *loadmap;
            filedata fileobject;
            if (config.usedstinputfile)
            {
                cout << "Using dst-input file map" << endl;
                indexmap = config.loaddstmap;
            }
            else
            {
                cout << "Using local dir index map" << endl;
                indexmap = config.indexmap;
                getDirectory(rootdir, 0, config);
                cout << "Number of elements generated for (" << rootdir << ") : " << config.indexmap->size() << endl;
            }
            loadmap = config.loadsrcmap;
            // map<string, filedata> &item;
            //item = indexmap[1];
            std::for_each(indexmap->begin(), indexmap->end(), [loadmap, &missingfiles, &matchedfiles](std::pair<string,filedata> item)
            {
                //cout << item.second.md5 << ": " << item.second.fullpath << endl;
                auto pairmd5 = loadmap->find(item.second.md5);

                if (pairmd5 == loadmap->end())
                {
                    cout  << "Missing: " << item.second.fullpath << endl;
                    missingfiles++;
                }
                else
                {
                    cout  << "Match: " << item.second.fullpath <<
                    " is " << pairmd5->second.fullpath << endl;
                    matchedfiles++;
                }
            }
            );

            auto pair = indexmap->find("./debug.log");

        }
    });
    cout << "Matched files: " << matchedfiles << endl;
    cout << "Missing files: " << missingfiles << endl;

    if (config.debug.is_open())
    {
        config.debug.close();
    }
    if (config.out.is_open())
    {
        config.out.close();
    }

    config.indexmap->clear();
    delete (config.indexmap);
    config.indexmap = nullptr;
    return 0;
}