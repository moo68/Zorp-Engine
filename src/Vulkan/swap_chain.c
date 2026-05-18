#include "ZorpEngine/Vulkan/swap_chain.h"
#include "ZorpEngine/Vulkan/physical_device.h"

#include "vulkan/vulkan.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_log.h"

#include <stdlib.h>


/*
 * Return a filled out Swapchain struct. The Swapchain struct should contain all
 * the necessary information/data about the swapchain.
 */
Swapchain create_swap_chain(SwapChainSupportDetails swap_chain_support, 
                                 VkDevice device, VkSurfaceKHR surface,
                                 SDL_Window *window,
                                 QueueFamilyIndices queue_families) {
    // Get basic info about the swap chain.
    VkSurfaceFormatKHR surface_format =
        choose_swap_surface_format(swap_chain_support.formats,
                                   swap_chain_support.num_formats);

    VkPresentModeKHR present_mode =
        choose_swap_present_mode(swap_chain_support.present_modes,
                                 swap_chain_support.num_present_modes);

    VkExtent2D extent = choose_swap_extent(&(swap_chain_support.capabilities),
                                           window);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if ((swap_chain_support.capabilities.maxImageCount > 0) &&
        (image_count > swap_chain_support.capabilities.maxImageCount)) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    // Create swap chain info.
    VkSwapchainCreateInfoKHR swap_chain_info =
        create_swap_chain_info(swap_chain_support, surface, surface_format,
                               present_mode, extent, image_count,
                               queue_families);

    // Actually create the swap chain itself.
    VkSwapchainKHR swap_chain_KHR = {0};
    if (vkCreateSwapchainKHR(device, &swap_chain_info, NULL, &swap_chain_KHR) != VK_SUCCESS) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create Vulkan swap chain!");
        abort();
    }

    // Create swap chain images.
    VkImage *swap_chain_images = NULL;
    uint32_t swap_chain_image_count = 0;

    vkGetSwapchainImagesKHR(device, swap_chain_KHR, &swap_chain_image_count, NULL);
    swap_chain_images = malloc(swap_chain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swap_chain_KHR, &swap_chain_image_count, swap_chain_images);

    // Create the final swapchain struct. Note that this doesn't include image views.
    Swapchain swap_chain = {
        .swap_chain_KHR = swap_chain_KHR,
        .images = swap_chain_images,
        .image_count = swap_chain_image_count,
        .image_views = NULL,
        .image_view_count = 0,
        .image_format = surface_format.format,
        .extent = extent
    };

    // Create the image views.
    create_image_views(device, &swap_chain);

    return swap_chain;
}


/*
 * Return a filled out VkSwapchainCreateInfoKHR struct.
 */
VkSwapchainCreateInfoKHR create_swap_chain_info(SwapChainSupportDetails swap_chain_support,
                                                VkSurfaceKHR surface,
                                                VkSurfaceFormatKHR surface_format,
                                                VkPresentModeKHR present_mode,
                                                VkExtent2D extent,
                                                uint32_t image_count,
                                                QueueFamilyIndices queue_families) {
    VkSwapchainCreateInfoKHR swap_chain_info = {0};

    swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_info.surface = surface;
    swap_chain_info.minImageCount = image_count;
    swap_chain_info.imageFormat = surface_format.format;
    swap_chain_info.imageColorSpace = surface_format.colorSpace;
    swap_chain_info.imageExtent = extent;
    swap_chain_info.imageArrayLayers = 1;
    swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[2] = {queue_families.graphics_family,
                                        queue_families.present_family};
    if (queue_families.graphics_family != queue_families.present_family) {
        swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_info.queueFamilyIndexCount = 2;
        swap_chain_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swap_chain_info.queueFamilyIndexCount = 0;
        swap_chain_info.pQueueFamilyIndices = NULL;
    }

    swap_chain_info.preTransform =
        swap_chain_support.capabilities.currentTransform;
    swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_info.presentMode = present_mode;
    swap_chain_info.clipped = VK_TRUE;
    swap_chain_info.oldSwapchain = VK_NULL_HANDLE;

    return swap_chain_info;
}

/*
 * Return the best possible swap chain surface format available.
 */
VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR *formats, uint32_t num_formats) {
    for (int i = 0; i < (int)num_formats; i++) {
        VkSurfaceFormatKHR curr_format = formats[i];
        if ((curr_format.format == VK_FORMAT_B8G8R8A8_SRGB) &&
            (curr_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return curr_format;
        }
    }

    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Coudln't find ideal swapchain surface format!");
    return formats[0];
}

/*
 * Return the best possible swap chain present mode possible.
 */
VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes, uint32_t num_present_modes) {
    for (int i = 0; i < (int)num_present_modes; i++) {
        VkPresentModeKHR curr_present_mode = present_modes[i];
        if (curr_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return curr_present_mode;
        }
    }
 
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Coudln't find ideal swapchain present mode!");
    return VK_PRESENT_MODE_FIFO_KHR;
}

/*
 * Return the extent of the given surface for use in the swap chain.
 */
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window) {
    // If Vulkan alrady figured out the extent, just return it.
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    // Make a new VkExtent2D struct.
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    VkExtent2D actual_extent = {
        .width = (uint32_t)width,
        .height = (uint32_t)height
    };

    // Clamp the struct's values if need be.
    if (actual_extent.width < capabilities->minImageExtent.width) {
        actual_extent.width = capabilities->minImageExtent.width;
    }
    if (actual_extent.width > capabilities->maxImageExtent.width) {
        actual_extent.width = capabilities->maxImageExtent.width;
    }
    if (actual_extent.height < capabilities->minImageExtent.height) {
        actual_extent.height = capabilities->minImageExtent.height;
    }
    if (actual_extent.height > capabilities->maxImageExtent.height) {
        actual_extent.height = capabilities->maxImageExtent.height;
    }

    return actual_extent;
}

/*
 * Create all the necessary image views for swap chain images. This just
 * directly fills out the given Swapchain struct.
 */
void create_image_views(VkDevice device, Swapchain *swap_chain) {
    swap_chain->image_view_count = swap_chain->image_count;
    swap_chain->image_views = malloc(swap_chain->image_view_count * sizeof(VkImageView));
    
    for (int i = 0; i < (int)swap_chain->image_count; i++) {
        VkImageViewCreateInfo curr_image_view_creation_info = {0};

        curr_image_view_creation_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        curr_image_view_creation_info.image = swap_chain->images[i];
        curr_image_view_creation_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        curr_image_view_creation_info.format = swap_chain->image_format;

        curr_image_view_creation_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        curr_image_view_creation_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        curr_image_view_creation_info.subresourceRange.baseMipLevel = 0;
        curr_image_view_creation_info.subresourceRange.levelCount = 1;
        curr_image_view_creation_info.subresourceRange.baseArrayLayer = 0;
        curr_image_view_creation_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &curr_image_view_creation_info, NULL, &swap_chain->image_views[i]) != VK_SUCCESS) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create Vulkan image views!");
            abort();
        }
    }
}

void destroy_swapchain(Swapchain *swap_chain, VkDevice device) {
    for (int i = 0; i < (int)swap_chain->image_view_count; i++) {
        vkDestroyImageView(device, swap_chain->image_views[i], NULL);
    }

    free(swap_chain->image_views);
    free(swap_chain->images);

    vkDestroySwapchainKHR(device, swap_chain->swap_chain_KHR, NULL);
}

