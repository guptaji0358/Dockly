# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "qml\\CMakeFiles\\DocklyApp_autogen.dir\\AutogenUsed.txt"
  "qml\\CMakeFiles\\DocklyApp_autogen.dir\\ParseCache.txt"
  "qml\\DocklyApp_autogen"
  "scripts\\CMakeFiles\\Dockly_autogen.dir\\AutogenUsed.txt"
  "scripts\\CMakeFiles\\Dockly_autogen.dir\\ParseCache.txt"
  "scripts\\CMakeFiles\\core_test_autogen.dir\\AutogenUsed.txt"
  "scripts\\CMakeFiles\\core_test_autogen.dir\\ParseCache.txt"
  "scripts\\CMakeFiles\\guard3_test_autogen.dir\\AutogenUsed.txt"
  "scripts\\CMakeFiles\\guard3_test_autogen.dir\\ParseCache.txt"
  "scripts\\CMakeFiles\\guard_test_autogen.dir\\AutogenUsed.txt"
  "scripts\\CMakeFiles\\guard_test_autogen.dir\\ParseCache.txt"
  "scripts\\CMakeFiles\\lhcore_autogen.dir\\AutogenUsed.txt"
  "scripts\\CMakeFiles\\lhcore_autogen.dir\\ParseCache.txt"
  "scripts\\Dockly_autogen"
  "scripts\\core_test_autogen"
  "scripts\\guard3_test_autogen"
  "scripts\\guard_test_autogen"
  "scripts\\lhcore_autogen"
  )
endif()
