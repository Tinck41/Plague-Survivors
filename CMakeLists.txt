cmake_minimum_required(VERSION 3.16)

project(Plague-Survivors LANGUAGES CXX C)

set(CMAKE_CXX_STANDART 20)
set(SFML_STATIC_LIBRARIES TRUE)
set(BOX2D_BUILD_UNIT_TESTS FALSE)
set(BOX2D_BUILD_TESTBED FALSE)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SFML)
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
	src/core/gameObject.h
	src/core/gameObject.cpp
	src/core/componentHolder.h
	src/core/componentHolder.cpp
	src/core/relationshipHolder.h
	src/core/relationshipHolder.cpp
	src/core/componentBase.h
	src/core/componentBase.cpp
	src/core/sceneBase.h
	src/core/sceneController.h
	src/core/sceneController.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system)
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
