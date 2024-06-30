include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/nes)
include_directories(${EmuWorld_SOURCE_DIR}/external/libretro-common/include)
include_directories("${EmuWorld_SOURCE_DIR}/external/DSP-Cpp-filters/lib")


file(GLOB_RECURSE RETROCORELIB_SRC
    ${EmuWorld_SOURCE_DIR}/src/nes/libretro_core/*.cpp
)

add_library(NesEmulator_LibretroCore SHARED ${RETROCORELIB_SRC} $<TARGET_OBJECTS:NesEmulator_Core> $<TARGET_OBJECTS:CommonUtils_Lib>)
set_target_properties(NesEmulator_LibretroCore PROPERTIES FOLDER ${MAIN_FOLDER})