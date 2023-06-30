cmake_minimum_required(VERSION 3.24)
project(NewVulkan CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# vulkan
find_package(Vulkan REQUIRED)

#glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)

set(SOURCE_DIR src)
set(INCLUDE_DIR include)

set(
	SOURCE_FILES
	${SOURCE_DIR}/logger.cpp
	${SOURCE_DIR}/math-core.cpp
	# ${SOURCE_DIR}/models.cpp
	${SOURCE_DIR}/model-handler.cpp
	${SOURCE_DIR}/render.cpp
	${SOURCE_DIR}/vulkan_helpers.cpp
	${SOURCE_DIR}/material.cpp
	${SOURCE_DIR}/image.cpp
)

set(
	INCLUDE_FILES
	${INCLUDE_DIR}/buffer.h
	${INCLUDE_DIR}/ecs.h
	${INCLUDE_DIR}/logger.h
	${INCLUDE_DIR}/math-core.h
	# ${INCLUDE_DIR}/models.h
	${INCLUDE_DIR}/model-handler.h
	${INCLUDE_DIR}/nve_types.h
	${INCLUDE_DIR}/render.h
	${INCLUDE_DIR}/vulkan_helpers.h
	${INCLUDE_DIR}/material.h
	${INCLUDE_DIR}/image.h
)

option(NVE_BUILD_EXAMPLES "build the example programs" OFF)

if (NVE_BUILD_EXAMPLES)
	add_subdirectory("examples")
endif()

#imgui
set(IMGUI_FOLDER external/imgui)

set(
	IMGUI_FILES

	${IMGUI_FOLDER}/imconfig.h
	${IMGUI_FOLDER}/imgui_demo.cpp
	${IMGUI_FOLDER}/imgui_impl_vulkan.cpp
	${IMGUI_FOLDER}/imgui_widgets.cpp
	${IMGUI_FOLDER}/imgui_draw.cpp
	${IMGUI_FOLDER}/imgui_impl_vulkan.h
	${IMGUI_FOLDER}/imstb_rectpack.h
	${IMGUI_FOLDER}/imgui.cpp
	${IMGUI_FOLDER}/imgui_impl_glfw.cpp
	${IMGUI_FOLDER}/imgui_internal.h
	${IMGUI_FOLDER}/imstb_textedit.h
	${IMGUI_FOLDER}/imgui.h
	${IMGUI_FOLDER}/imgui_impl_glfw.h
	${IMGUI_FOLDER}/imgui_tables.cpp
	${IMGUI_FOLDER}/imstb_truetype.h
)

set(
	EXTERNAL_LIBRARY_FILES
	${IMGUI_FILES}
	external/tinyobjloader/tiny_obj_loader.h
)

add_library(nve ${SOURCE_FILES} ${INCLUDE_FILES} ${EXTERNAL_LIBRARY_FILES})

set_target_properties(nve PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(nve PUBLIC include)
target_include_directories(nve PUBLIC external/imgui)
target_include_directories(nve PUBLIC external/tinyobjloader)
target_link_libraries(nve PUBLIC Vulkan::Vulkan glfw)
