add_library(safessh_crypto STATIC
    Vault.cpp
    KeyDerivation.cpp
)

target_include_directories(safessh_crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(safessh_crypto PUBLIC
    Qt6::Core
    libsodium
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

safessh_set_warnings(safessh_crypto)
