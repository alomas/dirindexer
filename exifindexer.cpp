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
#include "ExifReader.h"
#include "ImageFile.h"

using namespace std;

int loadConfig(cxxopts::Options &options, cxxopts::ParseResult& result, struct configdata& config)
{
    config.maxfilesize = result["max-size"].as<long long>();
    config.minfilesize = result["min-size"].as<long long>();
    config.maxdepth = result["max-depth"].as<int>();
    config.loadfile = result["load-file"].as<bool>();
    config.verbose = result["verbose"].as<bool>();
    config.usestat = result["stat"].as<bool>();
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
    config.indexmap = new std::multimap<string, filedata>;
    config.loadsrcmap = new std::multimap<string, filedata>;

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
            ("stat", "Use stat() functions for file type info (necessary for XFS)", cxxopts::value<bool>()->default_value("false"))
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
    auto filemap = config.indexmap;
    config.exif.open(config.outputfilestr + ".exif", std::ios::out);
    std::for_each(filemap->begin(), filemap->end(), [&config](const std::pair<string,filedata>& item)
    {
        //if (config.verbose)
        {
            // cout << item.second;

            string extension,includeextupper;
            string filename = item.second.fullpath;
            int len = filename.length();
            extension = filename.substr(4);
            size_t found = filename.find_last_of('.');
            string fileext = filename.substr(found + 1);
            string fileextupper = fileext;
            std::transform(fileextupper.begin(), fileextupper.end(), fileextupper.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            std::transform(includeextupper.begin(), includeextupper.end(), includeextupper.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            if (fileextupper == "JPG")
            {
                ImageFile *image;
                image = new ImageFile();
                image->setFilename(filename);
                image->openFile();
                image->findSOI();
                image->getNextChunk();
                char *exifbody = image->getExifBody();
                ExifReader *reader;
                reader = new ExifReader();
                if ((reader->setExifbody(exifbody, image->getExiflength(), filename) == -1))
                {
                    delete image;
                    delete reader;
                    return 1;
                }
                map<int,string> tagresult;
                map<int,string> tagquery = {
                        {0x10f, "No Make"},
                        {0x110, "No Model"},
                        {0x131, "No Software"},
                        {0x132, "No DateTime"},
                        {0xa002, "No Width"},
                        {0xa003, "No Length"},
                        {0xa401, "No HDR"},
                        {0x9203, "No Bright"}
                };
                tagresult = reader->getTags(tagquery);

                string md5;
                stringstream oss2, exifoss;
                for (auto it=tagresult.begin(); it!=tagresult.end(); it++)
                {
                    exifoss << it->second << "|";
                }
                md5 = getMD5FromString(exifoss.str().c_str(), exifoss.str().length(), config);

                oss2 << md5 << "|";
                oss2 << image->getExiflength() << "|";

                oss2 << item.second.filename << "|";
                oss2 << filename << "|" ;


                oss2 << exifoss.str();
                if (image->isJFIF())
                    oss2 << "JFIF|";
                else if (image->isJPG())
                    oss2 << "JPG|";
                else
                    oss2 << "UNK|";

                (config.exif) << oss2.str() << endl;
                delete reader;
                delete image;
            }
            if (fileextupper == "HEIC")
            {
                ImageFile *image;
                map<string, int> chunkmap;

                image = new ImageFile();
                //image->setDebugflag(true);
                image->setFilename(filename);
                image->openFile();
                image->getBoxes();
                image->getHeicExifInfo();
                char *exifdata;
                exifdata = (image->getExifData());
                ExifReader *reader;
                reader = new ExifReader();
                reader->setExifbody(exifdata, image->getHeicExifSize(), filename);
                //reader->printTags();
                map<int,string> tagresult;
                map<int,string> tagquery = {
                        {0x10f, "No Make"},
                        {0x110, "No Model"},
                        {0x131, "No Software"},
                        {0x132, "No DateTime"},
                        {0xa002, "No Width"},
                        {0xa003, "No Length"},
                        {0xa401, "No HDR"},
                        {0x9203, "No Bright"}
                };
                tagresult = reader->getTags(tagquery);
                string md5;
                stringstream oss2, exifoss;
                for (auto it=tagresult.begin(); it!=tagresult.end(); it++)
                {
                    exifoss << it->second << "|";
                }
                // md5 = getMD5FromString(image->getExifData(), image->getHeicExifSize(), config);
                md5 = getMD5FromString(exifoss.str().c_str(), exifoss.str().length(), config);


                oss2 << md5 << "|";
                oss2 << image->getHeicExifSize() << "|";

                oss2 << item.second.filename << "|";
                oss2 << filename << "|" ;

                oss2 << exifoss.str();
                if (image->isJFIF())
                    oss2 << "JFIF|";
                else if (image->isJPG())
                    oss2 << "JPG|";
                else
                    oss2 << "UNK|";

                (config.exif) << oss2.str() << endl;
                delete reader;
                delete image;

            }
            //cout << fileext << endl;
        }
       // config.out << item.second;
    }
    );
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
    config.loadsrcmap->clear();
    delete (config.loadsrcmap);
    delete (config.indexmap);
    config.exif.close();
    config.indexmap = nullptr;
    return 0;
}