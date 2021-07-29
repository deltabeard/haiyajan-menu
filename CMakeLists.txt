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

target_sources(${PROJECT_NAME} PRIVATE src/main.c src/font.c src/ui.c)
target_include_directories(${PROJECT_NAME} PRIVATE inc)

# Discover libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/cmake")

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(fribidi QUIET)

#if (NOT SDL_VERSION_MAJOR EQUAL 2)
#	message(SEND_ERROR "Only SDL2 is supported; Found SDL${SDL_VERSION_MAJOR}")
#endif()

# Add required dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::Core SDL2::Main)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::TTF)
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL_TTF_INCLUDE_DIRS})

# Add options to configure optional dependencies
option(USE_FRIBIDI "Use Fribidi library for bidirection text support"
        ${FRIBIDI_FOUND})

# Process optional dependencies
if(USE_FRIBIDI AND NOT FRIBIDI_FOUND)
    message(SEND_ERROR "Fribidi enabled, but not found")
elseif(USE_FRIBIDI)
    message(STATUS "Enabled: GNU FriBidi")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FRIBIDI_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${FRIBIDI_INCLUDE_DIRS})
else()
    message(STATUS "Disabled: GNU FriBidi")
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

# Package options
include(GNUInstallDirs)
install(TARGETS haiyajan-menu RUNTIME)

include(CPack)
set(CPACK_PACKAGE_VENDOR Deltabeard)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_ICON meta/icon.ico)
set(CPACK_RESOURCE_FILE_LICENSE LICENSE)
