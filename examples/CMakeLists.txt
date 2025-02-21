add_executable(Serialization serialization.cpp)
target_link_libraries(Serialization $<TARGET_OBJECTS:jsonVariantObj>)
install(TARGETS Serialization DESTINATION bin)

add_executable(Deserialization deserialization.cpp)
target_link_libraries(Deserialization $<TARGET_OBJECTS:jsonVariantObj>)
install(TARGETS Deserialization DESTINATION bin)
