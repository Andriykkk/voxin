#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <string.h>

const int WIDTH = 800;
const int HEIGHT = 600;

struct VulkanContext
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
};
void createInstance(VulkanContext &context)
{
    // Get required extensions
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Black Screen";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    vkCreateInstance(&createInfo, nullptr, &context.instance);
}

// Modified device selection and creation
void pickPhysicalDevice(VulkanContext &context)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        // Check for swapchain extension support
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        for (const auto &extension : availableExtensions)
        {
            if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            {
                context.physicalDevice = device;
                return;
            }
        }
    }

    throw std::runtime_error("Failed to find suitable physical device!");
}

void createLogicalDevice(VulkanContext &context)
{
    // Device extensions
    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0; // Should properly find graphics queue family
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(context.device, 0, 0, &context.graphicsQueue);
}
void createSurface(VulkanContext &context, GLFWwindow *window)
{
    glfwCreateWindowSurface(context.instance, window, nullptr, &context.surface);
}

void createSwapChain(VulkanContext &context, GLFWwindow *window)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    context.swapChainImageFormat = surfaceFormat.format;
    context.swapChainExtent = capabilities.currentExtent;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context.surface;
    createInfo.minImageCount = capabilities.minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = context.swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapChain);

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, nullptr);
    context.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, context.swapChainImages.data());
}

void createImageViews(VulkanContext &context)
{
    context.swapChainImageViews.resize(context.swapChainImages.size());
    for (size_t i = 0; i < context.swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = context.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = context.swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(context.device, &createInfo, nullptr, &context.swapChainImageViews[i]);
    }
}

void createRenderPass(VulkanContext &context)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = context.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &context.renderPass);
}

void createFramebuffers(VulkanContext &context)
{
    context.swapChainFramebuffers.resize(context.swapChainImageViews.size());
    for (size_t i = 0; i < context.swapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {context.swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = context.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = context.swapChainExtent.width;
        framebufferInfo.height = context.swapChainExtent.height;
        framebufferInfo.layers = 1;

        vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &context.swapChainFramebuffers[i]);
    }
}

void createCommandPool(VulkanContext &context)
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    poolInfo.flags = 0;

    vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool);
}

void createCommandBuffer(VulkanContext &context)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(context.device, &allocInfo, &context.commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(context.commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = context.renderPass;
    renderPassInfo.framebuffer = context.swapChainFramebuffers[0];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = context.swapChainExtent;
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(context.commandBuffer);

    vkEndCommandBuffer(context.commandBuffer);
}

void createSemaphores(VulkanContext &context)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.imageAvailableSemaphore);
    vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.renderFinishedSemaphore);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Black Screen", nullptr, nullptr);

    VulkanContext context;

    createInstance(context);
    createSurface(context, window);
    pickPhysicalDevice(context);
    createLogicalDevice(context);
    createSwapChain(context, window);
    createImageViews(context);
    createRenderPass(context);
    createFramebuffers(context);
    createCommandPool(context);
    createCommandBuffer(context);
    createSemaphores(context);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        uint32_t imageIndex;
        vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX,
                              context.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &context.commandBuffer;

        vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &context.swapChain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(context.graphicsQueue, &presentInfo);
        vkDeviceWaitIdle(context.device);
    }

    vkDeviceWaitIdle(context.device);
    vkDestroySemaphore(context.device, context.renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(context.device, context.imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(context.device, context.commandPool, nullptr);
    for (auto framebuffer : context.swapChainFramebuffers)
    {
        vkDestroyFramebuffer(context.device, framebuffer, nullptr);
    }
    vkDestroyRenderPass(context.device, context.renderPass, nullptr);
    for (auto imageView : context.swapChainImageViews)
    {
        vkDestroyImageView(context.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(context.device, context.swapChain, nullptr);
    vkDestroyDevice(context.device, nullptr);
    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    vkDestroyInstance(context.instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}