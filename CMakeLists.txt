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

SET(files_code
	code/main.cpp
	code/mapconverter.h
	code/mapconverter.cpp
	code/brush.cpp
	code/brush.h
	code/entity.cpp
	code/entity.h
	code/face.cpp
	code/map.cpp
	code/map.h
	code/math.h
	code/poly.cpp
)

SET(files_exts
	exts/flags.h
)

SET(files_exts_fx
	exts/fx/gltf.h
	exts/fx/json.hpp
)

SET(files_exts_stb
	exts/stb/stbimage.h
	exts/stb/stb_image_write.h
)

SOURCE_GROUP("mtg" FILES ${files_code})
SOURCE_GROUP("exts" FILES ${files_exts})
SOURCE_GROUP("fx" FILES ${files_exts_fx})
SOURCE_GROUP("stb" FILES ${files_exts_stb})

ADD_EXECUTABLE(mtg ${files_code} ${files_exts} ${files_exts_fx} ${files_exts_stb})

TARGET_INCLUDE_DIRECTORIES(mtg PRIVATE .)

IF(WIN32)
	IF(MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mtg)
		set_property(TARGET mtg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
	ENDIF()
ENDIF()
