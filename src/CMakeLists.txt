CMAKE_MINIMUM_REQUIRED(VERSION 3.16.3)

project(mython_interpreter)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} 
comparators.cpp
lexer.cpp
mython.cpp
object.cpp
object_holder.cpp
object_holder_test.cpp
object_test.cpp
parse.cpp
parse_test.cpp
statement.cpp
statement_test.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
