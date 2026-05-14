#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>

#include <stdbool.h>


extern const bool enable_validation_layers;
extern const int validation_layer_count;
extern const char *validation_layers[];


VkInstance create_instance();

VkApplicationInfo create_app_info(void);

VkInstanceCreateInfo create_instance_info(VkApplicationInfo *app_info,
    VkDebugUtilsMessengerCreateInfoEXT *debug_info);

bool are_validation_layers_supported(void);

#endif

