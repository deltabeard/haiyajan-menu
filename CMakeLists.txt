CMAKE_MINIMUM_REQUIRED(VERSION 3.19)

# Obtain the version of Haiyajan-menu that is being built.
FIND_PACKAGE(Git)
IF(GIT_FOUND)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --always --dirty=-dirty
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE PROC_RESULT
            OUTPUT_VARIABLE FULL_VERSION
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE PROC_RESULT
            OUTPUT_VARIABLE LAST_TAG
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()

IF(NOT GIT_FOUND OR NOT PROC_RESULT EQUAL 0)
    # If git is not available on the build platform, use this as a fallback.
    SET(LAST_TAG 0.1.0)
    SET(FULL_VERSION ${LAST_TAG}-LOCAL)
ENDIF()

MESSAGE(STATUS "Configuring version ${FULL_VERSION}")

# Initialise project information.
PROJECT(haiyajan-menu
        LANGUAGES C
        VERSION ${LAST_TAG}
        DESCRIPTION "User interface library for Haiyajan"
        HOMEPAGE_URL "https://github.com/deltabeard/haiyajan-menu")
SET(HAIYAJAN_MENU_LICENSE "LGPL-3.0, Mahyar Koshkouei (c) 2021")

# Add dependencies to project.
ADD_EXECUTABLE(${PROJECT_NAME})
TARGET_SOURCES(${PROJECT_NAME} PRIVATE src/main.c src/cache.c src/font.c src/ui.c)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE inc)

# Discover libraries
#INCLUDE(FindPkgConfig)
IF(VCPKG)
    FIND_PACKAGE(SDL2 CONFIG REQUIRED)
    FIND_PACKAGE(freetype CONFIG REQUIRED)
    FIND_PACKAGE(SDL2_ttf CONFIG REQUIRED)
    FIND_PACKAGE(fribidi QUIET)
#ELSEIF(PKG_CONFIG_FOUND)
#    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
#    PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf)
#    PKG_SEARCH_MODULE(FRIBIDI QUIET fribidi)
ELSE()
    # Use included FindSDL2 modules as a last resort.
    LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/cmake")
    FIND_PACKAGE(SDL2 REQUIRED)
    FIND_PACKAGE(SDL2_ttf REQUIRED)
    FIND_PACKAGE(fribidi QUIET)
ENDIF()

# Add required dependencies
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${SDL2_TTF_LIBRARIES})

# Some FindSDL2 modules use slightly different variables, so we just use both.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIR})

# Add options to configure optional dependencies
OPTION(USE_FRIBIDI "Use Fribidi library for bidirection text support"
        ${FRIBIDI_FOUND})

# Process optional dependencies
IF(USE_FRIBIDI AND NOT FRIBIDI_FOUND)
    MESSAGE(SEND_ERROR "Fribidi enabled, but not found")
ELSEIF(USE_FRIBIDI)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${FRIBIDI_LIBRARIES})
    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${FRIBIDI_INCLUDE_DIRS})
ELSE()
    ADD_COMPILE_DEFINITIONS(NO_FRIBIDI)
ENDIF()

# Add definitions of project information.
ADD_COMPILE_DEFINITIONS(COMPANY=Deltabeard)
ADD_COMPILE_DEFINITIONS(DESCRIPTION=${PROJECT_DESCRIPTION})
ADD_COMPILE_DEFINITIONS(LICENSE=${HAIYAJAN_MENU_LICENSE})
ADD_COMPILE_DEFINITIONS(NAME=${PROJECT_NAME})
ADD_COMPILE_DEFINITIONS(FULL_VERSION=${FULL_VERSION})
ADD_COMPILE_DEFINITIONS(VER_MAJOR=${haiyajan-menu_VERSION_MAJOR})
ADD_COMPILE_DEFINITIONS(VER_MINOR=${haiyajan-menu_VERSION_MINOR})
ADD_COMPILE_DEFINITIONS(VER_PATCH=${haiyajan-menu_VERSION_PATCH})
ADD_COMPILE_DEFINITIONS(VER_TWEAK=0)

# Collect source files and compress them
SET(SRC_ARCHIVE ${CMAKE_CURRENT_BINARY_DIR}/src.tar.xz)
FILE(GLOB PROJ_FILES LIST_DIRECTORIES false
        inc/*.h src/*.c LICENSE README.md CMakeLists.txt)
FILE(ARCHIVE_CREATE OUTPUT ${SRC_ARCHIVE}
        PATHS ${PROJ_FILES}
        FORMAT gnutar COMPRESSION XZ COMPRESSION_LEVEL 6)
FILE(READ ${SRC_ARCHIVE} filedata HEX)
STRING(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/src.c
        "/* This file is required for LGPLv3 compliance. */\n\n"
        "#include <stddef.h>\n\n"
        "const volatile unsigned char src[] = {${filedata}};\n"
        "const size_t src_sz = sizeof(src);\n")
TARGET_SOURCES(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src.c)

# Platform specific options
IF(MSVC)
    TARGET_SOURCES(${PROJECT_NAME} PRIVATE meta/winres.rc)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
            Winmm.lib Version.lib Setupapi.lib Imm32.lib)
    SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /W4)
ELSEIF(NINTENDO_SWITCH)
    # Fix linking with C++ libraries
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE stdc++)

    NX_GENERATE_NACP(${PROJECT_NAME}.nacp
            NAME "${PROJECT_NAME}"
            AUTHOR "Deltabeard"
            VERSION "${FULL_VERSION}")

    NX_CREATE_NRO(${PROJECT_NAME}
            NACP ${PROJECT_NAME}.nacp
            ICON "${PROJECT_SOURCE_DIR}/meta/icon.jpg")
ELSE()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE freetype m dl)
ENDIF()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "DEBUG")
endif()

IF(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR
		${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
	TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE -Wall -Wextra -std=c99 -pedantic)
	set(CMAKE_C_FLAGS_DEBUG "-Og -g3")
	set(CMAKE_C_FLAGS_MINSIZEREL "-flto -Os -s")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "-flto -O2 -g3")
	set(CMAKE_C_FLAGS_RELEASE "-flto -Ofast -g3")
ELSE()
	set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
ENDIF()

# Package options
INSTALL(TARGETS haiyajan-menu RUNTIME)

INCLUDE(CPack)
SET(CPACK_PACKAGE_VENDOR Deltabeard)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
SET(CPACK_PACKAGE_ICON meta/icon.ico)
SET(CPACK_RESOURCE_FILE_LICENSE LICENSE)

# The following macro was taken from the SDL2 project.
macro(message_bool_option _NAME _VALUE)
    if(${_VALUE})
        message(STATUS "  ${_NAME}: enabled")
    else()
        message(STATUS "  ${_NAME}: disabled")
    endif()
endmacro()

message(STATUS "Haiyajan-UI will build with the following options:")
message_bool_option("GNU FriBidi" USE_FRIBIDI)

MESSAGE(STATUS "  CC:      ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "  CFLAGS:  ${CMAKE_C_FLAGS}")
MESSAGE(STATUS "  LDFLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
