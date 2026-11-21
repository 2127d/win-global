# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/render-vines_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/render-vines_autogen.dir/ParseCache.txt"
  "CMakeFiles/sequoiavines5_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/sequoiavines5_autogen.dir/ParseCache.txt"
  "CMakeFiles/test-vines-motion_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/test-vines-motion_autogen.dir/ParseCache.txt"
  "render-vines_autogen"
  "sequoiavines5_autogen"
  "test-vines-motion_autogen"
  )
endif()
