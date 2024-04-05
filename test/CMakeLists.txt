enable_testing()
find_package(GTest REQUIRED)

add_executable(testSerialization testSerialization.cpp)
target_link_libraries(testSerialization GTest::GTest GTest::Main)

add_executable(testDeserialization testDeserialization.cpp)
target_link_libraries(testDeserialization GTest::GTest GTest::Main)

add_executable(testJsonSchema testJsonSchema.cpp)
target_link_libraries(testJsonSchema GTest::GTest GTest::Main)

add_test(JsonVariantTests testSerialization)
add_test(JsonVariantTests testDeserialization)
add_test(JsonVariantTests testJsonSchema)
