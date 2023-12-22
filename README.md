# map-to-gltf

CLI tool for converting Valve format .map files to .gltf/glb

## How to use

Run `$ mtg -h` or check [here](https://github.com/fLindahl/map-to-gltf/blob/01745cf5a7f1741f1c53ec82a09d2cad9b67624b/code/main.cpp#L14) for instructions.

## Build prerequisities

1. CMake
2. Compiler of choice

## How to build

1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `cmake --build . --config Release`

## Limitations

Currently only parses and works with Valves "220" format.

This tool has been built and tested with Trenchbroom-produced files.

Pull-request are warmly welcome!

Missing features:
- No support for WAD files	
- No support for CSG union between polygons *yet*. It does however merge polygons within a single entity based on their materials.
- Untested on Linux
