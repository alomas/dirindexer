//
// Created by Adrian Lomas on 01/09/2021.
//

#include "readindex.h"
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
    if (result.count("src-input"))
    {
        std::cout << "Using dst-input file " << config.dstinputfilestr << " (not indexing local folder)" << endl;
        config.usesrcinputfile = true;
        // Why would you specify a source file to load and then not load it?
        config.loadfile = true;
    }
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
    config.indexmap = new std::multimap<string, filedata>;
    config.loadsrcmap = new std::multimap<string, filedata>;
    config.loaddstmap = new std::multimap<string, filedata>;

    return 1;
}

int main(int argc, char *argv[]) {
    struct configdata config;
    long long dupesize = 0;

    std::cout << "ReadIndex v0.02 alpha" << endl;

    cxxopts::Options options("Dupecheck", "Find duplicates within single index.");
    options.add_options()
            ("d,debug", "Enable debugging (file as parm)", cxxopts::value<std::string>()->default_value("./debug.log"))
            ("r,root-dirs", "Root Directory(ies)", cxxopts::value<std::vector<std::string>>()->default_value("."))
            ("x,max-size", "Max Size file to index", cxxopts::value<long long>()->default_value("-1"))
            ("n,min-size", "Min Size file to index", cxxopts::value<long long>()->default_value("-1"))
            ("p,max-depth", "Max Depth to index", cxxopts::value<int>()->default_value("-1"))
            ("o,output", "Output filename", cxxopts::value<std::string>()->default_value("./output.txt"))
            ("c,case-insensitive", "Ignore case in exclude/include", cxxopts::value<bool>()->default_value("false"))
            ("i,input", "Input filename", cxxopts::value<std::string>()->default_value("./input.txt"))
            ("src-input", "Source Input filename (This is list of hashes you want to find in the destination input file)", cxxopts::value<std::string>()->default_value("./src-input.txt"))
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
    }

    std::multimap<long long, filedata> *dupemap;
    dupemap = new std::multimap<long long, filedata>;

        if (!(config.out.is_open()))
            config.out.open(config.outputfilestr, std::ios::out);

        multimap<string, filedata> *indexmap;
        multimap<string, filedata> *loadmap;
        std::multimap<long long, filedata> *dupemaplocal;

        dupemaplocal = dupemap;
        filedata fileobject;

        indexmap = config.loadsrcmap;

        loadmap = config.loaddstmap;
        while (!indexmap->empty()) {
            auto node = indexmap->begin();
            stringstream oss;
            string nodemd5 = node->second.md5;
            oss << node->second;
            string fullpath = oss.str();
            filedata object;
            object.filesize = node->second.filesize;
            object.filename = node->second.filename;
            object.md5 = node->second.md5;
            object.fullpath = node->second.fullpath;

            if (
                    ((config.maxfilesize < 0) ||
                    (config.maxfilesize > -1 && node->second.filesize < config.maxfilesize)) &&
                    ((config.minfilesize < 0) || (config.minfilesize > -1 && node->second.filesize > config.minfilesize))
                    )
            {
                dupesize += object.filesize;
                if (config.verbose)
                {
                    cout << object;
                }
                config.out << object;
            }
            indexmap->erase(nodemd5);
        }

    cout << "Total Data Size: ";
        long long tb = 1073741824 * 1024;
        if ((dupesize / 1024) > (1073741824))
        {
            long long gb = dupesize / 1073741824;

            double tb = (double) ((double) gb / 1024);

            std::cout << std::fixed;
            std::cout << std::setprecision(2);
            cout << tb << "TB" << endl;
        }
        else
            if ((dupesize > 1073741824))
                cout << (dupesize / 1073741824) << "GB" << endl;
            else
                if ((dupesize > 1048576))
                    cout << (dupesize / 1048576) << "MB" << endl;
                else
                    if ((dupesize > 1024))
                        cout << (dupesize / 1024) << "KB" << endl;

    if (config.debug.is_open()){
        config.debug.close();
    }
    if (config.out.is_open()){
        config.out.close();
    }

    config.indexmap->clear();
    delete (config.indexmap);
    config.indexmap = nullptr;
    return 0;
}