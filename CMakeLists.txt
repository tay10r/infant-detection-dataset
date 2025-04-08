cmake_minimum_required(VERSION 3.14.7)

project(cradle)

option(CRADLE_TESTS "Whether or not to build the unit tests." OFF)

find_package(OpenMP REQUIRED COMPONENTS CXX)
find_package(nlohmann_json CONFIG REQUIRED)

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

add_library(renderer
  include/cradle/exceptions.h
  include/cradle/renderer.h
  include/cradle/obj_model.h
  include/cradle/generator.h
  src/renderer.cpp
  src/obj_model.cpp
  src/room_generator.h
  src/room_generator.cpp
  src/cabinet_generator.h
  src/cabinet_generator.cpp

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
  src/path_tracer.h
  src/path_tracer.cpp)
target_include_directories(renderer
  PUBLIC
    include)
target_link_libraries(renderer
  PUBLIC
    stb
    bvh
    nlohmann_json::nlohmann_json
    OpenMP::OpenMP_CXX)
target_compile_features(renderer PUBLIC cxx_std_20)

if(CRADLE_TESTS)
  find_package(GTest CONFIG REQUIRED)

  add_executable(run_tests
    src/obj_lexer_test.cpp
    src/obj_parser_test.cpp
    src/renderer_tests.cpp)
  target_link_libraries(run_tests
    PUBLIC
      GTest::gtest
      GTest::gtest_main
      renderer)
endif()

add_executable(main
  src/main.cpp)
target_link_libraries(main
  PUBLIC
    renderer
    stb)
target_compile_definitions(main
  PUBLIC
    "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}/\"")
