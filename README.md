# Overview
This is my second try at making a simple render engine with [Vulkan](https://www.vulkan.org/) in C++. The basic stuff is mostly aligned with the great tutorial from https://vulkan-tutorial.com/.
Planned is instanced rasterized rendering with a simple model handler and after that raytraced and raymarched rendering with partly custom shaders. A GUI is also provided with the [DearImGui Library](https://github.com/ocornut/imgui).
In addition to the renderer, a simple entity-component-system is there for convenient control of the scene and objects (the binding-in with the renderer is currently in progress).

# Code Interface
The engine is designed to be (hopefully) as simple to use as possible and still provide a big range of control.
The basic interface in code currently looks something like this:
```c++
Renderer renderer;
RenderConfig renderConfig;
// set render configuration like:
renderConfig.width = 800;
renderConfig.height = 600;
// ...
renderer.init(renderConfig);
// render loop
while (true)
{
    NVE_RESULT res = renderer.render();
    if (res == NVE_RENDER_EXIT_SUCCESS)
        break;
}
```
Models are at the moment brought in through a model handler bound to the renderer. e.g.:
```c++
ModelHandler modelHandler;
renderer.bind_model_handler(&modelHandler);

Model cube = Model::create_model(Mesh::create_cube());
cube.m_info.position = Vector3(1, 0, 3);
modelHandler.add_model(&cube);
```


# Requirements
Folder *external* and in it the folders:
- GLFW
- imgui
    - {imgui_source_files}.cpp
    - {imgui_include_files}.h

For convenience, there is a script to automatically get the external dependencies and place them in the right folder. It is *init-dir.sh* for linux (make sure to *chmod +x* it before running (this is more of a note to myself to not forget it)) and *init-dir.bat* for windows. The script also generates the cmake stuff, so you can directly start with compiling. The script needs [wget](https://www.gnu.org/software/wget/) to work properly.

# Some Trivia
- x is forward/back, y is right/left and z is up/down
- Classes are in PascalCase, methods_or_functions in snake_case
