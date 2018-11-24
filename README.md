jsonVariant is a C++ header-only library for deserializing JSON strings to the recursive variants and serializing recursive variants to JSON strings

# Installation

Copy the header file into your project or use cmake for installing header to system folders

# Prerequisities

The library is uses rapidjson (https://github.com/Tencent/rapidjson) as a default backend. Alternatively it's also possible to use jsoncpp https://github.com/open-source-parsers/jsoncpp.
The json schema validation is supported only with rapidjson backend

# Build & Install

* `cmake -DCMAKE_BUILD_TYPE=Release -DRAPIDJSON_BACKEND=ON -DBUILD_EXAMPLES=ON .` - Replace _Release_ with _Debug_ for a build with debug symbols.
* `cmake --build . --target install` - use sudo on Linux for installing to system directories
