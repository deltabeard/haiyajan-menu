cmake_minimum_required(VERSION 3.19)
project(haiyajan_menu C)

set(CMAKE_C_STANDARD 99)

add_executable(${PROJECT_NAME})
find_package(SDL2 REQUIRED)

include_directories(/usr/include/fribidi)

target_sources(${PROJECT_NAME} PRIVATE src/main.c src/font.c src/ui.c)
target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_link_libraries(${PROJECT_NAME} SDL2::SDL2 SDL2_ttf fribidi)
