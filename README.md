A header only library to deserialization json string to the recursive variant and serialization recursive variant to json string

# Installation

Copy directly the header into your project or use cmake for complete instalation

# Prerequisities

The library is using the rapidjson (https://github.com/Tencent/rapidjson) as a default backend. Alternatively it's possible to use also the jsoncpp https://github.com/open-source-parsers/jsoncpp instead
but the json schema validation is supported only with rapidjson backend

# Build & Install

* `cmake -DCMAKE_BUILD_TYPE=Release -DRAPIDJSON_BACKEND=ON -DBUILD_EXAMPLES=ON .` - Replace _Release_ with _Debug_ for a build with debug symbols.
* `cmake --build . --target install` - use sudo on Linux for installing to system directories
