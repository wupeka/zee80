cmake_minimum_required (VERSION 2.8.11)

get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

# disallow in-source builds
if("${srcdir}" STREQUAL "${bindir}")
	message(FATAL_ERROR "No in-source build" "${srcdir}" "${bindir}")
endif()


project (zee80)

set(CMAKE_VERBOSE_MAKEFILE ON)
#find_package(wxWidgets REQUIRED net gl core base)

find_package(SDL2)
if (MINGW) 
	set(SDL2_LIB -lmingw32 SDL2::SDL2main SDL2::SDL2-static)
else()	
	set(SDL2_LIB -lSDL2 -lSDL2_image)
endif()

#include(${wxWidgets_USE_FILE})
set(CMAKE_CXX_FLAGS "-O2 -ggdb -Wall -fsanitize=address -mtune=native -Wall -std=c++17 -I/usr/local/include")
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib")

add_library (zee80 z80.cpp)

include_directories (SYSTEM ${bindir}/libstsound)

if (MINGW)
	set(hostsetting --host=i686-w64-mingw32)
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
endif()


add_library(stsound STATIC IMPORTED)
add_custom_command(TARGET zee80 PRE_BUILD COMMAND cd libstsound && ./configure ${hostsetting} && make && cp .libs/libstsound.a .. )
#set_target_properties(stsound PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${bindir}/libstsound" INTERFACE_LINK_LIBRARIES " libstsound.a ")
set_target_properties(stsound PROPERTIES IMPORTED_LOCATION libstsound.a)
set(STSOUND_LIBRARIES " libstsound.a ")
set(STSOUND_EXTRA_LINK_FLAG " libstsound.a ")
set(STSOUND_EXTRA_LINK_FLAG_STATIC " SDL2_EXTRA_LINK_FLAGS_STATIC ")

add_executable (zx48k zx48kmain.cpp zx48k.cpp zxtape.cpp emusdl.cpp)
add_executable (pandora pandora.cpp zx48k.cpp zxtape.cpp emusdl.cpp pandsnap.cpp)
add_executable (bigthings bigthings.cpp zx48k.cpp zxtape.cpp emusdl.cpp)

target_link_libraries (pandora LINK_PUBLIC zee80 ${SDL2_LIB} stsound)
target_link_libraries (bigthings LINK_PUBLIC zee80 ${SDL2_LIB} stsound)
target_link_libraries (zx48k LINK_PUBLIC zee80 ${SDL2_LIB} stsound)

#target_include_directories(zx48k PRIVATE ${Boost_INCLUDE_DIRS})
#target_include_directories(pandora PRIVATE ${Boost_INCLUDE_DIRS})
#target_include_directories(bigthings PRIVATE ${Boost_INCLUDE_DIRS})

#add_executable (cpc cpcapp.cpp cpc.cpp emusdl.cpp fdc765.cpp)
#add_executable (z80test z80test.cpp)

#target_link_libraries (cpc LINK_PUBLIC zee80 ${SDL2_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${wxWidgets_LIBRARIES})
#target_link_libraries (z80test LINK_PUBLIC zee80 ${SDL2_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${wxWidgets_LIBRARIES})
