#include "ZorpEngine/Vulkan/logical_device.h"
#include "ZorpEngine/Vulkan/physical_device.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL_log.h>

#include <string.h>
#include <stdlib.h>


VkDevice create_logical_device(VkPhysicalDevice physical_device, QueueFamilyIndices indices) {
    VkDevice device = {0};

    VkDeviceQueueCreateInfo queue_creation_infos[2] = {0};
    float queue_priority = 1.0f;
    uint32_t queue_creation_info_count = 0;

    // TODO: This should be more robust once we need to handle more than 2 queues.
    if (indices.graphics_family == indices.present_family) {
        // If the graphics queue and the present queue are the same, just use it.
        queue_creation_infos[0] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = indices.graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
        queue_creation_info_count = 1;
    }
    else {
        // If the graphics queue and the present queue are different, create different queues.
        queue_creation_infos[0] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = indices.graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
        queue_creation_infos[1] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = indices.present_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
        queue_creation_info_count = 2;
    }

    // Enable dynamic rendering (and any other needed features).
    VkPhysicalDeviceVulkan13Features vulkan_13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = NULL,
        .dynamicRendering = VK_TRUE
    };
    VkPhysicalDeviceFeatures2 device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan_13_features,
        .features = {0} // For Vulkan 1.0 features (i.e. geometry shader).
    };

    // Create the device info struct.
    VkDeviceCreateInfo device_creation_info = {0};
    device_creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_creation_info.pQueueCreateInfos = queue_creation_infos;
    device_creation_info.queueCreateInfoCount = queue_creation_info_count;
    device_creation_info.pEnabledFeatures = NULL; // Using pNext for features.
    device_creation_info.enabledExtensionCount = (uint32_t)device_extension_count;
    device_creation_info.ppEnabledExtensionNames = device_extensions;
    device_creation_info.enabledLayerCount = 0;
    device_creation_info.ppEnabledLayerNames = NULL;
    device_creation_info.pNext = &device_features;

    // Actually create the logical device.
    if (vkCreateDevice(physical_device, &device_creation_info, NULL, &device) != VK_SUCCESS) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create Vulkan logical device!");
        abort();
    }

    return device;
}

