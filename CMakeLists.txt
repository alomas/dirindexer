cmake_minimum_required(VERSION 3.10)
project(dirindexer)

set(CMAKE_CXX_STANDARD 17)
# symlink your version of openssl source to srcdir/openssl
include_directories($ENV{srcdir}/openssl/include/)
link_directories($ENV{srcdir}/openssl/)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

add_executable(dirindexer dirindexer.cpp)