#include <iostream>
#include <dirent.h>
#include <sstream>
#include <cstring>

using namespace std;

int getDirectory(const char *rootdir, int depth)
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
                        cout << "\t";
                    char *newpath;
                    newpath = new char(strlen(entry->d_name) + strlen(rootdir) + 1 );
                    ostringstream oss;
                    oss << rootdir << "/" << entry->d_name;

                    cout <<  entry->d_name << endl;
                    getDirectory(oss.str().c_str(), depth);
                }
                else
                {
                    for (int f=0; f<(depth+1); f++)
                        cout << "\t";
                    cout << entry->d_name << endl;
                }
            }
            else
            {
                cout << "Ignoring " << entry->d_name  << endl;
            }

        }
        closedir(dir);
    }
}
int main(int argc, char *argv[]) {
    struct dirent *entry;
    DIR *dir;
    const char *rootdir;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
    }
    getDirectory(rootdir, 0);

    return 0;
}
