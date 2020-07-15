cmake_minimum_required(VERSION 3.2)
project(nightc)

set(SOURCE_FILES night.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} ${NIGHTC_INCLUDE_LIBS})
target_link_libraries(${PROJECT_NAME} ${NIGHTC_LIBS})

add_subdirectory(driver)