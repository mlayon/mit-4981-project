cmake_minimum_required(VERSION 3.18.2)
project(darcyproj C)

set(CMAKE_C_STANDARD 11)

add_executable(server server_v2.c config.c)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(server PRIVATE Threads::Threads)
target_compile_options(server PRIVATE -Wpedantic -Wall -Wextra)

add_executable(gui gui.c)
target_compile_options(gui PRIVATE -Wpedantic -Wall -Wextra -lform -lncurses)

find_package(Curses REQUIRED)
target_link_libraries(gui ${CURSES_LIBRARIES})
