include_directories(${EmuWorld_SOURCE_DIR}/include/core)

file(GLOB_RECURSE TESTS_SRC
    "${EmuWorld_SOURCE_DIR}/tests/nes/*.cpp"
)

add_executable(NesEmulatorUnitTests ${TESTS_SRC})
target_link_libraries(NesEmulatorUnitTests NesEmulator_Core CommonUtils_Lib)
target_link_libraries(NesEmulatorUnitTests gtest_main)
set_target_properties(NesEmulatorUnitTests PROPERTIES FOLDER ${MAIN_FOLDER})

#include(GoogleTest)
#gtest_discover_tests(NesEmulatorUnitTests)