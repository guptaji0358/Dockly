# The Rust native layer (native/), included from scripts/CMakeLists.txt when DOCKLY_NATIVE is ON.
# Builds dockly_native.dll with cargo, puts it beside the other binaries and adds its tests.

find_program(DOCKLY_CARGO cargo REQUIRED)
get_filename_component(DN_CARGO_BIN ${DOCKLY_CARGO} DIRECTORY)

# cargo needs its own tools first on PATH. A self-contained toolchain (MSYS2) ships its linker next
# to cargo; name it, so that Qt's older gcc on PATH is not picked up to link a library built for another C runtime.
set(DN_ENV ${CMAKE_COMMAND} -E env --modify "PATH=path_list_prepend:${DN_CARGO_BIN}")
if(EXISTS ${DN_CARGO_BIN}/gcc.exe)
    list(APPEND DN_ENV "CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=${DN_CARGO_BIN}/gcc.exe")
endif()

set(DN_DIR    ${CMAKE_SOURCE_DIR}/native)
set(DN_TARGET ${CMAKE_BINARY_DIR}/native-target)
set(DN_DLL    ${CMAKE_BINARY_DIR}/dockly_native.dll)
set(DN_CARGO_ARGS --release --locked --manifest-path ${DN_DIR}/Cargo.toml --target-dir ${DN_TARGET})

# always runs: cargo itself decides what is out of date
add_custom_target(dockly_native
    COMMAND ${DN_ENV} -- ${DOCKLY_CARGO} build ${DN_CARGO_ARGS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DN_TARGET}/release/dockly_native.dll ${DN_DLL}
    BYPRODUCTS ${DN_DLL}
    COMMENT "Building the Rust native layer"
    VERBATIM)

target_sources(lhcore PRIVATE ${LH_INCLUDE}/nativebridge.h ${LH_SRC}/nativebridge.cpp)
target_include_directories(lhcore PUBLIC ${DN_DIR}/include)
target_compile_definitions(lhcore PUBLIC DOCKLY_NATIVE)
add_dependencies(lhcore dockly_native) # the DLL is loaded at run time, not linked

add_library(dn_badabi MODULE ${LH_MAIN}/native_badabi.c)
qt_add_executable(native_test ${LH_MAIN}/native_test.cpp)
target_link_libraries(native_test PRIVATE lhcore Qt6::Test)
target_compile_definitions(native_test PRIVATE DN_BADABI_PATH="$<TARGET_FILE:dn_badabi>")
add_dependencies(native_test dn_badabi)
add_test(NAME native_test COMMAND native_test)
add_test(NAME native_rust COMMAND ${DN_ENV} -- ${DOCKLY_CARGO} test ${DN_CARGO_ARGS})
