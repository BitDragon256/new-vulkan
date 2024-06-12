#include "render.h"

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "logger.h"
#include "vulkan/vulkan_helpers.h"
#include "material.h"

#ifdef RENDER_PROFILER
#define PROFILE_START(X) m_profiler.start_measure(X);
#define PROFILE_END(X) m_profiler.end_measure(X, true);
#else
#define PROFILE_START(X)
#define PROFILE_END(X) 0.f;
#endif

// ---------------------------------------
// NON-MEMBER FUNCTIONS
// ---------------------------------------

template<typename T>
void append_vector(std::vector<T>& origin, std::vector<T>& appendage)
{
      if (appendage.empty())
            return;
      origin.insert(origin.end(), appendage.begin(), appendage.end());
}


// ---------------------------------------
// RENDERER
// ---------------------------------------

// PUBLIC METHODS

Renderer::Renderer() :
      m_ecs{ this }, m_initialized{ false }
{}
Renderer::~Renderer()
{
      clean_up();
}
void Renderer::init(RenderConfig config)
{
      m_config = config;
      m_firstFrame = true;
      m_initialized = true;

      // add_descriptors();

      glfwInit();

      m_vulkanHandles.window.initialize(config.width, config.height, config.title);
      m_vulkanHandles.instance.initialize(config.vulkanApplicationName, config.vulkanApplicationVersion, "New Vulkan Engine", config.enableValidationLayers);
      m_vulkanHandles.surface.initialize(&m_vulkanHandles.instance, &m_vulkanHandles.window);
      m_vulkanHandles.physicalDevice.initialize(&m_vulkanHandles.instance, &m_vulkanHandles.surface);
      m_vulkanHandles.device.initialize(&m_vulkanHandles.physicalDevice, &m_vulkanHandles.surface);
      m_vulkanHandles.swapchain.initialize(
            &m_vulkanHandles.device,
            &m_vulkanHandles.physicalDevice,
            &m_vulkanHandles.window,
            &m_vulkanHandles.surface,
            &m_vulkanHandles.renderPass
      );
      m_vulkanHandles.renderPass.initialize(
            &m_vulkanHandles.device,
            &m_vulkanHandles.physicalDevice,
            &m_vulkanHandles.swapchain,
            &m_vulkanHandles.subpassCountHandler
      );
      m_vulkanHandles.commandPool.initialize(
            &m_vulkanHandles.device
      );
      m_vulkanHandles.mainCommandBuffers.initialize(
            &m_vulkanHandles.device,
            &m_vulkanHandles.commandPool,
            1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY
      );
      m_vulkanHandles.descriptorPool.initialize(
            &m_vulkanHandles.device
      );
      m_vulkanHandles.subpassCountHandler.initialize();

      for (auto geomHandler : all_geometry_handlers())
      {
            m_vulkanHandles.subpassCountHandler.add_subpass_count_callback([geomHandler]()
            {
                  return geomHandler.get()->subpass_count();
            });
      }

      m_vulkanHandles.window.try_update();
      m_vulkanHandles.instance.try_update();
      m_vulkanHandles.subpassCountHandler.try_update();

      initialize_sync_objects();

      //logger::log_cond(create_window(config.width, config.height, config.title) == NVE_SUCCESS, "window created");
      //logger::log_cond(create_instance() == NVE_SUCCESS, "instance created");
      //// logger::log_cond(create_debug_messenger() == NVE_SUCCESS, "debug messenger created");
      //logger::log_cond(get_surface() == NVE_SUCCESS, "surface created");
      //logger::log_cond(get_physical_device() == NVE_SUCCESS, "found physical device");
      //logger::log_cond(create_device() == NVE_SUCCESS, "logical device created");
      //logger::log_cond(create_swapchain() == NVE_SUCCESS, "swapchain created");
      //logger::log_cond(create_swapchain_image_views() == NVE_SUCCESS, "swapchain image views created");
      //create_depth_images();              logger::log("depth images created");
      //logger::log_cond(create_render_pass() == NVE_SUCCESS, "render pass created");
      //logger::log_cond(create_framebuffers() == NVE_SUCCESS, "framebuffers created");
      //logger::log_cond(create_commandpool() == NVE_SUCCESS, "command pool created");
      //logger::log_cond(create_commandbuffers() == NVE_SUCCESS, "command buffer created");
      //logger::log_cond(create_sync_objects() == NVE_SUCCESS, "sync objects created");

      m_threadPool.initialize(m_threadCount);

      // TODO delete this
      Shader::s_device = m_vulkanHandles.device;

      // create_descriptor_pool();           logger::log("descriptor pool created");

      initialize_geometry_handlers();     logger::log("geometry handlers initialized");

#ifndef NVE_NO_GUI

      // imgui
      imgui_init();

#endif

      m_deltaTime = 0;
      m_lastFrameTime = std::chrono::high_resolution_clock::now();

      m_avgRenderTime = 0;
      m_acquireImageTimeout = false;

      m_ecs.lock();
      m_guiManager.initialize(&m_ecs);

      init_default_camera();
}
NVE_RESULT Renderer::render()
{
      PROFILE_START("total render time");
      PROFILE_START("glfw window should close poll");
      if (glfwWindowShouldClose(m_vulkanHandles.window))
      {
            return NVE_RENDER_EXIT_SUCCESS;
      }
      PROFILE_END("glfw window should close poll");

      std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
      m_deltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_lastFrameTime).count() / 1000000000.f;
      m_lastFrameTime = now;

      PROFILE_START("ecs update");
      if (m_firstFrame || m_config.autoECSUpdate)
      {
            m_ecs.unlock();
            m_ecs.update_systems(m_deltaTime);
            m_ecs.lock();
      }
      PROFILE_END("ecs update");

      PROFILE_START("glfw poll events");
      glfwPollEvents();
      PROFILE_END("glfw poll events");

      PROFILE_START("draw frame");
      draw_frame();
      PROFILE_END("draw frame");
      PROFILE_END("total render time");

      m_ecs.unlock();

      return NVE_SUCCESS;
}
void Renderer::set_active_camera(Camera* camera)
{
      if (!camera)
            logger::log_err("camera is not valid");

      m_activeCamera = camera;
      m_activeCamera->m_extent.x = m_vulkanHandles.swapchain.m_extent.width;
      m_activeCamera->m_extent.y = m_vulkanHandles.swapchain.m_extent.height;
}
Camera& Renderer::active_camera()
{
      return *m_activeCamera;
}

