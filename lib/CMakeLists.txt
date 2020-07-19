
option(RE2_BUILD_TESTING "enable testing for RE2" OFF)
add_subdirectory(re2)

set(NIGHTC_LIBS
  re2
  PARENT_SCOPE
)

set(NIGHTC_INCLUDE_LIBS
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/re2
  PARENT_SCOPE
)
