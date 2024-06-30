add_compile_definitions(_SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING)

include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/nes)
include_directories("${EmuWorld_SOURCE_DIR}/external/DSP-Cpp-filters/lib")

file(GLOB_RECURSE CORELIB_SRC
    ${EmuWorld_SOURCE_DIR}/include/nes/core/*.h
    ${EmuWorld_SOURCE_DIR}/src/nes/core/*.cpp
)

add_library(NesEmulator_Core OBJECT ${CORELIB_SRC})
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
target_link_libraries(NesEmulator_Core CommonUtils_Lib)
set_target_properties(NesEmulator_Core PROPERTIES FOLDER ${MAIN_FOLDER})
