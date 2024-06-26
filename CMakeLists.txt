cmake_minimum_required(VERSION 3.0.0)
project(Jello-Sim VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ../bin)
    add_subdirectory(glfw)
endif()

include(CTest)
enable_testing()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
if(NOT WIN32)
    find_package(glfw3 REQUIRED)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(Jello-Sim ${PROJECT_SOURCE_DIR}/src/glad.c ${PROJECT_SOURCE_DIR}/src/main.cpp ${PROJECT_SOURCE_DIR}/src/simulator.cpp ${PROJECT_SOURCE_DIR}/src/utils.cpp)
target_link_libraries(Jello-Sim ${OPENGL_LIBRARY} glfw ${CMAKE_DL_LIBS})

set(EXECUTABLE_OUTPUT_PATH ../bin)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)