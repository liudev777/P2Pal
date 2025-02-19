# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/P2Pal_Devin_Liu_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/P2Pal_Devin_Liu_autogen.dir/ParseCache.txt"
  "P2Pal_Devin_Liu_autogen"
  )
endif()
