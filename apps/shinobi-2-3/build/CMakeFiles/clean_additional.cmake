# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/shinobi_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/shinobi_autogen.dir/ParseCache.txt"
  "shinobi_autogen"
  )
endif()
