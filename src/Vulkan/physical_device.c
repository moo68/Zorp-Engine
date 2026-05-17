#include "ZorpEngine/Vulkan/physical_device.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL_log.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


/*
 * Return the best available physical device.
 */
// TODO: We should prefer physical devices where the graphics family and present family are the same.
VkPhysicalDevice pick_physical_device(VkInstance instance,
                                      VkSurfaceKHR surface) {
    VkPhysicalDevice best_physical_device = VK_NULL_HANDLE;
    int best_device_score = 0;

    // Get the number of available physical devices.
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to find any physical devices!");
        abort();
    }

    VkPhysicalDevice *devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    /* 
     * Iterate through all available devices, checking if they're suitable. If
     * a device is suitable, give it a score. The device with the highest score
     * gets returned.
     */
    for (int i = 0; i < (int)device_count; i++) {
        VkPhysicalDevice curr_device = devices[i];
        int curr_device_score = 0;

        VkPhysicalDeviceProperties curr_device_properties = {0};
        vkGetPhysicalDeviceProperties(curr_device, &curr_device_properties);
        // TODO: Check for dynamic rendering features!!!
        /*VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
            .pNext = NULL,
        };
        VkPhysicalDeviceVulkan13Features vulkan_13_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &extended_dynamic_state_features,
        };
        VkPhysicalDeviceFeatures2 features2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &vulkan_13_features,
        };
        vkGetPhysicalDeviceFeatures2(curr_device, &features2);*/

        if (is_device_suitable(curr_device, surface,
                               curr_device_properties) == false) {
            continue;
        }

        if (curr_device_properties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            curr_device_score += 1000;
        }

        if (curr_device_score > best_device_score) {
            best_device_score = curr_device_score;
            best_physical_device = curr_device;
        }
    }

    if (best_physical_device == VK_NULL_HANDLE) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to find a suitable physical device!");
        abort();
    }

    free(devices);

    return best_physical_device;
}

/*
 * Determine whether or not the given device is suitable. If a device isn't
 * suitable, it isn't fit to run the program at all.
 */
bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                        VkPhysicalDeviceProperties properties) {
    QueueFamilyIndices family_indices = find_queue_families(device, surface);

    if (properties.apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    if (family_indices.graphics_family_found == false) {
        return false;
    }

    if (check_device_extension_support(device) == false) {
        return false;
    }
    else {
        SwapChainSupportDetails swap_chain_support =
            query_swap_chain_support(device, surface);
        if ((swap_chain_support.num_formats == 0) ||
            (swap_chain_support.num_present_modes == 0)) {
            return false;
        }
    }

    return true;
}

/*
 * Return a QueueFamilyIndices struct that contains the index values for the
 * necessary graphics and present queue families of the given device.
 */
QueueFamilyIndices find_queue_families(VkPhysicalDevice device,
                                       VkSurfaceKHR surface) {
    QueueFamilyIndices indices = {0};

    // Get the number of families and each family's struct.
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families =
        malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families);

    // Iterate through the array of families and set indices accordingly.
    for (int i = 0; i < (int)queue_family_count; i++) {
        VkQueueFamilyProperties curr_queue_family = queue_families[i];
        if (curr_queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.graphics_family_found = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &present_support);
        if (present_support == true) {
            indices.present_family = i;
            indices.present_family_found = true;
        }
    }

    free(queue_families);

    return indices;
}

/*
 * Return whether or not the given device supports the globally-initialized
 * list of required device extensions.
 */
bool check_device_extension_support(VkPhysicalDevice device) {
    // Get the number of each available extension and the extension itself.
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    if (extension_count == 0) {
        return false;
    }
    VkExtensionProperties *available_extensions =
        malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count,
                                         available_extensions);

    /* 
     * For each global, required extension, check if there's a matching
     * available extension.
     */
    for (int i = 0; i < device_extension_count; i++) {
        bool extension_found = false;
        const char *curr_extension = device_extensions[i];

        for (int j = 0; j < (int)extension_count; j++) {
            VkExtensionProperties curr_available_extension =
                available_extensions[j];
            if (strcmp(curr_extension, curr_available_extension.extensionName) == 0) {
                extension_found = true;
                break;
            }
        }

        if (extension_found == false) {
            return false;
        }
    }

    free(available_extensions);

    return true;
}

/*
 * Return a SwapChainSupportDetails struct containing info about the given
 * device's possible swapchain.
 */
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device,
                                                 VkSurfaceKHR surface) {
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &(details.capabilities));

    // Get the swap chain's formats.
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
    if (format_count != 0) {
        details.num_formats = format_count;
        details.formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                             details.formats);
    }

    // Get the swap chain's present modes.
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &present_mode_count, NULL);
    if (present_mode_count != 0) {
        details.num_present_modes = present_mode_count;
        details.present_modes = malloc(present_mode_count *
                                       sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                                  &present_mode_count,
                                                  details.present_modes);
    }

    return details;
}

