cmake_minimum_required(VERSION 3.28)

project(Plague-Survivors LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES TRUE)
set(BOX2D_BUILD_UNIT_TESTS FALSE)
set(BOX2D_BUILD_TESTBED FALSE)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SFML)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/flecs)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/box2d)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/spdlog)

include_directories(${CMAKE_SOURCE_DIR}/vendor/SFML/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/box2d/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} 
	src/main.cpp
	src/application.h
	src/application.cpp
	src/resourceManager.h
	src/resourceManager.cpp
	src/ecsModule/common.h
	src/ecsModule/ecsController.h
	src/ecsModule/ecsController.cpp
	src/ecsModule/uiModule/module.h
	src/ecsModule/uiModule/module.cpp
	src/ecsModule/physicsModule/module.h
	src/ecsModule/physicsModule/module.cpp
	src/ecsModule/timeModule/module.h
	src/ecsModule/timeModule/module.cpp
	src/ecsModule/cameraModule/module.h
	src/ecsModule/cameraModule/module.cpp
	src/ecsModule/inputModule/module.h
	src/ecsModule/inputModule/module.cpp
	src/ecsModule/spriteModule/module.h
	src/ecsModule/spriteModule/module.cpp
	src/ecsModule/playerModule/module.h
	src/ecsModule/playerModule/module.cpp
	src/ecsModule/transformModule/module.h
	src/ecsModule/transformModule/module.cpp
	src/ecsModule/appModule/module.h
	src/ecsModule/appModule/module.cpp
	src/ecsModule/sceneModule/module.h
	src/ecsModule/sceneModule/module.cpp
	src/ecsModule/renderModule/module.h
	src/ecsModule/renderModule/module.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system)
target_link_libraries(${PROJECT_NAME} PRIVATE flecs::flecs_static)
target_link_libraries(${PROJECT_NAME} PRIVATE box2d)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR} )
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR} )

#set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++${CMAKE_CXX_STANDARD}" )

if(WIN32)
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMENT "Copy OpenAL DLL"
		PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
		VERBATIM)
endif()

install(TARGETS ${PROJECT_NAME})
