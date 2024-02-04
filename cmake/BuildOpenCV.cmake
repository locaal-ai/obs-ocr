include(FetchContent)

set(OPENCV_SOURCE_URL
    ""
    CACHE STRING "URL of a Tesseract source tarball")

set(OPENCV_SOURCE_HASH
    ""
    CACHE STRING "Hash of a Tesseract source tarball")

FetchContent_Declare(
  opencv_source
  URL ${OPENCV_SOURCE_URL}
  URL_HASH ${OPENCV_SOURCE_HASH})
FetchContent_Populate(opencv_source)

set(BUILD_OPENCV_BASEDIR ${CMAKE_BINARY_DIR}/build-opencv)

set(BUILD_OPENCV_OUTPUTS
    ${BUILD_OPENCV_BASEDIR}/release/${CMAKE_BUILD_TYPE}/lib/libopencv_imgproc.a
    ${BUILD_OPENCV_BASEDIR}/release/${CMAKE_BUILD_TYPE}/lib/libopencv_core.a
    ${BUILD_OPENCV_BASEDIR}/release/${CMAKE_BUILD_TYPE}/lib/opencv4/3rdparty/libzlib.a)

set(BUILD_OPENCV_INCLUDE_DIR ${BUILD_OPENCV_BASEDIR}/release/${CMAKE_BUILD_TYPE}/include/opencv4)

add_custom_command(
  OUTPUT ${BUILD_OPENCV_OUTPUTS}
  COMMAND ${CMAKE_SOURCE_DIR}/vendor/obs-backgroundremoval-dep-opencv/build-linux.sh ${CMAKE_BUILD_TYPE} main
          ${opencv_source_SOURCE_DIR}
  WORKING_DIRECTORY ${BUILD_OPENCV_BASEDIR})
add_custom_target(build_opencv DEPENDS ${BUILD_OPENCV_OUTPUTS})

add_library(OpenCV INTERFACE)
add_dependencies(OpenCV build_opencv)
target_link_libraries(OpenCV INTERFACE ${BUILD_OPENCV_OUTPUTS})
target_include_directories(OpenCV SYSTEM INTERFACE ${BUILD_OPENCV_INCLUDE_DIR})
