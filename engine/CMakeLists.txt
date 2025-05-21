# CMakeLists.txt — внутри папки engine/
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

# Добавить директорию nnue в инклуды (если она существует)
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nnue)

# Оптимизации
if (MSVC)
    target_compile_options(engine PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(engine PRIVATE -O3 -mavx2 -march=native)
endif()

set_target_properties(engine PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)
