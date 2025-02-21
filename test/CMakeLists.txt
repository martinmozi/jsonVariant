find_package(Catch2 REQUIRED)

add_executable(testSerialization testSerialization.cpp)
target_link_libraries(testSerialization Catch2::Catch2WithMain $<TARGET_OBJECTS:jsonVariantObj>)

add_executable(testDeserialization testDeserializationVeggie.cpp testDeserializationTeam.cpp)
target_link_libraries(testDeserialization Catch2::Catch2WithMain $<TARGET_OBJECTS:jsonVariantObj>)
