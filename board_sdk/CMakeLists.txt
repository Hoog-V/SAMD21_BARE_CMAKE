# Make an automatic library - will be static or dynamic based on user setting
add_library(board_sdk "gcc/system_samd21.c" "gcc/gcc/startup_samd21.c")
target_include_directories(board_sdk PUBLIC "cmsis-header-sam/samd21a/gcc/" "cmsis-header-sam/samd21a/gcc/gcc/" "CMSIS_5/CMSIS/Core/Include/" "cmsis-header-sam/samd21a/include")
