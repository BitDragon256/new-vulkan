#!/bin/bash

@echo off

echo External dependencies...

mkdir external
pushd external

echo Cloning GLFW...
git clone https://github.com/glfw/glfw.git

echo cloning Dear ImGui...
mkdir imgui
pushd imgui
wget -O imgui.cpp             https://raw.githubusercontent.com/ocornut/imgui/master/imgui.cpp
wget -O imgui.h               https://raw.githubusercontent.com/ocornut/imgui/master/imgui.h
wget -O imgui_demo.cpp        https://raw.githubusercontent.com/ocornut/imgui/master/imgui_demo.cpp
wget -O imgui_draw.cpp        https://raw.githubusercontent.com/ocornut/imgui/master/imgui_draw.cpp
wget -O imgui_internal.h      https://raw.githubusercontent.com/ocornut/imgui/master/imgui_internal.h
wget -O imgui_tables.cpp      https://raw.githubusercontent.com/ocornut/imgui/master/imgui_tables.cpp
wget -O imgui_widgets.cpp     https://raw.githubusercontent.com/ocornut/imgui/master/imgui_widgets.cpp
wget -O imstb_rectpack.h      https://raw.githubusercontent.com/ocornut/imgui/master/imstb_rectpack.h
wget -O imstb_textedit.h      https://raw.githubusercontent.com/ocornut/imgui/master/imstb_textedit.h
wget -O imstb_truetype.h      https://raw.githubusercontent.com/ocornut/imgui/master/imstb_truetype.h
wget -O imconfig.h            https://raw.githubusercontent.com/ocornut/imgui/master/imconfig.h
wget -O imgui_impl_vulkan.h   https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_vulkan.h
wget -O imgui_impl_vulkan.cpp https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_vulkan.cpp
wget -O imgui_impl_glfw.h     https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_glfw.h
wget -O imgui_impl_glfw.cpp   https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_glfw.cpp

popd
popd

echo Cmake initialization...
mkdir build
pushd build
cmake ..
popd
echo Done...