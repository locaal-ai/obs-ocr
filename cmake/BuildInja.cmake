set(INJA_USE_EMBEDDED_JSON UNIX AND NOT APPLE)
set(INJA_INSTALL OFF)
set(INJA_EXPORT OFF)
set(BUILD_TESTING OFF)
set(INJA_BUILD_TESTS OFF)
set(BUILD_BENCHMARK OFF)
set(COVERALLS OFF)
# inja uses a lot of shadowing
add_compile_options(-Wno-shadow)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/inja ${CMAKE_BINARY_DIR}/inja EXCLUDE_FROM_ALL)