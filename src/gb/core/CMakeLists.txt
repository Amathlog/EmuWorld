add_compile_definitions(_SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING)

include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/gb)

file(GLOB_RECURSE GBCORELIB_SRC
    ${EmuWorld_SOURCE_DIR}/include/gb/core/*.h
    ${EmuWorld_SOURCE_DIR}/src/gb/core/*.cpp
)

add_library(GBEmulator_Core OBJECT ${GBCORELIB_SRC})

if (UNIX)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
endif(UNIX)

target_link_libraries(GBEmulator_Core CommonUtils_Lib)
set_target_properties(GBEmulator_Core PROPERTIES FOLDER ${MAIN_FOLDER})