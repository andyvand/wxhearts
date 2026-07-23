# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/wxhearts_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/wxhearts_autogen.dir/ParseCache.txt"
  "wxhearts_autogen"
  )
endif()
