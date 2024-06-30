include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/nes)
include_directories("${EmuWorld_SOURCE_DIR}/external/DSP-Cpp-filters/lib")

file(GLOB_RECURSE NES_FRONTEND_SRC
    "${EmuWorld_SOURCE_DIR}/src/nes/frontend/*.cpp"
    "${EmuWorld_SOURCE_DIR}/include/nes/frontend/*.h"
    "${GLAD_DIR}/src/glad.c"
)

add_executable(NesEmulator ${NES_FRONTEND_SRC})
target_link_libraries(NesEmulator CommonUtils_Lib NesEmulator_Core ${OPENGL_LIBRARIES} glfw ${CMAKE_DL_LIBS} rtaudio ImGuiFE ImGui -static-libgcc -static-libstdc++)
set_target_properties(NesEmulator PROPERTIES FOLDER ${MAIN_FOLDER})