int Renderer::get_key(int key)
{
      return glfwGetKey(m_vulkanHandles.window, key);
}
int Renderer::get_mouse_button(int btn)
{
      return glfwGetMouseButton(m_vulkanHandles.window, btn);
}
Vector2 Renderer::get_mouse_pos()
{
      double xPos, yPos;
      glfwGetCursorPos(m_vulkanHandles.window, &xPos, &yPos);
      return { xPos, yPos };
}
Vector2 Renderer::mouse_to_screen(Vector2 mouse)
{
      return 2.f * Vector2 {
            mouse.x / m_vulkanHandles.swapchain.m_extent.width / m_guiManager.m_cut.x,
                  mouse.y / m_vulkanHandles.swapchain.m_extent.height / m_guiManager.m_cut.y
      } - Vector2(1.f);
}

void Renderer::set_light_pos(Vector3 pos)
{
      m_cameraPushConstant.lightPos = pos;
}

void Renderer::gizmos_draw_line(Vector3 start, Vector3 end, Color color, float width)
{
      m_gizmosHandler.draw_line(start, end, color, width);
}
void Renderer::gizmos_draw_ray(Vector3 start, Vector3 direction, Color color, float width)
{
      m_gizmosHandler.draw_ray(start, direction, color, width);
}


EntityId Renderer::create_empty_game_object()
{
      const auto entity = m_ecs.create_entity();
      m_ecs.add_component<Transform>(entity);
      return entity;
}
EntityId Renderer::create_default_model(DefaultModel::DefaultModel defaultModel)
{
      const auto entity = create_empty_game_object();
      auto& model = m_ecs.add_component<DynamicModel>(entity);
      model.load_mesh(s_defaultModelToPath[defaultModel]);
      return entity;
}

void Renderer::reload_pipelines()
{
      await_last_frame_render();
      create_geometry_pipelines();
}

// PRIVATE METHODS

