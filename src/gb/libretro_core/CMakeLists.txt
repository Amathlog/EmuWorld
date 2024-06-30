include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/gb)
include_directories(${EmuWorld_SOURCE_DIR}/external/libretro-common/include)


file(GLOB_RECURSE GBRETROCORELIB_SRC
    ${EmuWorld_SOURCE_DIR}/src/gb/libretro_core/*.cpp
)

add_library(GBEmulator_LibretroCore SHARED ${GBRETROCORELIB_SRC} $<TARGET_OBJECTS:CommonUtils_Lib>)
target_link_libraries(GBEmulator_LibretroCore GBEmulator_Core)
set_target_properties(GBEmulator_LibretroCore PROPERTIES FOLDER ${MAIN_FOLDER})