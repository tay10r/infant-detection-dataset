cmake_minimum_required(VERSION 3.14.7)

project(renderer)

find_package(OpenMP REQUIRED COMPONENTS CXX)

add_library(stb
  src/stb/stb_image_write.h
  src/stb/stb_image_write.c
  src/stb/stb_image.h
  src/stb/stb_image.c)
target_include_directories(stb PUBLIC src/stb)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(bvh
  URL "https://github.com/madmann91/bvh/archive/fbdc59615112d471409515cd2ab826fcc3100fd7.zip"
  URL_HASH "SHA256=123eeccb045aa385819ec562ed168e73dfba4b95741d4119bd9ff7917f33ebcf")
FetchContent_MakeAvailable(bvh)

find_package(GTest CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

add_library(renderer
  src/exceptions.h
  src/stl.h
  src/stl.cpp
  src/obj_class.h
  src/obj_model.h
  src/obj_parser.h
  src/obj_parser.cpp
  src/obj_lexer.h
  src/obj_lexer.cpp
  src/generator.h
  src/generator.cpp
  src/scene.h
  src/scene.cpp
  src/renderer.h
  src/renderer.cpp)
target_link_libraries(renderer
  PUBLIC
    stb
    bvh
    Eigen3::Eigen
    OpenMP::OpenMP_CXX)
target_compile_features(renderer PUBLIC cxx_std_20)

add_executable(run_tests
  src/obj_lexer_test.cpp
  src/obj_parser_test.cpp)
target_link_libraries(run_tests
  PUBLIC
    GTest::gtest
    GTest::gtest_main
    renderer)

add_executable(main
  src/main.cpp)
target_link_libraries(main
  PUBLIC
    renderer
    stb)
target_compile_definitions(main
  PUBLIC
    "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}/\"")
