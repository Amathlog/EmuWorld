cmake_minimum_required(VERSION 3.12)

project(EmuWorld)

# Enable c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
  add_compile_definitions("_GLIBCXX_DEBUG")
endif()

set(EXTERNAL_FOLDER "External")
set(MAIN_FOLDER "EmuWorld")

if(MSVC)
  add_definitions(/MP)
endif()

# set(CMAKE_CXX_FLAGS "-Wall")
if(WIN32)
set(CMAKE_CXX_FLAGS_DEBUG "/ZI /Od")
set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else(WIN32)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif(WIN32)

# Include RTAudio
if(UNIX)
    set(RTAUDIO_API_PULSE ON)
    set(RTAUDIO_API_WASAPI OFF)
endif(UNIX)
if(WIN32)
   set(RTAUDIO_API_WASAPI ON)
   set(RTAUDIO_API_PULSE OFF)
endif(WIN32)
set(RTAUDIO_BUILD_TESTING OFF)
add_subdirectory("${EmuWorld_SOURCE_DIR}/external/rtaudio" rtaudio)
include_directories("${EmuWorld_SOURCE_DIR}/external/rtaudio")
set_target_properties(rtaudio PROPERTIES FOLDER ${EXTERNAL_FOLDER})
if(WIN32)
  target_compile_options(rtaudio PRIVATE /wd4251 /wd4996 /wd4267)
endif(WIN32)

# Include OpenGL
if(UNIX)
    set(OpenGL_GL_PREFERENCE "GLVND")
endif(UNIX)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# Include GLFW and GLAD
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_DIR "${EmuWorld_SOURCE_DIR}/external/glfw")
set(GLAD_DIR "${EmuWorld_SOURCE_DIR}/external/glad")
add_subdirectory("${GLFW_DIR}" glfw)
include_directories("${GLFW_DIR}/include")
include_directories("${GLAD_DIR}/include")

# Build ImGui
set(IMGUI_DIR "${EmuWorld_SOURCE_DIR}/external/imgui")
include_directories("${IMGUI_DIR}")
include_directories("${IMGUI_DIR}/backends")

file (GLOB IMGUI_SRC
    "${IMGUI_DIR}/*cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

add_library(ImGui STATIC ${IMGUI_SRC})
set_target_properties(ImGui PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Build ImGuiFileBrowser
set(IMGUI_FE_DIR "${EmuWorld_SOURCE_DIR}/external/ImGui-Addons/FileBrowser")
include_directories("${IMGUI_FE_DIR}")
file (GLOB IMGUI_FE_SRC
    "${IMGUI_FE_DIR}/*.cpp"
)

add_library(ImGuiFE STATIC ${IMGUI_FE_SRC})
set_target_properties(ImGuiFE PROPERTIES FOLDER ${EXTERNAL_FOLDER})
target_link_libraries(ImGuiFE ImGui)
if(WIN32)
  target_compile_options(ImGuiFE PRIVATE /wd4267 /wd4996 /wd4244)
endif(WIN32)

option(BUILD_TESTS "Build the tests" ON)

add_subdirectory("src")

if(UNIX)
  add_subdirectory("external/rgbds")
endif(UNIX)

# if (BUILD_TESTS)
    add_subdirectory("tests")
# endif(BUILD_TESTS)
