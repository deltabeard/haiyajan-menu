cmake_minimum_required(VERSION 3.19)

# Obtain the version of Haiyajan-menu that is being built.
# If git is not available on the build platform, use this as a fallback.
set(BUILD_VERSION 0.1.0)
find_package(Git)
if(GIT_FOUND)
	execute_process(
	    COMMAND ${GIT_EXECUTABLE} describe --tags --long --always --dirty=-dirty
	    WORKING_DIRECTORY "${local_dir}"
	    OUTPUT_VARIABLE GIT_VER
	    ERROR_QUIET
	    OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "Configuring version ${GIT_VER}")

	execute_process(
	    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
	    WORKING_DIRECTORY "${local_dir}"
	    OUTPUT_VARIABLE BUILD_VERSION
	    ERROR_QUIET
	    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	set(GIT_VER ${BUILD_VERSION}-LOCAL)
endif()

message(STATUS "Using build version: ${BUILD_VERSION}")
message(STATUS "Building commit: ${GIT_VER}")

# Initialise project information.
project(haiyajan-menu
        LANGUAGES C
        VERSION ${BUILD_VERSION}
        DESCRIPTION "User interface library for Haiyajan"
        HOMEPAGE_URL "https://github.com/deltabeard/haiyajan-menu")

set(CMAKE_C_STANDARD 99)
set(HAIYAJAN_MENU_LICENSE "LGPL-3.0, Mahyar Koshkouei (c) 2021")

# Add dependencies to project.
add_executable(${PROJECT_NAME})
find_package(SDL2 REQUIRED)
find_library(SDL2_TTF_LIBRARY SDL2_ttf REQUIRED)
find_library(FRIBIDI_LIBRARY fribidi)

target_sources(${PROJECT_NAME} PRIVATE src/main.c src/font.c src/ui.c)
target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_TTF_LIBRARY})

# If libfribidi is not available, disable its use.
# This option can be forced by the user in cmake-gui.
if(FRIBIDI_LIBRARY)
    set(FRIBIDI_LIBRARY_ENABLED_BY_DEFAULT ON)
else()
    set(FRIBIDI_LIBRARY_ENABLED_BY_DEFAULT OFF)
endif()

option(USE_FRIBIDI "Use Fribidi library for bidirection text support"
        ${FRIBIDI_LIBRARY_ENABLED_BY_DEFAULT})

if(USE_FRIBIDI)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FRIBIDI_LIBRARY})
else()
    add_compile_definitions(NO_FRIBIDI)
endif()

# Compile resources for Windows executable.
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE meta/winres.rc)
endif()

# Add definitions of project information.
add_compile_definitions(COMPANY=Deltabeard)
add_compile_definitions(DESCRIPTION=${PROJECT_DESCRIPTION})
add_compile_definitions(LICENSE=${HAIYAJAN_MENU_LICENSE})
add_compile_definitions(NAME=${PROJECT_NAME})
add_compile_definitions(GIT_VER=${GIT_VER})
add_compile_definitions(VER_MAJOR=${haiyajan-menu_VERSION_MAJOR})
add_compile_definitions(VER_MINOR=${haiyajan-menu_VERSION_MINOR})
add_compile_definitions(VER_PATCH=${haiyajan-menu_VERSION_PATCH})
add_compile_definitions(VER_TWEAK=0)

include(CPack)
