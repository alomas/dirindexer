#include <iostream>
#include <dirent.h>

using namespace std;

int main(int argc, char *argv[]) {
    struct dirent *entry;
    DIR *dir;
    const char *rootdir;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
    }
    dir = opendir(rootdir);
    if (dir == NULL)
    {
        perror("Error opening directory.");
        exit(1);
    }
    while ((entry = readdir(dir)))
    {
        cout << entry->d_name << endl;
    }

    return 0;
}
