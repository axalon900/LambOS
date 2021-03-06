if (NOT DEFINED LIBC_TARGET)
    set(LIBC_TARGET lambos-libc)
    message (WARNING "LIBC_TARGET was not set by project root. Defaulting to '${LIBC_TARGET}'.")
    set(LIBC_TARGET ${LIBC_TARGET} PARENT_SCOPE)
endif ()
if (NOT DEFINED LIBK_TARGET)
    set(LIBK_TARGET lambos-libk)
    message (WARNING "LIBK_TARGET was not set by project root. Defaulting to '${LIBK_TARGET}'.")
    set(LIBK_TARGET ${LIBK_TARGET} PARENT_SCOPE)
endif ()

set (STDIO_SOURCES
        src/stdio/gets.c
        src/stdio/printf.c
        )

set (STDLIB_SOURCES
        src/stdlib/exit.c
        src/stdlib/liballoc.c
        src/stdlib/itoa.c
        src/stdlib/liballoc_hook.cpp
        )

set (STRING_SOURCES
        src/string/memset.cpp
        src/string/string.c
        src/string/strtok.c
        )

set (SOURCES
    ${STDIO_SOURCES}
    ${STDLIB_SOURCES}
    ${STRING_SOURCES}
    src/ctype.c
    src/sys/syscall.cpp
    src/sys/syscall_kernel.cpp
    src/lang_support.cpp)

macro(add_libc TARGET_NAME IS_KERNEL)
    add_library(${TARGET_NAME} STATIC ${SOURCES})
    target_link_libraries(${TARGET_NAME} gcc)
    target_include_directories(${TARGET_NAME} PUBLIC include)
    target_include_directories(${TARGET_NAME} PRIVATE src syscall/include)
    set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD ${LAMBOS_CXX_STANDARD})
    set(LIBC_C_COMPILER_FLAGS ${C_COMPILER_FLAGS} -Wno-unused-function)
    set(LIBC_CXX_COMPILER_FLAGS ${CXX_COMPILER_FLAGS} -Wno-unused-function)
    if (${IS_KERNEL})
        message("Generating libc target '${TARGET_NAME}' for kernel target")
        set(LIBC_C_COMPILER_FLAGS ${LIBC_C_COMPILER_FLAGS} -DKERNEL)
        set(LIBC_CXX_COMPILER_FLAGS ${LIBC_CXX_COMPILER_FLAGS} -DKERNEL)
        target_include_directories(${TARGET_NAME} PRIVATE $<TARGET_PROPERTY:${KERNEL_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
    else()
        message("Generating libc target '${TARGET_NAME}' for non-kernel targets")
    endif()
    target_compile_options(${TARGET_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:${LIBC_C_COMPILER_FLAGS}>
            $<$<COMPILE_LANGUAGE:CXX>:${LIBC_CXX_COMPILER_FLAGS}>)
endmacro()

add_libc(${LIBC_TARGET} NO)
add_libc(${LIBK_TARGET} YES)
