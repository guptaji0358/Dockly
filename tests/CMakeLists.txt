find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(core_test core_test.cpp)
target_link_libraries(core_test PRIVATE lhcore Qt6::Test)
add_test(NAME core_test COMMAND core_test)
