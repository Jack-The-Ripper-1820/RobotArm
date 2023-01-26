# CMakeList.txt : CMake project for RobotArm, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("RobotArm")

include_directories("common")
include_directories("models")

file(GLOB_RECURSE COMMON_SOURCES "common/*.cpp" "common/*.hpp")
file(GLOB SHADERS "shaders/*.glsl" "/*.glsl")

# Add source to this project's executable.
add_executable (${PROJECT_NAME} "RobotArm.cpp" "RobotArm.h" ${COMMON_SOURCES} ${SHADERS} )
file(COPY ${SHADERS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)

set(CMAKE_TOOLCHAIN_FILE "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "C:/src/vcpkg/installed/x64-windows/share")

#install(FILES ${CMAKE_PREFIX_PATH}/bin/glew32d.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

find_package(OpenGL REQUIRED) 
find_package(GLEW REQUIRED) 
find_package(glfw3 CONFIG REQUIRED) 
find_package(glm CONFIG REQUIRED)   
find_package(imgui CONFIG REQUIRED)

# TODO: Add tests and install targets if needed.
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL GLEW::GLEW glfw glm::glm imgui::imgui)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/bin/glew32d.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)

