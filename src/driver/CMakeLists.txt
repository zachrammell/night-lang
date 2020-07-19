cmake_minimum_required(VERSION 3.2)
project(driver)

set(SOURCE_FILES driver.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
