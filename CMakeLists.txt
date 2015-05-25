cmake_minimum_required(VERSION 3.0.0)
project(WikipediaFinder)
file(GLOB TheSources
    "*.hpp"
    "*.cpp"
)
add_executable(WikipediaFinder ${TheSources})

# only version 3.1.0
# set_property(TARGET WikipediaFinder PROPERTY CXX_STANDARD 11)
# set_property(TARGET WikipediaFinder PROPERTY CXX_STANDARD_REQUIRED ON)
# set_property(TARGET WikipediaFinder PROPERTY CXX_EXTENSIONS OFF)

# for older versions. Note - not compatible with every compiler
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")