void Renderer::recreate_render_pass()
{
      if (m_lastGeometryHandlerSubpassCount < geometry_handler_subpass_count())
      {
            m_vulkanHandles.renderPass.unresolve();
            m_vulkanHandles.renderPass.try_update();

            // vkDestroyRenderPass(m_device, m_renderPass, nullptr);
            // create_render_pass();
            // create_framebuffers();
            // set_geometry_handler_subpasses();
            // create_geometry_pipelines();
            // update_geometry_handler_framebuffers();

            m_lastGeometryHandlerSubpassCount = geometry_handler_subpass_count();
      }
}

/*
void Renderer::create_depth_images()
{
      m_depthImages.resize(m_swapchainImages.size());
      for (auto& image : m_depthImages)
      {
            image.create(
                  m_device,
                  m_physicalDevice,
                  m_swapchainExtent.width,
                  m_swapchainExtent.height,
                  find_depth_format(m_physicalDevice),
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  VK_IMAGE_ASPECT_DEPTH_BIT
            );
      }
}
*/

void Renderer::initialize_geometry_handlers()
{
      GeometryHandlerVulkanObjects vulkanObjects;
      vulkanObjects.device = &m_vulkanHandles.device;
      vulkanObjects.physicalDevice = &m_vulkanHandles.physicalDevice;
      vulkanObjects.commandPool = &m_vulkanHandles.commandPool;
      vulkanObjects.renderPass = &m_vulkanHandles.renderPass;
      vulkanObjects.transferQueue = &transfer_queue();
      vulkanObjects.queueFamilyIndex = transfer_queue_family();
      vulkanObjects.descriptorPool = &m_vulkanHandles.descriptorPool;

      vulkanObjects.swapchainExtent = &m_vulkanHandles.swapchain.m_extent;
      // vulkanObjects.framebuffers = std::vector<VkFramebuffer*>(m_swapchainFramebuffers.size());
      // for (size_t i = 0; i < vulkanObjects.framebuffers.size(); i++)
      //     vulkanObjects.framebuffers[i] = &m_swapchainFramebuffers[i];
      vulkanObjects.framebuffers = to_ref_vec(m_vulkanHandles.swapchain.m_framebuffers);
      vulkanObjects.firstSubpass = 0;

      vulkanObjects.pCameraPushConstant = &m_cameraPushConstant;

      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            handler->initialize(vulkanObjects, &m_guiManager);
      }
      set_geometry_handler_subpasses();

      m_ecs.register_system<StaticGeometryHandler>(&m_staticGeometryHandler);
      m_ecs.register_system<DynamicGeometryHandler>(&m_dynamicGeometryHandler);
      m_ecs.register_system<GizmosHandler>(&m_gizmosHandler);
}
void Renderer::set_geometry_handler_subpasses()
{
      uint32_t subpass = 0;
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            handler->set_first_subpass(subpass);
            subpass += handler->subpass_count();
      }
}
void Renderer::create_geometry_pipelines()
{
      auto handlers = all_geometry_handlers();
      m_vulkanHandles.subpassCountHandler.check_subpasses();

      for (auto geometryHandler : handlers)
      {
            std::vector<vk::PipelineRef> pipelines;
            geometryHandler->get_pipelines(pipelines);

            for (auto pipeline : pipelines)
                  m_pipelineBatchCreator.schedule_creation(pipeline);
      }
      m_pipelineBatchCreator.create_all();
}
std::vector<REF(GeometryHandler)> Renderer::all_geometry_handlers()
{
      return {
          &m_staticGeometryHandler,
          &m_dynamicGeometryHandler,
          &m_gizmosHandler
      };
}
// TODO staged buffer copy synchronization
void Renderer::wait_for_geometry_handler_buffer_cpies()
{
      std::vector<VkFence> fences;
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            auto handlerFences = handler->buffer_cpy_fences();
            append_vector(fences, handlerFences);
      }
      vkWaitForFences(m_vulkanHandles.device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, UINT32_MAX);
}

void Renderer::destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
      }
}

