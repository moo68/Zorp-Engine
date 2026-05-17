#ifndef VULKAN_LOGICAL_DEVICE_H 
#define VULKAN_LOGICAL_DEVICE_H

#include <vulkan/vulkan.h>

#include "ZorpEngine/Vulkan/physical_device.h"


extern const int device_extension_count;
extern const char *device_extensions[]; 


VkDevice create_logical_device(VkPhysicalDevice physical_device, QueueFamilyIndices indices);

#endif
