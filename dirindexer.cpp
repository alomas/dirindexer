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
    int filesize;
};

std::fstream out;

std::ostream& operator << (std::ostream& os, const filedata fileobject)
{
    return os << fileobject.fullpath << endl << fileobject.filename << endl;
}

std::istream& operator >> (std::istream& os, filedata& fileobject)
{
    os >> fileobject.fullpath;
    os >> fileobject.filename;
    return os;
}

int getDirectory(const char *rootdir, int depth, std::map<string, filedata> *filemap)
{
    struct dirent *entry;
    DIR *dir;
    cout << "Listing directory " << rootdir << endl;
    dir = opendir(rootdir);
    depth++;
    if (dir == NULL)
    {
        perror("Error opening directory.");

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
                    cout << "(D)";
                    for (int f=0; f<depth; f++)
                        cout << ".\t";
                    ostringstream oss;
                    oss << rootdir;
                    if (rootdir[strlen(rootdir)] != '/')
                        oss << "/";
                    oss << entry->d_name;

                    cout <<  entry->d_name << endl;
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
                    for (int f=0; f<(depth+1); f++)
                        cout << ".\t";
                    cout << entry->d_name << endl;
                    fileobject.fullpath = oss.str();
                    if (out.is_open())
                    {
                        out << fileobject;
                        //out.close();
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
    std::map<std::string, filedata> *filemap;
    filemap = new std::map<string, filedata>;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
    }
    out.open("object.txt", std::ios::out );
    getDirectory(rootdir, 0, filemap);
    cout << "Number of elements for (" << rootdir << ") : " << filemap->size() << endl;

    filedata indata;
    filedata outdata;
    std::map<string,filedata>::iterator it=filemap->begin();
    outdata = (filedata)(it->second);


    if (out.is_open())
    {
        out << outdata;
        out.close();
    }
    std::fstream in("object.txt", std::ios::in);
    if (in.is_open())
    {
        in >> indata;
        in.close();
    }
    filemap->clear();
    if (filemap)
    {
        delete(filemap);
        filemap = NULL;
    }
    return 0;
}
