include(FetchContent)
FetchContent_Declare(clap
    GIT_REPOSITORY https://github.com/lucaspujol/clap.git
    GIT_TAG main)
FetchContent_MakeAvailable(clap)

FetchContent_Declare(stu
    GIT_REPOSITORY https://github.com/louisbgl/stu-cpp.git
    GIT_TAG main)
FetchContent_MakeAvailable(stu)

set(BENCHMARKS bench_speed bench_memory bench_vs_handcrafted)

foreach(BENCH ${BENCHMARKS})
    add_executable(${BENCH} ${BENCH}.cpp)
    target_link_libraries(${BENCH} PRIVATE cord clap stu)
endforeach()
