cmake_minimum_required(VERSION 3.2)

add_subdirectory(natvis)

project(nightc)

set(SOURCE_FILES night.cpp tokenizer.cpp parser.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${NIGHTC_NATVIS})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} ${NIGHTC_INCLUDE_LIBS} ${NIGHTC_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${NIGHTC_LIBS})

add_subdirectory(driver)