void Renderer::record_main_command_buffer(uint32_t frame)
{
      // -------------------------------------------

      VkCommandBuffer mainCommandBuffer = m_vulkanHandles.mainCommandBuffers.get_command_buffer(frame);

      // -------------------------------------------

      vkResetCommandBuffer(mainCommandBuffer, 0);

      // -------------------------------------------

      VkCommandBufferBeginInfo commandBufferBI = {};
      commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      commandBufferBI.pNext = nullptr;
      commandBufferBI.flags = 0;
      commandBufferBI.pInheritanceInfo = nullptr;

      {
            auto res = vkBeginCommandBuffer(mainCommandBuffer, &commandBufferBI);
            logger::log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording");
      }

      // -------------------------------------------

      VkRenderPassBeginInfo renderPassBI = {};
      renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBI.renderPass = m_vulkanHandles.renderPass;
      renderPassBI.framebuffer = m_vulkanHandles.swapchain.m_framebuffers[frame];
      renderPassBI.renderArea.offset = { 0, 0 };
      renderPassBI.renderArea.extent = m_vulkanHandles.swapchain.m_extent;

      std::vector<VkClearValue> clearValues(2);
      clearValues[0].color = { m_config.clearColor.x / 255.f, m_config.clearColor.y / 255.f, m_config.clearColor.z / 255.f, 1.f };
      clearValues[1].depthStencil = { 1.0f, 0 };
      renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
      renderPassBI.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(mainCommandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

      // -------------------------------------------

      std::vector<VkCommandBuffer> secondaryCommandBuffers;
      auto geometryHandlers = all_geometry_handlers();
      for (auto geometryHandler : geometryHandlers)
      {
            auto geometryHandlerCommandBuffers = geometryHandler->get_command_buffers(frame);
            append_vector(secondaryCommandBuffers, geometryHandlerCommandBuffers);
      }

      uint32_t subpassCount = secondaryCommandBuffers.size();
      for (uint32_t subpass = 0; subpass < subpassCount; subpass++)
      {
            if (subpass < secondaryCommandBuffers.size())
                  vkCmdExecuteCommands(mainCommandBuffer, 1, &secondaryCommandBuffers[subpass]);
            if (subpass != subpassCount - 1)
                  vkCmdNextSubpass(mainCommandBuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
      }

      // -------------------------------------------

      vkCmdEndRenderPass(mainCommandBuffer);

      {
            auto res = vkEndCommandBuffer(mainCommandBuffer);
            logger::log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording: " + std::string(string_VkResult(res)));
      }

      // -------------------------------------------
}
NVE_RESULT Renderer::submit_command_buffers(
      std::vector<VkCommandBuffer> commandBuffers,
      std::vector<VkSemaphore> waitSems,
      std::vector<VkPipelineStageFlags> waitStages,
      std::vector<VkSemaphore> signalSems
)
{
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
      submitInfo.pWaitSemaphores = waitSems.data();
      submitInfo.pWaitDstStageMask = waitStages.data();

      submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
      submitInfo.pCommandBuffers = commandBuffers.data();

      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSems.size());
      submitInfo.pSignalSemaphores = signalSems.data();

      graphics_queue().submit(submitInfo, &m_vulkanHandles.inFlightFences[frame_object_index()]);

      return NVE_SUCCESS;
}

NVE_RESULT Renderer::draw_frame()
{
      if (m_firstFrame)
            first_frame();

      //auto renderStart = std::chrono::high_resolution_clock::now();
      float renderTime = 0;
      m_profiler.begin_label("draw_frame");
      PROFILE_START("await fences");
      // Wait for the previous frame to finish
      if (!m_acquireImageTimeout)
            m_vulkanHandles.inFlightFences[frame_object_index()].wait();

      renderTime += PROFILE_END("await fences");

      m_vulkanHandles.subpassCountHandler.check_subpasses();

      // PROFILE_START("render pass recreation");
      // // recreate render pass if needed
      // recreate_render_pass();
      // renderTime += PROFILE_END("render pass recreation");

      PROFILE_START("acquire image");
      // Acquire an image from the swap chain
      {
            // auto res = vkAcquireNextImageKHR(m_device, m_swapchain, 1, m_imageAvailableSemaphores[m_frame], VK_NULL_HANDLE, &imageIndex);
            auto res = m_vulkanHandles.swapchain.next_image();
            if (res == VK_TIMEOUT)
            {
                  m_acquireImageTimeout = true;
                  return NVE_SUCCESS;
            }
            else
            {
                  m_acquireImageTimeout = false;
            }
      }
      renderTime += PROFILE_END("acquire image");

      // update push constant
      m_cameraPushConstant.projView = m_activeCamera->projection_matrix() * m_activeCamera->view_matrix();
      m_cameraPushConstant.camPos = m_activeCamera->m_position;

      // collect semaphores
      // this has to be done before the model handler update because the geometry handlers otherwise doesn't update its buffers
      std::vector<REF(vk::Semaphore)> waitSemaphores = { m_vulkanHandles.swapchain.current_image_available_semaphore() };

      // record command buffers
      PROFILE_START("record cmd buffers");
      auto geometryHandlers = all_geometry_handlers();
      for (auto geometryHandler : geometryHandlers)
      {
            // TODO staged buffer copy synchronization
            //auto sems = geometryHandler->buffer_cpy_semaphores();
            //for (auto sem : sems)
            //    if (sem != VK_NULL_HANDLE)
            //        waitSemaphores.push_back(sem);

            m_threadPool.doJob(std::bind(&Renderer::genCmdBuf, this, geometryHandler.get()));
      }
      m_threadPool.wait_for_finish();
      renderTime += PROFILE_END("record cmd buffers");

      PROFILE_START("record main cmd buf");
      record_main_command_buffer(frame_object_index());
      renderTime += PROFILE_END("record main cmd buf");

#ifndef NVE_NO_GUI

      PROFILE_START("gui draw");
      if (!m_imguiDraw)
            gui_begin();

      imgui_draw(m_vulkanHandles.swapchain.m_currentImageIndex);
      m_imguiDraw = false;
      renderTime += PROFILE_END("gui draw");

#endif

      std::vector<REF(vk::Semaphore)> signalSemaphores = { &m_vulkanHandles.renderFinishedSemaphores[frame_object_index()]};
      std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

      // TODO staged buffer copy synchronization
      //while (waitSemaphores.size() > waitStages.size())
      //    waitStages.push_back(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

      // wait_for_geometry_handler_buffer_cpies();

      PROFILE_START("submit cmd buf");
      // Submit the recorded command buffers
      std::vector<VkCommandBuffer> commandBuffers = {
            current_main_command_buffer()
      };
#ifndef NVE_NO_GUI
      commandBuffers.push_back(curent_imgui_command_buffer());
#endif // !NVE_NO_GUI


      graphics_queue().submit_command_buffers(
            commandBuffers,
            waitSemaphores,
            waitStages,
            signalSemaphores,
            &m_vulkanHandles.inFlightFences[frame_object_index()]
      );
      
      renderTime += PROFILE_END("submit cmd buf");

      PROFILE_START("present image");
      // Present the swap chain image
      m_vulkanHandles.swapchain.present_current_image(signalSemaphores);
      renderTime += PROFILE_END("present image");
#ifdef RENDER_PROFILER
      m_profiler.out_buf() << "total render time: " << renderTime << " seconds\n";
#endif
      m_profiler.end_label();

      m_vulkanHandles.swapchain.next_frame();

      //auto renderEnd = std::chrono::high_resolution_clock::now();
      //std::cout << "total render time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(renderEnd - renderStart).count() / NANOSECONDS_PER_SECOND;

      return NVE_SUCCESS;
}
void Renderer::first_frame()
{
      recreate_render_pass();
      create_geometry_pipelines();

      m_firstFrame = false;
}

void Renderer::clean_up()
{
      if (!m_initialized)
            return;
      m_initialized = false;

      vkDeviceWaitIdle(m_vulkanHandles.device);

#ifndef NVE_NO_GUI
      imgui_cleanup();
#endif

      geometry_handler_cleanup();
      m_pipelineBatchCreator.destroy();

      m_vulkanHandles.instance.unresolve();
      m_vulkanHandles.window.unresolve();

      glfwTerminate();
}
void Renderer::geometry_handler_cleanup()
{
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
            handler->cleanup();
}

uint32_t Renderer::geometry_handler_subpass_count()
{
      auto handlers = all_geometry_handlers();
      uint32_t subpassCount = 0;
      for (auto handler : handlers)
            subpassCount += handler->subpass_count();
      return subpassCount;
}

void Renderer::init_default_camera()
{
      set_active_camera(&m_defaultCamera);
}

//void Renderer::add_descriptors()
//{
//    if (m_config.useModelHandler)
//    {
//        VkDescriptorSetLayoutBinding binding = {};
//        binding.binding = NVE_MODEL_INFO_BUFFER_BINDING;
//        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//        binding.descriptorCount = NVE_MAX_MODEL_INFO_COUNT;
//
//        m_descriptors.push_back(binding);
//    }
//}
//void Renderer::update_model_info_descriptor_set()
//{
//    VkWriteDescriptorSet write = {};
//    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    write.dstSet = m_descriptorSet;
//    write.dstBinding = NVE_MODEL_INFO_BUFFER_BINDING;
//    write.dstArrayElement = 0;
//    write.descriptorCount = 1;
//    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//
//    VkDescriptorBufferInfo bufferInfo = {};
//    bufferInfo.buffer = m_pModelHandler->model_buffer();
//    bufferInfo.offset = 0;
//    bufferInfo.range = VK_WHOLE_SIZE;
//
//    write.pBufferInfo = &bufferInfo;
//
//    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
//}

void Renderer::genCmdBuf(GeometryHandler* geometryHandler)
{
      geometryHandler->record_command_buffers(frame_object_index());
};
void Renderer::await_last_frame_render()
{
      m_vulkanHandles.inFlightFences[last_frame_object_index()].wait();
}

uint32_t Renderer::frame_object_index()
{
      return m_vulkanHandles.swapchain.m_frameObectIndex;
}
uint32_t Renderer::last_frame_object_index()
{
      return m_vulkanHandles.swapchain.m_lastFrameObjectIndex;
}
uint32_t Renderer::swapchain_image_index()
{
      return m_vulkanHandles.swapchain.m_currentImageIndex;
}

vk::Queue& Renderer::graphics_queue()
{
      return m_vulkanHandles.device.m_graphicsQueue;
}
uint32_t Renderer::graphics_queue_family()
{
      return m_vulkanHandles.device.graphics_queue_family();
}
vk::Queue& Renderer::presentation_queue()
{
      return m_vulkanHandles.device.m_presentationQueue;
}
uint32_t Renderer::presentation_queue_family()
{
      return m_vulkanHandles.device.presentation_queue_family();
}
vk::Queue& Renderer::transfer_queue()
{
      return m_vulkanHandles.device.m_transferQueue;
}
uint32_t Renderer::transfer_queue_family()
{
      return m_vulkanHandles.device.transfer_queue_family();
}
vk::Queue& Renderer::compute_queue()
{
      return m_vulkanHandles.device.m_computeQueue;
}
uint32_t Renderer::compute_queue_family()
{
      return m_vulkanHandles.device.compute_queue_family();
}

VkCommandBuffer Renderer::current_main_command_buffer()
{
      return m_vulkanHandles.mainCommandBuffers.get_command_buffer(frame_object_index());
}

void Renderer::initialize_sync_objects()
{
      m_vulkanHandles.inFlightFences.resize(m_vulkanHandles.swapchain.size());
      m_vulkanHandles.renderFinishedSemaphores.resize(m_vulkanHandles.swapchain.size());

      for (auto& fence : m_vulkanHandles.inFlightFences)
      {
            fence.initialize(&m_vulkanHandles.device, true);
            fence.try_update();
      }
      for (auto& semaphore : m_vulkanHandles.renderFinishedSemaphores)
      {
            semaphore.initialize(&m_vulkanHandles.device);
            semaphore.try_update();
      }
}

#ifndef NVE_NO_GUI

// ---------------------------------------
// GUI
// ---------------------------------------

NVE_RESULT Renderer::imgui_init()
{
      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
      //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

      // Setup Dear ImGui style
      ImGui::StyleColorsDark();
      //ImGui::StyleColorsClassic();

      imgui_create_descriptor_pool();
      imgui_create_render_pass();
      imgui_create_framebuffers();
      imgui_create_command_pool();
      imgui_create_command_buffers();

      ImGui_ImplGlfw_InitForVulkan(m_window, true);
      ImGui_ImplVulkan_InitInfo initInfo = {};
      initInfo.Instance = m_vulkanHandles.instance;
      initInfo.PhysicalDevice = m_vulkanHandles.physicalDevice;
      initInfo.Device = m_vulkanHandles.device;
      initInfo.QueueFamily = 42; // is it working?
      initInfo.Queue = graphics_queue();
      initInfo.PipelineCache = VK_NULL_HANDLE;
      initInfo.DescriptorPool = m_imgui_descriptorPool;
      initInfo.Subpass = 0;

      uint32_t imageCount = m_vulkanHandles.swapchain.size();

      initInfo.MinImageCount = imageCount;
      initInfo.ImageCount = imageCount;
      initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
      initInfo.Allocator = nullptr;
      //initInfo.CheckVkResultFn = imgui_error_handle;
      ImGui_ImplVulkan_Init(&initInfo, m_imgui_renderPass);

      imgui_upload_fonts();
      m_imguiDraw = false;

      logger::log("imgui initialized");

      return NVE_SUCCESS;
}

void imgui_error_handle(VkResult err)
{
      throw std::runtime_error("graphical user interface error: " + err);
}
NVE_RESULT Renderer::imgui_create_render_pass()
{
      VkAttachmentDescription attachment = {};
      attachment.format = m_swapchainImageFormat;
      attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentReference colorAttachment = {};
      colorAttachment.attachment = 0;
      colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachment;

      // an external subpass dependency, so that the main subpass with the geometry is rendered before the gui
      // this is basically a driver-handled pipeline barrier
      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo renderPassCI = {};
      renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassCI.attachmentCount = 1;
      renderPassCI.pAttachments = &attachment;
      renderPassCI.subpassCount = 1;
      renderPassCI.pSubpasses = &subpass;
      renderPassCI.dependencyCount = 1;
      renderPassCI.pDependencies = &dependency;

      auto res = vkCreateRenderPass(m_device, &renderPassCI, nullptr, &m_imgui_renderPass);
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui render pass");

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_framebuffers()
{
      m_imgui_framebuffers.resize(m_swapchainImages.size());

      VkImageView attachment[1] = {};
      VkFramebufferCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      info.renderPass = m_imgui_renderPass;
      info.attachmentCount = 1;
      info.pAttachments = attachment;
      info.width = m_swapchainExtent.width;
      info.height = m_swapchainExtent.height;
      info.layers = 1;
      for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
      {
            attachment[0] = m_swapchainImageViews[i];
            auto res = vkCreateFramebuffer(m_device, &info, nullptr, &m_imgui_framebuffers[i]);
            logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui framebuffer no " + i);
      }

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_descriptor_pool()
{
      // array from https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
      VkDescriptorPoolSize poolSizes[] =
      {
          { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
          { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
          { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
          { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
          { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
          { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
      };

      VkDescriptorPoolCreateInfo descPoolCI = {};
      descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      descPoolCI.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
      descPoolCI.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
      descPoolCI.pPoolSizes = poolSizes;

      auto res = vkCreateDescriptorPool(m_device, &descPoolCI, nullptr, &m_imgui_descriptorPool);
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui descriptor pool");

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_upload_fonts()
{
      VkCommandBuffer commandBuffer = begin_single_time_cmd_buffer(m_imgui_commandPool, m_device);
      ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
      end_single_time_cmd_buffer(commandBuffer, m_imgui_commandPool, m_device, m_graphicsQueue);

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_command_pool()
{
      VkCommandPoolCreateInfo cmdPoolCI = {};
      cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      cmdPoolCI.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

      auto res = vkCreateCommandPool(m_device, &cmdPoolCI, nullptr, &m_imgui_commandPool);
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui command pool");

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_command_buffers()
{
      m_imgui_commandBuffers.resize(m_swapchainImages.size());

      VkCommandBufferAllocateInfo cmdBufferAI = {};
      cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdBufferAI.commandPool = m_imgui_commandPool;
      cmdBufferAI.commandBufferCount = static_cast<uint32_t>(m_swapchainImages.size());
      cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

      auto res = vkAllocateCommandBuffers(m_device, &cmdBufferAI, m_imgui_commandBuffers.data());
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui command buffers");

      return NVE_SUCCESS;
}

void Renderer::gui_begin()
{
      if (!m_imguiDraw && !m_acquireImageTimeout)
      {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            m_imguiDraw = true;
      }
}
void Renderer::draw_engine_gui(std::function<void(void)> guiDraw)
{
      if (m_acquireImageTimeout)
            return;
      gui_begin();

      m_guiManager.activate();
      m_guiManager.draw_entity_info();
      m_guiManager.draw_system_info();

      guiDraw();
}
void Renderer::imgui_draw(uint32_t imageIndex)
{
      //ImGui::ShowDemoWindow();

      ImGui::Render();

      {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            auto res = vkBeginCommandBuffer(m_imgui_commandBuffers[imageIndex], &info);
            logger::log_cond_err(res == VK_SUCCESS, "failed to begin imgui command buffer on image index " + imageIndex);
      }

      {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_imgui_renderPass;
            info.framebuffer = m_imgui_framebuffers[imageIndex];
            info.renderArea.extent.width = m_swapchainExtent.width;
            info.renderArea.extent.height = m_swapchainExtent.height;
            info.clearValueCount = 1;
            info.pClearValues = &m_imgui_clearColor;
            vkCmdBeginRenderPass(m_imgui_commandBuffers[imageIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
      }

      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_imgui_commandBuffers[imageIndex]);

      vkCmdEndRenderPass(m_imgui_commandBuffers[imageIndex]);
      auto res = vkEndCommandBuffer(m_imgui_commandBuffers[imageIndex]);
      logger::log_cond_err(res == VK_SUCCESS, "failed to end imgui command buffer no " + imageIndex);
}
void Renderer::imgui_cleanup()
{
      for (auto fb : m_imgui_framebuffers)
            vkDestroyFramebuffer(m_device, fb, nullptr);

      vkDestroyRenderPass(m_device, m_imgui_renderPass, nullptr);
      vkFreeCommandBuffers(m_device, m_imgui_commandPool, static_cast<uint32_t>(m_imgui_commandBuffers.size()), m_imgui_commandBuffers.data());
      vkDestroyCommandPool(m_device, m_imgui_commandPool, nullptr);

      // Resources to destroy when the program ends
      ImGui_ImplVulkan_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
      vkDestroyDescriptorPool(m_device, m_imgui_descriptorPool, nullptr);
}

#endif

// ---------------------------------------
// VERTEX
// ---------------------------------------

VkVertexInputBindingDescription Vertex::getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription = {};

      bindingDescription.binding = 0;
      bindingDescription.stride = sizeof(Vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> Vertex::getAttributeDescriptions() {
      std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> attributeDescriptions = {};

      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[0].offset = offsetof(Vertex, pos);

      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset = offsetof(Vertex, normal);

      attributeDescriptions[2].binding = 0;
      attributeDescriptions[2].location = 2;
      attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[2].offset = offsetof(Vertex, color);

      attributeDescriptions[3].binding = 0;
      attributeDescriptions[3].location = 3;
      attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[3].offset = offsetof(Vertex, uv);

      attributeDescriptions[4].binding = 0;
      attributeDescriptions[4].location = 4;
      attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
      attributeDescriptions[4].offset = offsetof(Vertex, material);

      return attributeDescriptions;
}

bool Vertex::operator== (const Vertex& other) const
{
      return pos == other.pos && normal == other.normal && color == other.color && uv == other.uv;
}

bool operator<(const Vector3& l, const Vector3& r)
{
      return
            l.x < r.x &&
            l.y < r.y &&
            l.z < r.z
            ;
}
bool operator<(const Vector2& l, const Vector2& r)
{
      return
            l.x < r.x &&
            l.y < r.y
            ;
}

bool Vertex::operator<(const Vertex& other)
{
      return
            pos < other.pos &&
            normal < other.normal &&
            color < other.color &&
            uv < other.uv &&
            material < other.material
            ;
}

// ---------------------------------------
// CAMERA
// ---------------------------------------
Camera::Camera() :
      m_position(0), m_rotation(0), m_fov(90), m_nearPlane(0.01f), m_farPlane(1000.f), m_extent(1080, 1920), renderer(nullptr), m_orthographic{ false }
{}
glm::mat4 Camera::view_matrix()
{
      if (!m_orthographic)
            return glm::lookAt(m_position, m_position + glm::rotate(glm::qua(glm::radians(m_rotation)), VECTOR_FORWARD), VECTOR_DOWN);

      float zoom = 1.f / math::max(0.001f, m_position.z + 1);
      float ratio = m_extent.y / m_extent.x;
      return glm::transpose(glm::mat4x4(
            zoom * ratio, 0, 0, m_position.x * ratio,
            0, zoom, 0, m_position.y,
            0, 0, 1, 0,
            0, 0, 0, 1
      ));
}
glm::mat4 Camera::projection_matrix()
{
      if (!m_orthographic)
            return glm::perspective(glm::radians(m_fov), m_extent.x / m_extent.y, m_nearPlane, m_farPlane);
      return glm::transpose(glm::mat4x4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1
      ));
}

#undef PROFILE_START
#undef PROFILE_END
#undef PROFILE_LABEL
#undef PROFILE_LABEL_END
