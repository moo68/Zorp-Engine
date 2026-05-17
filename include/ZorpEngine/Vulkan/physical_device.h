#ifndef VULKAN_PHYSICAL_DEVICE_H
#define VULKAN_PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>

#include <stdbool.h>


typedef struct {
    uint32_t graphics_family;
    bool graphics_family_found;

    uint32_t present_family;
    bool present_family_found;
} QueueFamilyIndices;

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceFormatKHR *formats;
    uint32_t num_formats;

    VkPresentModeKHR *present_modes;
    uint32_t num_present_modes;
} SwapChainSupportDetails;


extern const int device_extension_count;
extern const char *device_extensions[];


VkPhysicalDevice pick_physical_device(VkInstance instance,
                                      VkSurfaceKHR surface);

bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                        VkPhysicalDeviceProperties properties);

QueueFamilyIndices find_queue_families(VkPhysicalDevice device,
                                       VkSurfaceKHR surface);

bool check_device_extension_support(VkPhysicalDevice device);

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device,
                                                 VkSurfaceKHR surface);

#endif

