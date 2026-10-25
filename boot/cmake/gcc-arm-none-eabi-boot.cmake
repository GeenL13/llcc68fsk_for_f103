# Reuse the project's GCC/linker selection; keep Boot's Debug image within 12 KiB.
include("${CMAKE_CURRENT_LIST_DIR}/../../upgrade/gcc-arm-none-eabi-ab.cmake")
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3")
