cmake_minimum_required(VERSION 3.10)

project(Reversi)

# Переменные
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/include/glad)
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/include/GLFW)
set(STB_DIR ${CMAKE_SOURCE_DIR}/include/stb)
set(NUCKLEAR_DIR ${CMAKE_SOURCE_DIR}/include/nucklear)
set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include/user_headers)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)


# .c файлы
add_executable(Reversi 
    ${SRC_DIR}/glad.c
    ${SRC_DIR}/main.c
    ${SRC_DIR}/stack.c
    ${SRC_DIR}/text_render.c
)


# .h файлы
include_directories(
    ${GLAD_DIR}
    ${GLFW_DIR} 
    ${STB_DIR}
    ${NUCKLEAR_DIR}
    ${HEADERS_DIR}

)


# Линкуем библиотеки
target_link_libraries(Reversi
    ${LIBS_DIR}/libglfw3.a
)


# Где будет exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)