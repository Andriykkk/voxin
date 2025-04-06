#define GLFW_INCLUDE_VULKAN
#include <vector>

#include "globals.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char *TITLE = "Voxin";

const std::vector<const char *> validationLayers = {
    // "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
