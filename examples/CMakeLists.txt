add_executable(Serialization serialization.cpp)
install(TARGETS Serialization DESTINATION bin)

add_executable(Deserialization deserialization.cpp)
install(TARGETS Deserialization DESTINATION bin)
