cmake_minimum_required(VERSION 3.19)
project(haiyajan_menu
        LANGUAGES C
        VERSION 0.0.1
        DESCRIPTION "User interface library for Haiyajan"
        HOMEPAGE_URL "https://github.com/deltabeard/haiyajan-menu")

set(CMAKE_C_STANDARD 99)

add_executable(${PROJECT_NAME})
find_package(SDL2 REQUIRED)
find_library(SDL2_TTF_LIBRARY SDL2_ttf REQUIRED)
find_library(FRIBIDI_LIBRARY fribidi)

target_sources(${PROJECT_NAME} PRIVATE src/main.c src/font.c src/ui.c)
target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_TTF_LIBRARY})

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
