# Tests placeholder
# Add test executables here

add_executable(test_utils
    unit/test_url_utils.cpp
    unit/test_hash_utils.cpp
)

target_link_libraries(test_utils
    PRIVATE
    ${PROJECT_NAME}
)

add_test(NAME test_utils COMMAND test_utils)
