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

set(SOURCE_DIR src)
set(INCLUDE_DIR include)

set(
	SOURCE_FILES
	${SOURCE_DIR}/logger.cpp
	${SOURCE_DIR}/math-core.cpp
	# ${SOURCE_DIR}/models.cpp
	${SOURCE_DIR}/model-handler.cpp
	${SOURCE_DIR}/nve_types.cpp
	${SOURCE_DIR}/render.cpp
	${SOURCE_DIR}/material.cpp
	${SOURCE_DIR}/image.cpp
	${SOURCE_DIR}/profiler.cpp
	${SOURCE_DIR}/physics.cpp
	${SOURCE_DIR}/gui.cpp
	${SOURCE_DIR}/flags.cpp
	${SOURCE_DIR}/tritri.cpp
	${SOURCE_DIR}/post_processing.cpp
	${SOURCE_DIR}/thread_pool.cpp
	${SOURCE_DIR}/gizmos.cpp

	${SOURCE_DIR}/vulkan/vulkan_helpers.cpp
	${SOURCE_DIR}/vulkan/pipeline.cpp
	${SOURCE_DIR}/vulkan/vulkan_handles.cpp
	${SOURCE_DIR}/vulkan/raytracing_handles.cpp
	
	${SOURCE_DIR}/simple_fluid.cpp
      ${SOURCE_DIR}/spatial_hash_grid.cpp

      # Position Based Dynamics
      ${SOURCE_DIR}/pbd.cpp
      ${SOURCE_DIR}/pbd/fluid_constraints.cpp

	${SOURCE_DIR}/reference.cpp
	${SOURCE_DIR}/dependency.cpp
)

set(
	INCLUDE_FILES
	${INCLUDE_DIR}/ecs.h
	${INCLUDE_DIR}/logger.h
	${INCLUDE_DIR}/math-core.h
	# ${INCLUDE_DIR}/models.h
	${INCLUDE_DIR}/model-handler.h
	${INCLUDE_DIR}/nve_types.h
	${INCLUDE_DIR}/nve_types_fwd.h
	${INCLUDE_DIR}/render.h
	${INCLUDE_DIR}/material.h
	${INCLUDE_DIR}/image.h
	${INCLUDE_DIR}/profiler.h
	${INCLUDE_DIR}/physics.h
	${INCLUDE_DIR}/gui.h
	${INCLUDE_DIR}/flags.h
	${INCLUDE_DIR}/tritri.h
	${INCLUDE_DIR}/post_processing.h
	${INCLUDE_DIR}/space_consistent_vector.h
	${INCLUDE_DIR}/thread_pool.h
	${INCLUDE_DIR}/gizmos.h
	${INCLUDE_DIR}/component-editor.h

	${INCLUDE_DIR}/vulkan/buffer.h
	${INCLUDE_DIR}/vulkan/vulkan_helpers.h
	${INCLUDE_DIR}/vulkan/pipeline.h
	${INCLUDE_DIR}/vulkan/vulkan_handles.h
	${INCLUDE_DIR}/vulkan/vulkan_handles_fwd.h
	${INCLUDE_DIR}/vulkan/raytracing_handles.h

	${INCLUDE_DIR}/simple_fluid.h
      ${INCLUDE_DIR}/spatial_hash_grid.h

      # Position Based Dynamics
      ${INCLUDE_DIR}/pbd.h
      ${INCLUDE_DIR}/pbd/fluid_constraints.h

	${INCLUDE_DIR}/reference.h
	${INCLUDE_DIR}/dependency.h
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
