cmake_minimum_required(VERSION 3.10)

project(Reversi)

# Переменные
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/include/glad)
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/include/GLFW)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(CGLM_DIR ${CMAKE_SOURCE_DIR}/include/cglm/)


# .c файлы
add_executable(Reversi 
    ${SRC_DIR}/glad.c
    ${SRC_DIR}/main.c
)

add_library(cglm STATIC
    ${CGLM_DIR}/src/affine.c
    ${CGLM_DIR}/src/io.c
    ${CGLM_DIR}/src/mat4.c
)

# .h файлы
include_directories(
    ${GLAD_DIR}
    ${GLFW_DIR} 
    ${CGLM_DIR}/include
)

# Линкуем библиотеки
target_link_libraries(Reversi
    ${LIBS_DIR}/libglfw3.a
)

target_include_directories(cglm PUBLIC
    ${CGLM_DIR}/include
)

# Где будет exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)