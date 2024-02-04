include(FetchContent)

set(TESSERACT_SOURCE_URL
    ""
    CACHE STRING "URL of a Tesseract source tarball")

set(TESSERACT_SOURCE_HASH
    ""
    CACHE STRING "Hash of a Tesseract source tarball")

set(LEPTONICA_SOURCE_URL
    ""
    CACHE STRING "URL of a Leptonica source tarball")

set(LEPTONICA_SOURCE_HASH
    ""
    CACHE STRING "Hash of a Leptonica source tarball")

FetchContent_Declare(
  tesseract_source
  URL ${TESSERACT_SOURCE_URL}
  URL_HASH ${TESSERACT_SOURCE_HASH})
FetchContent_Populate(tesseract_source)

FetchContent_Declare(
  leptonica_source
  URL ${LEPTONICA_SOURCE_URL}
  URL_HASH ${LEPTONICA_SOURCE_HASH})
FetchContent_Populate(leptonica_source)

set(BUILD_TESSERACT_BASEDIR ${CMAKE_BINARY_DIR}/build-obs-ocr-dep)

set(BUILD_TESSERACT_OUTPUTS ${BUILD_TESSERACT_BASEDIR}/release/tesseract/${CMAKE_BUILD_TYPE}/lib/libtesseract.a
                            ${BUILD_TESSERACT_BASEDIR}/release/tesseract/${CMAKE_BUILD_TYPE}/lib/libleptonica.a)

set(BUILD_TESSERACT_INCLUDE_DIR ${BUILD_TESSERACT_BASEDIR}/release/tesseract/${CMAKE_BUILD_TYPE}/include)

add_custom_command(
  OUTPUT ${BUILD_TESSERACT_OUTPUTS}
  COMMAND ${CMAKE_SOURCE_DIR}/vendor/obs-ocr-deps/build-linux.sh ${CMAKE_BUILD_TYPE} main main
          ${tesseract_source_SOURCE_DIR} ${leptonica_source_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/build-obs-ocr-dep)
add_custom_target(build_tesseract DEPENDS ${BUILD_TESSERACT_OUTPUTS})

add_library(Tesseract INTERFACE)
add_dependencies(Tesseract build_tesseract)
target_link_libraries(Tesseract INTERFACE ${BUILD_TESSEACT_OUTPUTS})
target_include_directories(Tesseract SYSTEM INTERFACE ${BUILD_TESSERACT_INCLUDE_DIR})
