# Make an automatic library - will be static or dynamic based on user setting
add_library(board_sdk "samd21a/gcc/system_samd21.c" "samd21a/gcc/gcc/startup_samd21.c")
target_include_directories(board_sdk PUBLIC "samd21a/gcc/" "samd21a/gcc/gcc/" "CMSIS/Core/Include/" "samd21a/include")
