function(safessh_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor
            -Wold-style-cast -Wcast-align
            -Woverloaded-virtual -Wconversion
            -Wsign-conversion -Wnull-dereference
            -Wformat=2
        )
    endif()
endfunction()
