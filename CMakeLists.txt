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
    GIT_TAG fastest
)
FetchContent_MakeAvailable(SFML CppAI)

add_executable(checkers src/UI_checkers.cpp)
target_sources(checkers
PRIVATE
    src/players/utils/node.cpp
    src/players/utils/netUtils.cpp
)
target_compile_features(checkers PRIVATE cxx_std_17)
target_link_libraries(checkers PRIVATE SFML::Graphics CppAI)
target_compile_options(checkers PRIVATE -O3)

add_executable(compete src/UI_competition.cpp)
target_sources(compete
PRIVATE
    src/players/utils/node.cpp
    src/players/utils/netUtils.cpp
    src/competition.cpp
)
target_compile_features(compete PRIVATE cxx_std_17)
target_link_libraries(compete PRIVATE SFML::Graphics CppAI)
target_compile_options(compete PRIVATE -O3)

add_executable(evolution src/evolution.cpp)
target_sources(evolution
PRIVATE
    src/players/utils/netUtils.cpp
)
target_compile_features(evolution PRIVATE cxx_std_17)
target_link_libraries(evolution PRIVATE CppAI)
target_compile_options(evolution PRIVATE -O3)

add_executable(basic_train src/basicNetTraining.cpp)
target_sources(basic_train
PRIVATE
    src/players/utils/netUtils.cpp
)
target_compile_features(basic_train PRIVATE cxx_std_17)
target_link_libraries(basic_train PRIVATE CppAI)
target_compile_options(basic_train PRIVATE -O3)

add_executable(train src/training.cpp)
target_sources(train
PRIVATE
    src/players/utils/node.cpp
    src/players/utils/netUtils.cpp
)
target_compile_features(train PRIVATE cxx_std_17)
target_link_libraries(train PRIVATE CppAI)
target_compile_options(train PRIVATE -O3)