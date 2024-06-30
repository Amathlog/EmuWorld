add_subdirectory(core)
add_subdirectory(libretro_core)

option(GBEMULATOR_ONLY_CORE "Build only the core lib" OFF)

if (NOT GBEMULATOR_ONLY_CORE)
    add_subdirectory(frontend)
endif()
