cmake_minimum_required(VERSION 3.10)
project(dirindexer)

set(CMAKE_CXX_STANDARD 17)
# symlink your version of openssl source to srcdir/openssl
include_directories($ENV{SRCDIR}/include $ENV{WINLIBS}/dirent/ $ENV{WINLIBS}/cxxopts/)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
#include_directories($ENV{srcdir}/include/ $ENV{WINLIBS}/dirent/ $ENV{WINLIBS}/getopt/)
link_directories($ENV{srcdir}/ /mnt)
link_libraries(libcrypto)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

add_executable(dirindexer dirindexer.cpp)