# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

set(SYNTHSOURCE ../../source/)

SET(GCC_COVERAGE_COMPILE_FLAGS -Wall -Werror -g -std=c++14 -Ofast)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(GCC_COVERAGE_COMPILE_FLAGS ${GCC_COVERAGE_COMPILE_FLAGS} -frecord-gcc-switches)
endif()

include_directories(${PROJECT_SOURCE_DIR}/${SYNTHSOURCE})
add_compile_options(${GCC_COVERAGE_COMPILE_FLAGS})

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

file(   GLOB MYFILES ${SYNTHSOURCE}/*.cpp
        ${SYNTHSOURCE}/aaudio/*.cpp
        ${SYNTHSOURCE}/tools/AdpfWrapper.cpp
        ${SYNTHSOURCE}/tools/HostTools.cpp
        )

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             src/main/cpp/native-lib.cpp

        ${MYFILES})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log
              )

find_library(opensles-lib OpenSLES)
find_library(aaudio-lib aaudio)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}

                       ${opensles-lib}
                       ${aaudio-lib}

                       )

