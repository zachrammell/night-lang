cmake_minimum_required(VERSION 3.2)

add_subdirectory(natvis)

project(nightc)

set(SOURCE_FILES night.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${NIGHTC_NATVIS})
target_include_directories(${PROJECT_NAME} ${NIGHTC_INCLUDE_LIBS})
target_link_libraries(${PROJECT_NAME} ${NIGHTC_LIBS})

add_subdirectory(driver)
