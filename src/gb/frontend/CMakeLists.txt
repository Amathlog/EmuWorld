include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/gb)

file(GLOB_RECURSE GB_FRONTEND_SRC
    "${EmuWorld_SOURCE_DIR}/src/gb/frontend/*.cpp"
    "${EmuWorld_SOURCE_DIR}/include/gb/frontend/*.h"
    "${GLAD_DIR}/src/glad.c"
)

add_executable(GBEmulator ${GB_FRONTEND_SRC})
target_link_libraries(GBEmulator CommonUtils_Lib GBEmulator_Core ${OPENGL_LIBRARIES} glfw ${CMAKE_DL_LIBS} rtaudio ImGuiFE ImGui)
set_target_properties(GBEmulator PROPERTIES FOLDER ${MAIN_FOLDER})
