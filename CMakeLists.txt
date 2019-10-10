# CMake version
cmake_minimum_required(VERSION 3.1)

# Project definition
project(ncnet CXX)

# Sources
set(SRCS
        src/Client.cpp
        src/Connection.cpp
        src/EventPipe.cpp
        src/Network.cpp
        src/Packet.cpp
        src/Log.cpp
        src/Server.cpp)

# Compiler options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-Wall -Wextra -pedantic -fopenmp)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

# Library
add_library(ncnet STATIC ${SRCS})

# Linking
target_link_libraries(ncnet PRIVATE Threads::Threads)
target_link_libraries(ncnet PRIVATE ${OpenMP_CXX_LIBRARIES})

# Includes
target_include_directories(ncnet PRIVATE include)

# Install
install(TARGETS ncnet
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES
            include/Client.h
            include/Network.h
            include/Connection.h
            include/Packet.h
            include/Server.h
            include/Transfer.h
            include/EventPipe.h
            include/Boilerplate.h
            include/Log.h
        DESTINATION
            include/ncnet)