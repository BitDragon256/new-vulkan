cmake_minimum_required(VERSION 3.24)
project(NewVulkan CXX)

# vulkan
find_package(Vulkan REQUIRED)

#glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("external/glfw")

set(SOURCE_DIR src)
file(GLOB SOURCE_FILES
	${SOURCE_DIR}/*.cpp
)
set(INCLUDE_DIR include)
file(GLOB INCLUDE_FILES
	${INCLUDE_DIR}/*.h
)

add_executable(nve ${SOURCE_FILES} ${INCLUDE_FILES})

set_target_properties(nve PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET nve PROPERTY CXX_STANDARD 20)

target_include_directories(nve PUBLIC include)
target_link_libraries(nve PUBLIC Vulkan::Vulkan glfw)
