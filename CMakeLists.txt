PROJECT(map-to-gltf)
CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)
IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
	
ENDIF()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

SET(files_map_to_gltf 
	flags.h
	main.cpp
)

SET(files_exts_map_files
	exts/map-files/brush.cpp
	exts/map-files/brush.h
	exts/map-files/entity.cpp
	exts/map-files/entity.h
	exts/map-files/face.cpp
	exts/map-files/map.cpp
	exts/map-files/map.h
	exts/map-files/math.h
	exts/map-files/poly.cpp
	exts/map-files/property.cpp
	exts/map-files/texture.cpp
	exts/map-files/WAD3.cpp
	exts/map-files/WAD3.h
)

SET(files_exts_fx
	exts/fx/gltf.h
	exts/fx/json.hpp
)

SET(files_exts_stb
	exts/stb/stbimage.h
)

SOURCE_GROUP("mtg" FILES ${files_map_to_gltf})
SOURCE_GROUP("map-files" FILES ${files_exts_map_files})
SOURCE_GROUP("fx" FILES ${files_exts_fx})
SOURCE_GROUP("stb" FILES ${files_exts_stb})

ADD_EXECUTABLE(mtg ${files_map_to_gltf} ${files_exts_map_files} ${files_exts_fx} ${files_exts_stb})

IF(WIN32)
	IF(MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mtg)
		set_property(TARGET mtg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
	ENDIF()
ENDIF()
