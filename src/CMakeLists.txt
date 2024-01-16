add_executable(ratings main.cpp ratings.cpp threads/threads.cpp)

target_compile_options(ratings PRIVATE -march=native -Wall)
set_property(TARGET ratings PROPERTY CXX_STANDARD 20)
target_link_libraries(ratings PRIVATE absl::flat_hash_map)

add_executable(absl_test absl.cpp)
target_compile_options(absl_test PRIVATE -march=native -Wall)
target_link_libraries(absl_test PRIVATE absl::flat_hash_map)
