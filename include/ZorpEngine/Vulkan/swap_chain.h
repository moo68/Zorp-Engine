#ifndef VULKAN_SWAP_CHAIN_H
#define VULKAN_SWAP_CHAIN_H

#include "ZorpEngine/Vulkan/physical_device.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>


typedef struct {
    VkSwapchainKHR swap_chain_KHR;

    VkImage *images;
    uint32_t image_count;

    VkImageView *image_views;
    uint32_t image_view_count;

    VkFormat image_format;
    VkExtent2D extent;
} Swapchain;


Swapchain create_swap_chain(SwapChainSupportDetails swap_chain_support, 
                                 VkDevice device, VkSurfaceKHR surface,
                                 SDL_Window *window,
                                 QueueFamilyIndices queue_families);

VkSwapchainCreateInfoKHR create_swap_chain_info(SwapChainSupportDetails swap_chain_support,
                                                VkSurfaceKHR surface,
                                                VkSurfaceFormatKHR surface_format,
                                                VkPresentModeKHR present_mode,
                                                VkExtent2D extent,
                                                uint32_t image_count,
                                                QueueFamilyIndices queue_families);

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR *formats,
                                              uint32_t num_formats);

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes,
                                          uint32_t num_present_modes);

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities,
                              SDL_Window *window);

void create_image_views(VkDevice device, Swapchain *swap_chain);

void destroy_swapchain(Swapchain *swap_chain, VkDevice device);

#endif

