project(MyNNUEEngine LANGUAGES CXX)

# ----------------------------------------------------------------------------
# ПЕРЕКЛЮЧАТЕЛЬ: собирать NNUE-cpp сейчас?  OFF = оставляем на потом
# ----------------------------------------------------------------------------
option(WITH_NNUE "Compile built-in NNUE sources" OFF)

set(SRCS
    main.cpp
    bitboard.cpp
    position.cpp
    movegen.cpp
    search.cpp 
    zobrist.cpp 
)

if (WITH_NNUE)
    file(GLOB_RECURSE NNUE_CPP CONFIGURE_DEPENDS nnue/*.cpp nnue/*/*.cpp)
    list(APPEND SRCS ${NNUE_CPP})
endif()

add_executable(engine ${SRCS})

target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nnue)

if (MSVC)
    target_compile_options(engine PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(engine PRIVATE -O3 -mavx2 -march=native)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
