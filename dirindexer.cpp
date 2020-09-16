#include <iostream>
#include <dirent.h>
#include <sstream>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>

using namespace std;

struct filedata {
    string filename;
    string fullpath;
    unsigned long filesize;
};

std::fstream out, in;

std::ostream& operator << (std::ostream& os, const filedata fileobject)
{
    return os << fileobject.fullpath << endl << fileobject.filename << endl << fileobject.filesize << endl;
}

std::istream& operator >> (std::istream& os, filedata& fileobject)
{

    std::getline( os,  fileobject.fullpath);
    std::getline( os , fileobject.filename);
    string filesizestr;
    std::getline( os , filesizestr);
    if (!(filesizestr.empty()))
        fileobject.filesize = std::stol(filesizestr);

    return os;
}
int loadTree(std::map<string, filedata> *filemap)
{
    filedata indata;
    int count = 0;
    while (!(in.eof()))
    {
        in >> indata;
        if ((in.eof()))
        {
            bool eof = true;
        }
        else
        {
            count++;
            filemap->insert(std::make_pair(indata.fullpath, indata));
        }
    }
    cout << "Number of elements read in: " << filemap->size() << endl;
}
int getDirectory(const char *rootdir, int depth, std::map<string, filedata> *filemap)
{
    struct dirent *entry;
    DIR *dir;
    dir = opendir(rootdir);
    depth++;
    if (dir == NULL)
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
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                if (entry->d_type & DT_DIR)
                {
                    ostringstream oss;
                    oss << rootdir;
                    if (rootdir[strlen(rootdir)] != '/')
                        oss << "/";
                    oss << entry->d_name;
                    getDirectory(oss.str().c_str(), depth, filemap);
                }
                else
                {
                    ostringstream oss;
                    oss << rootdir;
                    if (rootdir[strlen(rootdir)] != '/')
                        oss << "/";
                    oss << entry->d_name;
                    filemap->insert(std::make_pair(oss.str(), fileobject));
                    fileobject.fullpath = oss.str();
                    unsigned long filesize;
                    std::ifstream file(fileobject.fullpath);
                    file.seekg (0, file.end);
                    unsigned long length = file.tellg();
                    file.close();
                    fileobject.filesize = length;
                    if (out.is_open())
                    {
                        out << fileobject;
                    }
                }
            }
        }
        closedir(dir);
    }
}

int main(int argc, char *argv[]) {
    struct dirent *entry;
    DIR *dir;
    const char *rootdir;
    std::map<std::string, filedata> *filemap, *filemapin;
    filemap = new std::map<string, filedata>;
    filemapin = new std::map<string, filedata>;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
    }
    if (argc > 2)
        out.open(argv[2], std::ios::out);
    else
        out.open("object.txt", std::ios::out );
    getDirectory(rootdir, 0, filemap);
    cout << "Number of elements generated for (" << rootdir << ") : " << filemap->size() << endl;

    filedata indata;
    filedata outdata;
    std::map<string,filedata>::iterator it=filemap->begin();
    outdata = (filedata)(it->second);


    if (out.is_open())
    {
        out.close();
    }
    if (argc > 2)
        in.open(argv[2], std::ios::in);
    else
        in.open("object.txt", std::ios::in );

    loadTree(filemapin);

    filemap->clear();
    if (filemap)
    {
        delete(filemap);
        filemap = NULL;
    }
    return 0;
}
