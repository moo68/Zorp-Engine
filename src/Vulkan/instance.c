#include "ZorpEngine/Vulkan/instance.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


/*
 * Return a filled out VkInstance struct.
 */
VkInstance create_instance() {
    VkInstance instance = {0};

    VkApplicationInfo app_info = create_app_info();

    /*
     * This struct, when passed to the instance, enables extra validation
     * layers. 
     */
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {0};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    VkInstanceCreateInfo instance_info =
        create_instance_info(&app_info, &debug_info);

    if (vkCreateInstance(&instance_info, NULL, &instance) != VK_SUCCESS) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create Vulkan instance!");
        abort();
    }

    /*
     * Inside create_instance_info(), SDL_malloc is used for extension names.
     * Now that the instance is created, we don't need those names on the heap
     * anymore.
     */
    SDL_free((void *)instance_info.ppEnabledExtensionNames);

    return instance;
}

/*
 * Return a filled out VkApplicationInfo struct. Basically just bookkeeping.
 */
VkApplicationInfo create_app_info(void) {
    VkApplicationInfo app_info = {0};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ZorpEngine";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "ZorpEngine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pNext = NULL;

    return app_info;
}

/*
 * Return a filled out VkInstanceCreateInfo struct. This sets up extensions
 * and layers.
 */
VkInstanceCreateInfo create_instance_info(VkApplicationInfo *app_info,
    VkDebugUtilsMessengerCreateInfoEXT *debug_info) {

    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = app_info;

    // Get the number of extensions for the Vulkan instance.
    uint32_t instance_extension_count = 0;
    const char * const *instance_extensions =
        SDL_Vulkan_GetInstanceExtensions(&instance_extension_count);
    if (instance_extensions == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
        SDL_ClearError();
        abort();
    }

    /*
     * Actually set the app_info struct's extensions. Note that we're manually
     * adding an extra extension at the beginning of the extensions array.
     */
    uint32_t extension_count = instance_extension_count + 1;
    const char **extensions = SDL_malloc(extension_count *
                                         sizeof(const char *));
    extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    SDL_memcpy(&extensions[1], instance_extensions,
               instance_extension_count * sizeof(const char *));

    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    // If using validation layers, pNext should use the given debug info.
    if (enable_validation_layers == true) {
        instance_info.enabledLayerCount = validation_layer_count;
        instance_info.ppEnabledLayerNames = validation_layers;
        instance_info.pNext = debug_info;
    }
    else {
        instance_info.enabledLayerCount = 0;
        instance_info.ppEnabledLayerNames = NULL;
        instance_info.pNext = NULL;
    }

    return instance_info;
}

/*
 * Return whether or not all the validation layers specified in the global
 * validation_layers array are actually supported.
 */
bool are_validation_layers_supported(void) {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    if (layer_count == 0) {
        return true;
    }

    VkLayerProperties *available_layers = malloc(layer_count *
                                                 sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (int i = 0; i < validation_layer_count; i++) {
        bool layer_found = false;
        const char *curr_validation_layer = validation_layers[i];

        for (int j = 0; j < (int)layer_count; j++) {
            VkLayerProperties curr_available_layer = available_layers[j];
            if (strcmp(curr_validation_layer, curr_available_layer.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (layer_found == false) {
            return false;
        }
    }

    free(available_layers);

    return true;
}

