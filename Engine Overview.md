## `GeometryHandler`
A `GeometryHandler` handles the rendering of a specific subset of the geometry of a scene:
- _dynamic_: instanced rendering, models have a transform
- _static_: batched rendering, transforms are baked into the models

For rendering a specific set of geometry, the `GeometryHandler` class is derived and the `virtual methods` are overwritten.
The derived `GeometryHandler` is responsible for:
- `Command Buffer` recording
- passing models to the `GeometryHandler`

The `GeometryHandler` is responsible for:
- `Pipeline` creation and lifetime-handling
- `Material` handling (includes `Shaders` and `Textures`)
- `Decriptor` handling (creation of `Descriptor Set` and updating)
-  `Model` handling

Because different `Models` can have different `Materials` and with it different `Shaders`, the `GeometryHandler` divides `Models` into different `MeshGroups`. Those `MeshGroups` each save the mesh data, the `Shader`, the `Pipeline` responsible for its rendering, the `Command Buffers` for the different frames and the `Command Pool` used for creating them.
Each `MeshGroup` is rendered on its own in a secondary `Command Buffer`, which is called from the main `Command Buffer` in the `Renderer` in a designated subpass.

# Pipeline Creation
Each `MeshGroup` has its own pipeline, which, together with their `CreationInfos`, are collected by the `GeometryHandler` and given to the `Renderer`. Then they are processed by a `PipelineBatchCreator`, which divides them in groups based on the `Pipeline` type and `PipelineLayout` and then creates them in batch.
The `CreationInfo` is stored in the corresponding `Pipeline` object.