cmake_minimum_required(VERSION 3.24)
project(NewVulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# vulkan
find_package(Vulkan REQUIRED)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)

# physx
# option(PHYSX_BUILD_TYPE "The build type of PhysX, i.e., one of {debug, checked, profile, release}" "Debug")
# set(PHYSX_BUILD_TYPE "checked")

# include_directories(PhysX/include)

# if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#     message("Building snippet in debug configuration")
#     add_compile_definitions(_DEBUG)
#     link_directories("external/PhysX/physx/bin/win.x86_64.vc143.mt/debug") # This is the path where PhysX libraries are installed
# else()
#     message("Building snippet in release configuration with PhysX ${PHYSX_BUILD_TYPE} configuration")
#     add_compile_definitions(NDEBUG)
#     link_directories("external/PhysX/physx/bin/win.x86_64.vc143.mt/${PHYSX_BUILD_TYPE}") # This is the path where PhysX libraries are installed
# endif()

# gsl
# set(NO_AMPL_BINDINGS 1)
# set(GSL_DISABLE_TESTS 1)
# add_subdirectory(external/gsl)

# engine source
file(
		GLOB_RECURSE
		INCLUDE_FILES
		${CMAKE_SOURCE_DIR}/include/*.h
)
file(
		GLOB_RECURSE
		SOURCE_FILES
		${CMAKE_SOURCE_DIR}/src/*.cpp
)

option(NVE_BUILD_EXAMPLES "build the example programs" OFF)

if (NVE_BUILD_EXAMPLES)
	add_subdirectory("examples")
endif()

# imgui
set(IMGUI_FOLDER ${CMAKE_SOURCE_DIR}/external/imgui)
set(
		IMGUI_FILE_NAMES
		imconfig.h
		imgui_demo.cpp
		imgui_widgets.cpp
		imgui_draw.cpp
		imstb_rectpack.h
		imgui.cpp
		imgui_internal.h
		imstb_textedit.h
		imgui.h
		imgui_tables.cpp
		imstb_truetype.h

		backends/imgui_impl_glfw.h
		backends/imgui_impl_glfw.cpp
		backends/imgui_impl_vulkan.h
		backends/imgui_impl_vulkan.cpp
)

foreach (IMGUI_FILE IN LISTS ${IMGUI_FILE_NAMES})
	if (NOT EXISTS ${IMGUI_FOLDER}/${IMGUI_FILE})
		file(
				DOWNLOAD
				"https://raw.githubusercontent.com/ocornut/imgui/master/${IMGUI_FILE}"
				${IMGUI_FOLDER}/${IMGUI_FILE}
		)
	endif()
	LIST(APPEND IMGUI_FILES ${IMGUI_FOLDER}/${IMGUI_FILE})
endforeach()

# tinyobjectloader
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/external/tinyobjloader/tiny_obj_loader.h)
	file(
			DOWNLOAD
			"https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h"
			${CMAKE_SOURCE_DIR}/external/tinyobjloader/tiny_obj_loader.h
	)
endif()

# stbimage
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/external/stbimage/stb_image.h)
	file(
			DOWNLOAD
			"https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
			${CMAKE_SOURCE_DIR}/external/stbimage/stb_image.h
	)
endif()

# mINI
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/external/mini/ini.h)
	file(
			DOWNLOAD
			"https://raw.githubusercontent.com/pulzed/mINI/master/src/mini/ini.h"
			${CMAKE_SOURCE_DIR}/external/mini/ini.h
	)
endif()

set(
	EXTERNAL_LIBRARY_FILES
	${IMGUI_FILES}
	external/tinyobjloader/tiny_obj_loader.h
	external/stbimage/stb_image.h
	external/mini/ini.h
)

add_library(nve ${SOURCE_FILES} ${INCLUDE_FILES} ${EXTERNAL_LIBRARY_FILES})

set_target_properties(nve PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(nve PUBLIC include)
target_include_directories(nve PUBLIC external/imgui)
target_include_directories(nve PUBLIC external/tinyobjloader)
target_include_directories(nve PUBLIC external/stbimage)
target_include_directories(nve PUBLIC external/mini)
target_include_directories(nve PUBLIC external/PhysX/physx/include)
target_link_libraries(
	nve PUBLIC
	Vulkan::Vulkan
	glfw
	# gsl
	# PhysX_64
	# PhysXCommon_64
	# PhysXFoundation_64
)

add_definitions(-DROOT_DIRECTORY=\"${CMAKE_SOURCE_DIR}\")
