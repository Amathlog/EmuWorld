add_subdirectory(core)
add_subdirectory(libretro_core)

option(NESEMULATOR_ONLY_CORE "Build only the core lib" OFF)

if (NOT NESEMULATOR_ONLY_CORE)
    add_subdirectory(frontend)
endif(NOT NESEMULATOR_ONLY_CORE)