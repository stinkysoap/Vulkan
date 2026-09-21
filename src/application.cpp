#include "application.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>
void Application::run() {
  running = true;

  while (running) {

    SDL_Event event{0};

    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
        break;

      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        width = event.window.data1;
        height = event.window.data2;

        break;
      }
    }
    render();
  }
}

bool Application::initialize() {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    window = SDL_CreateWindow("Vulkan Learning", width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
      showError("Error Creating Window");
      return false;
    }
    if (!initializeVulkan()) {
      showError("Cannot Initialze Vulkan");
      return false;
    }
  } else {
    showError("Unable to initilize SDL3 ");
    return false;
  }
  return true;
}

bool Application::initializeVulkan() {
  if (!createVulkanInstance()) {
    showError("Couldnt Create A Vulkan Instance ");
  }
  return true;
}

bool Application::createVulkanInstance() {
  if (volkInitialize() != VK_SUCCESS) {
    showError("Error Initlizing Volk");
    return false;
  }
  // create Vulkan Application Instance
  VkApplicationInfo appInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "My First Triangle",
      .apiVersion = VulkanVersion,
  };
  // find the required extensions for the platform and
  //  add debug for ourselves
  uint32_t instExtCount = 0;
  const char *const *extensions =
      SDL_Vulkan_GetInstanceExtensions(&instExtCount);
  std::vector<const char *> requestedExtensions{
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

  for (int i = 0; i < instExtCount; ++i) {
    requestedExtensions.push_back(extensions[i]);
  }
  for (const char *ext : requestedExtensions) {
    std::cout << ext << std::endl;
  }
  return true;
}
