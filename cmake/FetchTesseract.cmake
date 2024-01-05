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
      set(Tesseract_HASH SHA256=70eac5b554527fbe23924c4d685675160329db8d3c8b1aca7c86abea20597a64)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-macos-${Tesseract_VERSION}-Release.tar.gz")
      set(Tesseract_HASH SHA256=70ed181826bc14c387e9f42ca652a564c279d1ebbf0f670b14d75237c861b14b)
    endif()
  elseif(MSVC)
    if(Tesseract_BUILD_TYPE STREQUAL Debug)
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-windows-${Tesseract_VERSION}-Debug.zip")
      set(Tesseract_HASH SHA256=9e32b60b7bb80e98a6d4f35c54859e4cdee0e29b1d2652daf99fe2c24494ae01)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-windows-${Tesseract_VERSION}-Release.zip")
      set(Tesseract_HASH SHA256=840c00f126d7e7932ecdf517cfce7e905f0fd801e495be855c55febffe5bdc60)
    endif()
  else()
    if(Tesseract_BUILD_TYPE STREQUAL Debug)
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-linux-${Tesseract_VERSION}-Debug.tar.gz")
      set(Tesseract_HASH SHA256=cb754aaab8e2ec3cdbb1894921f2f27d62cd05b55496b164f8eb0b38c935d3a0)
    else()
      set(Tesseract_URL "${Tesseract_BASEURL}/tesseract-linux-${Tesseract_VERSION}-Release.tar.gz")
      set(Tesseract_HASH SHA256=f4cdcb9e86374bcb9fadcf23dbe0b097622bec3fcec352b78999f90b902df0a3)
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
                                            ${tesseract_SOURCE_DIR}/lib/libleptonica.a)
  target_include_directories(Tesseract SYSTEM INTERFACE ${tesseract_SOURCE_DIR}/include)
endif()
