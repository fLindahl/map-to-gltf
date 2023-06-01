# map-to-gltf

CLI tool for converting Valve format .map files to .gltf/glb

## How to use

Run `$ mtg -h` for instructions.

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

Missing features:
- No support for WAD files	
- No support for CSG union between polygons *yet*. It does however merge polygons within a single entity based on their materials.
- Untested on Linux
- No support for embedding textures in glb/gltf. Currently only uses external resources.