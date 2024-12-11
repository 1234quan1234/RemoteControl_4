cmake_minimum_required(VERSION 3.10)

# Set the project name
project(GmailTest)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(GmailTest Gmail_Test.cpp)

# Include directories for Google APIs Client Library
include_directories("D:/path/to/google-api-cpp-client/include")

# Link directories for Google APIs Client Library
link_directories("D:/path/to/google-api-cpp-client/lib")

# Link the necessary libraries
target_link_libraries(GmailTest
    googleapis_cpp_gmail
    googleapis_cpp_client
    googleapis_cpp_http
    googleapis_cpp_util
    curl
    ssl
    crypto
    pthread
)
