echo external dependencies...

mkdir external
pushd external

echo cloning GLFW...
git clone https://github.com/glfw/glfw.git

echo cloning Dear ImGui...
mkdir imgui
pushd imgui
wget -O imgui.cpp         https://raw.githubusercontent.com/ocornut/imgui/master/imgui.cpp
wget -O imgui.h           https://raw.githubusercontent.com/ocornut/imgui/master/imgui.h
wget -O imgui_demo.cpp    https://raw.githubusercontent.com/ocornut/imgui/master/imgui_demo.cpp
wget -O imgui_draw.cpp    https://raw.githubusercontent.com/ocornut/imgui/master/imgui_draw.cpp
wget -O imgui_internal.h  https://raw.githubusercontent.com/ocornut/imgui/master/imgui_internal.h
wget -O imgui_tables.cpp  https://raw.githubusercontent.com/ocornut/imgui/master/imgui_tables.cpp
wget -O imgui_widgets.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_widgets.cpp
wget -O imstb_rectpack.h  https://raw.githubusercontent.com/ocornut/imgui/master/imstb_rectpack.h
wget -O imstb_textedit.h  https://raw.githubusercontent.com/ocornut/imgui/master/imstb_textedit.h
wget -O imstb_truetype.h  https://raw.githubusercontent.com/ocornut/imgui/master/imstb_truetype.h
wget -O imconfig.h        https://raw.githubusercontent.com/ocornut/imgui/master/imconfig.h

popd
popd

echo cmake initialization...
mkdir build
pushd build
cmake ..
popd
echo done...