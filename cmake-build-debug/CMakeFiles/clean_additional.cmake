# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\myPython_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\myPython_autogen.dir\\ParseCache.txt"
  "myPython_autogen"
  )
endif()
