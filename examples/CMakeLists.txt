set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

link_libraries(nve)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

set_directory_properties(PROPERTIES CMAKE_CXX_STANDARD 20)

add_executable(ecs ecs-example.cpp)
add_executable(render-loop render-loop-example.cpp)
add_executable(gui-example gui-example.cpp)
add_executable(main-devel main-devel.cpp)
add_executable(math-test math-test.cpp)
add_executable(rope-sim rope-sim.cpp)
add_executable(simple-fluid-example simple-fluid-example.cpp)
add_executable(physx-example physx/main.cpp)
add_executable(interface-example interface.cpp)
