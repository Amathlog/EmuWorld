# Add GTest
set(BUILD_GMOCK OFF)
add_subdirectory(${EmuWorld_SOURCE_DIR}/external/googletest build)
#set_target_properties(gtest gtest_main uninstall PROPERTIES FOLDER ${EXTERNAL_FOLDER}/gtest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_subdirectory(gb)
add_subdirectory(nes)