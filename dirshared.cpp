#include <cstring>
#include <openssl/md5.h>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <iomanip>
#include "dirshared.h"
#include <sstream>
#include <vector>

using namespace std;

string getMD5(const char *fullpath, struct configdata &config)
{
    MD5_CTX mdContext;
    size_t bytes;
    unsigned char data[8192];
    int i;
    unsigned char checksum[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen ((const char *) fullpath, "rb");

    if (inFile == nullptr)
    {
        return string("00000000000000000000000000000000");
    }
    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 8192, inFile)) != 0)
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
    (config.debug) << oss.str() << " " << fullpath << endl;
    fclose (inFile);
    string md5;
    md5 = oss.str();
    return md5;
}