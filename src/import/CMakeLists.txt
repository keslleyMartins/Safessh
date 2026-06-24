add_library(safessh_import STATIC
    TermiusImporter.cpp
    SSHConfigImporter.cpp
)

target_include_directories(safessh_import PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(safessh_import PUBLIC
    safessh_core
    Qt6::Core
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

safessh_set_warnings(safessh_import)
