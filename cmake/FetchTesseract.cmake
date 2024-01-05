include(FetchContent)

set(CUSTOM_TESSERACT_URL
    ""
    CACHE STRING "URL of a downloaded Tesseract static library tarball")

set(CUSTOM_TESSERACT_HASH
    ""
    CACHE STRING "Hash of a downloaded Tesseract staitc library tarball")

if(CUSTOM_TESSERACT_URL STREQUAL "")
  set(USE_PREDEFINED_TESSERACT ON)
else()
  message(STATUS "Using custom Tesseract: ${CUSTOM_TESSERACT_URL}")
  if(CUSTOM_TESSERACT_HASH STREQUAL "")
    message(
      FATAL_ERROR
        "CUSTOM_TESSERACT_HASH not found. Both of CUSTOM_TESSERACT_URL and CUSTOM_TESSERACT_HASH must be present!")
  else()
    set(USE_PREDEFINED_TESSERACT OFF)
  endif()
endif()

if(USE_PREDEFINED_TESSERACT)
  set(Tesseract_VERSION "5.3.3")
  set(Tesseract_BASEURL "https://github.com/occ-ai/obs-ocr-deps/releases/download/${Tesseract_VERSION}-1")

  if(${CMAKE_BUILD_TYPE} STREQUAL Release OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    set(Tesseract_BUILD_TYPE Release)
  else()
    set(Tesseract_BUILD_TYPE Debug)
  endif()

  if(APPLE)
    if(Tesseract_BUILD_TYPE STREQUAL Debug)
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-macos-${Tesseract_VERSION}-Debug.tar.gz")
      set(Tesseract_HASH SHA256=22c7e6a417108b690f4b3bd81364d7025ad1dfc7f7660b5462bf038f4c369421)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-macos-${Tesseract_VERSION}-Release.tar.gz")
      set(Tesseract_HASH SHA256=04aaa394e64af855c66b41c28192c38cfdf01a4030387cb7690e417b7bbd823b)
    endif()
  elseif(MSVC)
    if(Tesseract_BUILD_TYPE STREQUAL Debug)
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-windows-${Tesseract_VERSION}-Debug.zip")
      set(Tesseract_HASH SHA256=8aeae11027f4f1e34a25061208ef88188410e10129b06c0c46c24c411dd1fafc)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-windows-${Tesseract_VERSION}-Release.zip")
      set(Tesseract_HASH SHA256=55d52af8b7a12f3e45475ac28474642d8f1ae9c30f8f5275082f3c0c4b54a308)
    endif()
  else()
    if(Tesseract_BUILD_TYPE STREQUAL Debug)
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-linux-${Tesseract_VERSION}-Debug.tar.gz")
      set(Tesseract_HASH SHA256=105bbd5677a48321b3a81d5b63855a742c72139872aaba42d384f459fde937b5)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-linux-${Tesseract_VERSION}-Release.tar.gz")
      set(Tesseract_HASH SHA256=ec106bbd7083c3067bc55cd3df465b9aff93bc2306894be352cd37cbbd26fd91)
    endif()
  endif()
else()
  set(Tesseract_URL "${CUSTOM_TESSERACT_URL}")
  set(Tesseract_HASH "${CUSTOM_TESSERACT_HASH}")
endif()

FetchContent_Declare(
  tesseract
  URL ${Tesseract_URL}
  URL_HASH ${Tesseract_HASH})
FetchContent_MakeAvailable(tesseract)

add_library(Tesseract INTERFACE)
if(MSVC)
  target_link_libraries(Tesseract INTERFACE ${tesseract_SOURCE_DIR}/lib/tesseract53.lib
                                            ${tesseract_SOURCE_DIR}/lib/leptonica-1.84.1.lib)
  target_include_directories(Tesseract SYSTEM INTERFACE ${tesseract_SOURCE_DIR}/include)
elseif(APPLE)
  target_link_libraries(Tesseract INTERFACE ${tesseract_SOURCE_DIR}/lib/libtesseract.a
                                            ${tesseract_SOURCE_DIR}/lib/libleptonica.a)
  target_include_directories(Tesseract SYSTEM INTERFACE ${tesseract_SOURCE_DIR}/include)
else()
  target_link_libraries(Tesseract INTERFACE ${tesseract_SOURCE_DIR}/lib/libtesseract.a
                                            ${tesseract_SOURCE_DIR}/lib/lib/libleptonica.a)
  target_include_directories(Tesseract SYSTEM INTERFACE ${tesseract_SOURCE_DIR}/include)
endif()
