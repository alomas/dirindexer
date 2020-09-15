#include <iostream>
#include <dirent.h>
#include <sstream>
#include <cstring>
#include <map>
#include <iterator>

using namespace std;

int getDirectory(const char *rootdir, int depth, std::map<string, string> *filemap)
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
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                if (entry->d_type & DT_DIR)
                {
                    cout << "(D)";
                    for (int f=0; f<depth; f++)
                        cout << ".\t";
                    char *newpath;
                    newpath = new char(strlen(entry->d_name) + strlen(rootdir) + 1 );
                    ostringstream oss;
                    oss << rootdir;
                    if (strcmp(rootdir, "/"))
                        oss << "/";
                    oss << entry->d_name;

                    cout <<  entry->d_name << endl;
                    getDirectory(oss.str().c_str(), depth, filemap);
                }
                else
                {
                    ostringstream oss;
                    oss << rootdir;
                    if (strcmp(rootdir, "/"))
                        oss << "/";
                    oss << entry->d_name;
                    filemap->insert(std::make_pair(oss.str(), entry->d_name));
                    for (int f=0; f<(depth+1); f++)
                        cout << ".\t";
                    cout << entry->d_name << endl;
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
    std::map<std::string, std::string> *filemap;
    filemap = new std::map<string, string>;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
    }
    getDirectory(rootdir, 0, filemap);
    cout << "Number of elements for (" << rootdir << ") : " << filemap->size() << endl;
    return 0;
}
