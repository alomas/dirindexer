#include <iostream>
#include <iomanip>
#include <dirent.h>
#include <sstream>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <openssl/md5.h>

using namespace std;

struct filedata {
    string filename;
    string fullpath;
    unsigned long filesize{};
    string md5;
};

std::fstream in;
std::fstream out;
std::fstream debug;

std::ostream& operator << (std::ostream& os, const filedata& fileobject)
{
    return os << fileobject.fullpath << endl << fileobject.filename << endl << fileobject.filesize << endl << fileobject.md5 << endl;
}

std::istream& operator >> (std::istream& os, filedata& fileobject)
{
    std::getline( os,  fileobject.fullpath);
    std::getline( os , fileobject.filename);
    string filesizestr;
    std::getline( os , filesizestr);
    if (!(filesizestr.empty()))
        fileobject.filesize = std::stol(filesizestr);
    std::getline(os, fileobject.md5);
    return os;
}

string getMD5(const char *fullpath)
{
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    int i;
    unsigned char checksum[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen ((const char *) fullpath, "rb");

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
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
    fclose (inFile);
    string md5;
    md5 = oss.str();
    return md5;
}

int loadTree(std::map<string, filedata> *filemap)
{
    filedata indata;
    int count = 0;
    debug.open("debug.txt", std::ios::out);
    while (!(in.eof()))
    {
        in >> indata;
        if (!(in.eof()))
        {
            count++;
            filemap->insert(std::make_pair(indata.fullpath, indata));
            debug << indata;
        }
    }
    cout << "Number of elements read in: " << filemap->size() << endl;
    debug.close();
    return 0;
}



unsigned long getFileSize(const string& fullpath)
{
    std::ifstream file(fullpath);
    file.seekg (0, file.end);
    unsigned long length = file.tellg();
    file.close();
    return length;
}

int getDirectory(const char *rootdir, int depth, std::map<string, filedata> *filemap)
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
                if (entry->d_type & DT_DIR)
                {
                    ostringstream oss;
                    oss << rootdir;
                    if (strcmp(rootdir, "/") != 0)
                        oss << "/";
                    oss << entry->d_name;
                    getDirectory(oss.str().c_str(), depth, filemap);
                }
                else
                    if (((entry->d_type & DT_REG) == DT_REG) && (entry->d_type & DT_LNK) != DT_LNK)
                    {
                        ostringstream oss;
                        oss << rootdir;
                        if (strcmp(rootdir, "/") != 0)
                            oss << "/";
                        oss << entry->d_name;
                        filemap->insert(std::make_pair(oss.str(), fileobject));
                        fileobject.fullpath = oss.str();
                        fileobject.filesize = getFileSize(fileobject.fullpath);
                        fileobject.md5 = string(getMD5(fileobject.fullpath.c_str()));
                        if (out.is_open())
                            out << fileobject;
                    }
            }
        }
        closedir(dir);
    }
    return 0;
}


int main(int argc, char *argv[]) {
    char *rootdir;
    std::map<std::string, filedata> *filemap;
    filemap = new std::map<string, filedata>;

    std::cout << "DirIndexer v0.01α" << std::endl;
    if (argc > 1)
    {
        rootdir = argv[1];
        int dirlen = strlen(rootdir);
        if (strlen(rootdir) > 1 && rootdir[dirlen-1] == '/')
            rootdir[dirlen-1] = 0;
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

    filemap->clear();
    loadTree(filemap);

    delete (filemap);
    filemap = nullptr;
    return 0;
}
