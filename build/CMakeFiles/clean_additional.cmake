# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\ItemShopFinder_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ItemShopFinder_autogen.dir\\ParseCache.txt"
  "ItemShopFinder_autogen"
  )
endif()
