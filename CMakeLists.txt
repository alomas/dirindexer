cmake_minimum_required(VERSION 3.10)
project(dirindexer)

set(CMAKE_CXX_STANDARD 17)
# symlink your version of openssl source to srcdir/openssl
if (WIN32)
	include_directories($ENV{SRCDIR}/include $ENV{WINLIBS}/dirent/ $ENV{WINLIBS}/cxxopts/)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	link_directories($ENV{srcdir}/ /mnt)
	link_libraries(libcrypto)
endif (WIN32)


#include_directories($ENV{srcdir}/include/ $ENV{WINLIBS}/dirent/ $ENV{WINLIBS}/getopt/)

if (UNIX) 
	if (NOT APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
	endif (NOT APPLE)
endif (UNIX)

if (UNIX)
	include_directories($ENV{srcdir}/openssl/include/ $ENV{libs}/cxxopts/)
	link_directories($ENV{srcdir}/openssl/)
	link_libraries(crypto)
endif (UNIX)

add_executable(dirindexer dirshared.cpp dirindexer.cpp dirshared.h)
add_executable(dirupdate dirshared.cpp dirshared.h dirupdate.cpp)
add_executable(findbymd5 dirshared.cpp dirshared.h findbymd5.cpp)


