#minimum need for version
cmake_minimum_required(VERSION 3.16)

# project name called "linux-pwn-lab" using C++
project(linux-pwn-lab CXX)

#C++ version C++17
set(CMAKE_CXX_STANDARD 17)

#must use C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)        #find capstone installed or not   if not installed compire error
pkg_check_modules(CAPSTONE REQUIRED capstone)   #go to find capstone and set capstone

#builed an ".exe" called hello , the origin code is hello.cpp
add_executable(hello src/hello.cpp)

add_executable(mapview
                 src/main.cpp
                 src/process/process.cpp
                 src/memory/memory.cpp
                 src/symbols/symbols.cpp
                 src/debugger/debugger.cpp
                 src/debugger/cli.cpp
              )

target_link_libraries(mapview PRIVATE ${CAPSTONE_LIBRARIES}) #link to capstone set as private find .so or .a
target_include_directories(mapview PRIVATE ${CAPSTONE_INCLUDE_DIRS})    #link capstone .h

# push the .exe under build
set_target_properties(hello PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set_target_properties(mapview PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)