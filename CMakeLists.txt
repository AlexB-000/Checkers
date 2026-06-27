cmake_minimum_required(VERSION 3.28)
project(Checkers LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FETCHContent_Declare(CppAI
    GIT_REPOSITORY https://github.com/AlexB-000/Cpp_AI_lib.git
    GIT_TAG main
)
FetchContent_MakeAvailable(SFML CppAI)

add_executable(checkers src/UI_checkers.cpp)
target_sources(checkers
PRIVATE
    src/players/utils/node.cpp
)
target_compile_features(checkers PRIVATE cxx_std_17)
target_link_libraries(checkers PRIVATE SFML::Graphics CppAI)
target_compile_options(checkers PRIVATE -O3)

add_executable(compete src/UI_competition.cpp)
target_sources(compete
PRIVATE
    src/players/utils/node.cpp
    src/competition.cpp
)
target_compile_features(compete PRIVATE cxx_std_17)
target_link_libraries(compete PRIVATE SFML::Graphics CppAI)
target_compile_options(compete PRIVATE -O3)

add_executable(evolution src/evolution.cpp)
target_compile_features(evolution PRIVATE cxx_std_17)
target_link_libraries(evolution PRIVATE CppAI)
target_compile_options(evolution PRIVATE -O3)

add_executable(deep_learning src/deep_learning.cpp)
target_compile_features(deep_learning PRIVATE cxx_std_17)
target_link_libraries(deep_learning PRIVATE CppAI)
target_compile_options(deep_learning PRIVATE -O3)

add_executable(train_bootstrap src/training_bootstrapping.cpp)
target_sources(train_bootstrap
PRIVATE
    src/players/utils/node.cpp
)
target_compile_features(train_bootstrap PRIVATE cxx_std_17)
target_link_libraries(train_bootstrap PRIVATE CppAI)
target_compile_options(train_bootstrap PRIVATE -O3)

add_executable(train_raw src/training_rawresult.cpp)
target_sources(train_raw
PRIVATE
    src/players/utils/node.cpp
)
target_compile_features(train_raw PRIVATE cxx_std_17)
target_link_libraries(train_raw PRIVATE CppAI)
target_compile_options(train_raw PRIVATE -O3)