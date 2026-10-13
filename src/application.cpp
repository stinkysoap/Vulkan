#include "application.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstddef>
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

bool Application::createSurface() {
  if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &surface)) {
    return false;
  }
  return true;
}

void Application::shutdown() {
  // there is more stuff
  if (vulkanInstance) {
    vkDestroyInstance(vulkanInstance, nullptr);
  }

  volkFinalize();

  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
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
  if (!createSurface()) {
    showError("Couldnt Create a window surface ");
    return false;
  }
  if (physicalDevice = findPhysicalDevice(); !physicalDevice) {
    showError("Unable to find an appropriate physical Device");
  }
  if (!findGraphicsQueue()) {
    showError("Unable to find a compatible graphics queue ");
    return false;
  }
  if (!createDevice(physicalDevice)) {
    showError("Couldnt Create the logical GPU Device");
    return false;
  }
  return true;
}
bool Application::createDevice(VkPhysicalDevice physicalDevice) {
  // query supported features
  VkPhysicalDeviceVulkan14Features supportedFeatures14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features supportedFeatures13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &supportedFeatures14};
  VkPhysicalDeviceVulkan12Features supportedFeatures12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &supportedFeatures13};
  VkPhysicalDeviceFeatures2 supportedFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &supportedFeatures12};
  vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);

  // check if what we need is supported
  if (!supportedFeatures13.dynamicRendering ||
      !supportedFeatures13.synchronization2 ||
      !supportedFeatures12.timelineSemaphore) {
    showError("Physical Device Doesnt Meet the feature requirements ");
    return false;
  }
  // produce a separte features struct chain for device creation
  VkPhysicalDeviceVulkan14Features features14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr,
  };
  VkPhysicalDeviceVulkan13Features features13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &features14,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &features13,
      .timelineSemaphore = VK_TRUE};
  VkPhysicalDeviceFeatures2 features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &features12};

  // Request the queues we will be using
  std::vector<float> queuePriotities{1.0f};
  VkDeviceQueueCreateInfo gfxQueueInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = gfxQueueFamIdx,
      .queueCount = 1,
      .pQueuePriorities = queuePriotities.data()};
  // device specific extenstions
  const std::vector<const char *> deviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo devCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &gfxQueueInfo,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = nullptr};
  if (vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device) !=
      VK_SUCCESS) {
    return false;
  }
  vkGetDeviceQueue(device, gfxQueueFamIdx, 0, &gfxQueue);
  if (!gfxQueue) {
    showError("Coudlnt get the graphics queue ");
    return false;
  }
  return true;
}

VkPhysicalDevice Application::findPhysicalDevice() {
  uint32_t physDeviceCount = 0;
  vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, nullptr);
  std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
  vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount,
                             physicalDevices.data());
  VkPhysicalDevice physicalDevice = nullptr;
  if (physDeviceCount) {
    physicalDevice = physicalDevices[0];
    for (auto &pdev : physicalDevices) {

      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(pdev, &props);
      if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        physicalDevice = pdev;
        break;
      }
    }
  }
  return physicalDevice;
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
  std::vector<const char *> requestedLayers{"VK_LAYER_KHRONOS_validation"};

  VkDebugUtilsMessengerCreateInfoEXT debugInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback};

  VkInstanceCreateInfo instCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = &debugInfo,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(requestedLayers.size()),
      .ppEnabledLayerNames = requestedLayers.data(),
      .enabledExtensionCount =
          static_cast<uint32_t>(requestedExtensions.size()),
      .ppEnabledExtensionNames = requestedExtensions.data()};
  if (vkCreateInstance(&instCreateInfo, nullptr, &vulkanInstance) !=
      VK_SUCCESS) {
    return false;
  }
  volkLoadInstance(vulkanInstance);

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

void Application::showError(const std::string &errorMessasge) const {
  std::cerr << "[Error]: " << errorMessasge << std::endl;
}

bool Application::findGraphicsQueue() {
  uint32_t queueFamCount = 0;

  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount,
                                            nullptr);
  std::vector<VkQueueFamilyProperties2> queueFamProps(
      queueFamCount, {.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount,
                                            queueFamProps.data());
  for (int currentFamIdx = 0; currentFamIdx < queueFamProps.size();
       currentFamIdx++) {
    /// check if it has Presentation support
    VkBool32 hasPresentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, currentFamIdx, surface,
                                         &hasPresentSupport);
    const auto &props = queueFamProps[currentFamIdx];
    // ensure this is a Graphics queue with Presentation support
    if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        hasPresentSupport) {
      gfxQueueFamIdx = currentFamIdx;
      return true;
    }
  }
  return false;
}

void Application::render() { return; }
