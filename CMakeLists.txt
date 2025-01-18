cmake_minimum_required(VERSION 3.16)

project(Plague-Survivors LANGUAGES CXX C)

set(CMAKE_CXX_STANDART 20)
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
	src/ecsModule/components/transformComponent.h
	src/ecsModule/components/spriteComponent.h
	src/ecsModule/components/velocityComponent.h
	src/ecsModule/components/playerComponent.h
	src/ecsModule/components/rendererComponent.h
	src/ecsModule/components/timeComponent.h
	src/ecsModule/components/physicsComponent.h
	src/ecsModule/components/rigidbodyComponent.h
	src/ecsModule/components/boxColliderComponent.h
	src/ecsModule/components/inputComponent.h
	src/ecsModule/components/cameraComponent.h
	src/ecsModule/components/stateComponent.h
	src/ecsModule/components/dashComponent.h
	src/ecsModule/components/ui/imageComponent.h
	src/ecsModule/components/ui/styleComponent.h
	src/ecsModule/components/ui/nodeComponent.h
	src/ecsModule/components/ui/nodeBundle.h
	src/ecsModule/components/ui/buttonBundle.h
	src/ecsModule/components/ui/textBundle.h
	src/ecsModule/types.h
	src/ecsModule/ecsController.h
	src/ecsModule/ecsController.cpp
	src/ecsModule/systems/renderSystem.h
	src/ecsModule/systems/renderSystem.cpp
	src/ecsModule/systems/timeSystem.h
	src/ecsModule/systems/timeSystem.cpp
	src/ecsModule/systems/physicsSystem.h
	src/ecsModule/systems/physicsSystem.cpp
	src/ecsModule/systems/inputSystem.h
	src/ecsModule/systems/inputSystem.cpp
	src/ecsModule/systems/cameraSystem.h
	src/ecsModule/systems/cameraSystem.cpp
	src/ecsModule/systems/ui/uiSystem.h
	src/ecsModule/systems/ui/uiSystem.cpp
	src/ecsModule/components/ui/interactionComponent.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system)
target_link_libraries(${PROJECT_NAME} PRIVATE flecs::flecs_static)
target_link_libraries(${PROJECT_NAME} PRIVATE box2d)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR} )
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR} )

if(WIN32)
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMENT "Copy OpenAL DLL"
		PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
		VERBATIM)
endif()

install(TARGETS ${PROJECT_NAME})
