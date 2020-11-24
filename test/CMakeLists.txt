enable_testing()
find_package(GTest REQUIRED)

add_executable(testSerialization testSerialization.cpp)
target_link_libraries(testSerialization GTest::GTest GTest::Main)

add_test(JsonVariantTests testSerialization)
