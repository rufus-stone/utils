message(STATUS "Building hamarr_tests...")

add_library(hmr_catch_main STATIC catch_main.cpp)

add_executable(hamarr_tests tests.cpp)
target_compile_features(hamarr_tests PRIVATE cxx_std_17)

target_link_libraries(hamarr_tests PRIVATE hmr_catch_main hamarr::hamarr)
