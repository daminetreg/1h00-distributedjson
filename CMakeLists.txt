cmake_minimum_required(VERSION 3.0.0)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/daminetreg/hunter/archive/releases/sam3-support-clang.tar.gz"
    SHA1 "01c971397c44fc87eb1c25e204db4c75aac7edba" 
)


project(jsonsync
  VERSION "0.0.1")

find_package(Threads)

hunter_add_package(Boost)
find_package(Boost CONFIG 1.62 REQUIRED)
include_directories(AFTER ${Boost_INCLUDE_DIRS})
add_definitions(
  -DBOOST_SYSTEM_NO_DEPRECATED
  -DBOOST_ERROR_CODE_HEADER_ONLY
)

hunter_add_package(lib-cpp-pre)
find_package(lib-cpp-pre 1.4.1 REQUIRED)
include_directories(AFTER ${LIB_CPP_PRE_INCLUDE_DIRS})

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src) 
file (GLOB_RECURSE JSONSYNC_SRCS src/*.cpp)
add_executable(jsonsync ${JSONSYNC_SRCS})
target_link_libraries(jsonsync ${CMAKE_THREAD_LIBS_INIT})
