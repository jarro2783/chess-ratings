enable_language(CUDA)

add_executable(ratings main.cpp ratings.cpp threads/threads.cpp ratings_cuda.cu
ratings_cuda_impl.cpp)

target_compile_options(ratings PRIVATE -march=native -Wall)
set_property(TARGET ratings PROPERTY CXX_STANDARD 20)
target_link_libraries(ratings PRIVATE absl::flat_hash_map stdc++)
