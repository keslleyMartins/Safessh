add_subdirectory(core)
add_subdirectory(crypto)
add_subdirectory(ui)
add_subdirectory(import)

add_executable(safessh main.cpp)

target_link_libraries(safessh PRIVATE
    safessh_core
    safessh_crypto
    safessh_ui
    safessh_import
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
)

safessh_set_warnings(safessh)
