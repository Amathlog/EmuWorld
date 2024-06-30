include_directories(${EmuWorld_SOURCE_DIR}/include)
include_directories(${EmuWorld_SOURCE_DIR}/include/gb)
include_directories(${EmuWorld_SOURCE_DIR}/tests)

file(GLOB_RECURSE TESTS_SRC
    "${EmuWorld_SOURCE_DIR}/tests/gb/*.cpp"
)

add_executable(GBEmulatorUnitTests ${TESTS_SRC})
target_link_libraries(GBEmulatorUnitTests GBEmulator_Core CommonUtils_Lib)
target_link_libraries(GBEmulatorUnitTests gtest_main)
set_target_properties(GBEmulatorUnitTests PROPERTIES FOLDER ${MAIN_FOLDER})

#include(GoogleTest)
#gtest_discover_tests(NesEmulatorUnitTests)