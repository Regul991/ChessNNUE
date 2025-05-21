cmake_minimum_required(VERSION 3.14)
project(MyNNUEEngine LANGUAGES CXX)

# ----------------------------------------------------------------------------
# ПЕРЕКЛЮЧАТЕЛЬ: собирать NNUE-cpp сейчас? OFF = оставляем на потом
# ----------------------------------------------------------------------------
option(WITH_NNUE "Compile built-in NNUE sources" OFF)

# Указываем поддиректорию движка
add_subdirectory(engine)