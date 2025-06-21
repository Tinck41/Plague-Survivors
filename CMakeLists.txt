cmake_minimum_required(VERSION 3.28)

project(Plague-Survivors LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
	enable_language(OBJC)
endif()

set(BOX2D_BUILD_UNIT_TESTS FALSE)
set(BOX2D_BUILD_TESTBED FALSE)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

#add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/raylib)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/flecs)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/box2d)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/spdlog)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/glm)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL3)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL3_image)

include_directories(${CMAKE_SOURCE_DIR}/vendor/box2d/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/glm)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} 
	src/main.cpp
	src/texture.h
	src/texture.cpp
	src/ecsModule/assetModule/module.h
	src/ecsModule/assetModule/module.cpp
	#src/ecsModule/common.h
	#src/ecsModule/uiModule/module.h
	#src/ecsModule/uiModule/module.cpp
	#src/ecsModule/physicsModule/module.h
	#src/ecsModule/physicsModule/module.cpp
	#src/ecsModule/timeModule/module.h
	#src/ecsModule/timeModule/module.cpp
	src/ecsModule/cameraModule/module.h
	src/ecsModule/cameraModule/module.cpp
	#src/ecsModule/inputModule/module.h
	#src/ecsModule/inputModule/module.cpp
	#src/ecsModule/playerModule/module.h
	#src/ecsModule/playerModule/module.cpp
	src/ecsModule/transformModule/module.h
	src/ecsModule/transformModule/module.cpp
	src/ecsModule/appModule/module.h
	src/ecsModule/appModule/module.cpp
	#src/ecsModule/sceneModule/module.h
	#src/ecsModule/sceneModule/module.cpp
	src/ecsModule/renderModule/module.h
	src/ecsModule/renderModule/module.cpp
	src/ecsModule/spriteModule/module.h
	src/ecsModule/spriteModule/module.cpp
	#src/ecsModule/tweenModule/module.h
	#src/ecsModule/tweenModule/module.cpp
	src/ecsModule/utils.h
	src/ecsModule/utils.cpp
	src/utils/sdl.h
	src/utils/sdl.cpp
)

#target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
target_link_libraries(${PROJECT_NAME} PRIVATE flecs::flecs_static)
target_link_libraries(${PROJECT_NAME} PRIVATE box2d)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_image::SDL3_image)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR} )
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR} )
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++${CMAKE_CXX_STANDARD}" )

#if(WIN32)
#	add_custom_command(
#		TARGET ${PROJECT_NAME}
#		COMMENT "Copy OpenAL DLL"
#		PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
#		VERBATIM)
#endif()

install(TARGETS ${PROJECT_NAME})
