add_compile_definitions(_SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING)

include_directories(${EmuWorld_SOURCE_DIR}/include)

file(GLOB_RECURSE COMMONUTILSLIB_SRC
    ${EmuWorld_SOURCE_DIR}/include/common/utils/*.h
    ${EmuWorld_SOURCE_DIR}/src/common/utils/*.cpp
)

add_library(CommonUtils_Lib OBJECT ${COMMONUTILSLIB_SRC})
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
target_link_libraries(CommonUtils_Lib)
set_target_properties(CommonUtils_Lib PROPERTIES FOLDER ${MAIN_FOLDER})