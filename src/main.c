// REMINDER: RE_ENABLE -Werror soon!!!!
#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>

#include "ZorpEngine/shader_utils.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>


// Structs:
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


// Global variables:
const int num_validation_layers = 1;
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

const int num_device_extensions = 1;
const char *device_extensions[] = {
    "VK_KHR_swapchain"
};

VkImage *swap_chain_images = NULL;
uint32_t num_swap_chain_images = 0;
VkFormat swap_chain_image_format = {0};
VkExtent2D swap_chain_extent = {0};

VkImageView *swap_chain_image_views = NULL;
uint32_t num_swap_chain_image_views = 0;

VkRenderPass render_pass = {0};
VkPipelineLayout pipeline_layout = {0};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif


// Function prototypes:
VkApplicationInfo create_app_info(void);

VkInstanceCreateInfo create_instance_creation_info(
    VkApplicationInfo *app_info,
    VkDebugUtilsMessengerCreateInfoEXT *debug_creation_info);

bool are_validation_layers_supported(void);

bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

bool check_device_extension_support(VkPhysicalDevice device);

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR *formats, uint32_t num_formats);

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes, uint32_t num_present_modes);

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window);

void create_image_views();

VkShaderModule create_shader_module(VkDevice device, const char *code, size_t size);

void create_render_pass(VkRenderPass *render_pass);


// Function definitions:
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        return -1;
    }

    // TODO: Insert SDL_SetAppMetadata here.

    // Create a window.
    SDL_Window *window = SDL_CreateWindow("ZorpEngine", 1200, 800, SDL_WINDOW_VULKAN);
    if (window == NULL) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        return -1;
    }

    // Create a Vulkan instance.
    VkInstance instance = {0};
    VkApplicationInfo app_info = create_app_info();
    VkDebugUtilsMessengerCreateInfoEXT debug_creation_info = {0};
    debug_creation_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    VkInstanceCreateInfo creation_info = create_instance_creation_info(&app_info, &debug_creation_info);

    if (vkCreateInstance(&creation_info, NULL, &instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance!\n");
    }

    // TODO: Insert all the stuff to enumerate through extensions here.

    // Check validation layers.
    if ((enable_validation_layers == true) && (are_validation_layers_supported() == false)) {
        fprintf(stderr, "Could not properly find all Vulkan validation layers!\n");
        return -1;
    }
    
    // TODO: Setup custom validation layer callback to filter specific errors.

    // Create a window surface.
    VkSurfaceKHR surface = {0};
    if (SDL_Vulkan_CreateSurface(window, instance, NULL, &surface) == false) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        return -1;
    }

    // Choose a physical device.
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "Failed to find Vulkan-compatible GPU!\n");
        return -1;
    }

    VkPhysicalDevice *devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    for (int i = 0; i < (int)device_count; i++) {
        VkPhysicalDevice curr_device = devices[i];
        if (is_device_suitable(curr_device, surface) == true) {
            physical_device = curr_device;
            break;
        }
    }
    if (physical_device == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable GPU!\n");
        return -1;
    }

    // Create a logical device.
    VkDevice device = {0};

    QueueFamilyIndices indices = find_queue_families(physical_device, surface);
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

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo device_creation_info = {0};
    device_creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_creation_info.pQueueCreateInfos = queue_creation_infos;
    device_creation_info.queueCreateInfoCount = queue_creation_info_count;
    device_creation_info.pEnabledFeatures = &device_features;
    device_creation_info.enabledExtensionCount = (uint32_t)num_device_extensions;
    device_creation_info.ppEnabledExtensionNames = device_extensions;
    if (enable_validation_layers == true) {
        device_creation_info.enabledLayerCount = (uint32_t)num_validation_layers;
        device_creation_info.ppEnabledLayerNames = validation_layers;
    }
    else {
        device_creation_info.enabledLayerCount = 0;
        device_creation_info.ppEnabledLayerNames = NULL;
    }

    if (vkCreateDevice(physical_device, &device_creation_info, NULL, &device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a logical device!\n");
        return -1;
    }

    // Create queue handles.
    VkQueue graphics_queue = {0};
    vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);

    VkQueue present_queue = {0};
    vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);

    // Create the swap chain.
    SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device, surface);
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats, swap_chain_support.num_formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes, swap_chain_support.num_present_modes);
    VkExtent2D extent = choose_swap_extent(&(swap_chain_support.capabilities), window);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if ((swap_chain_support.capabilities.maxImageCount > 0) && (image_count > swap_chain_support.capabilities.maxImageCount)) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swap_chain_creation_info = {0};
    swap_chain_creation_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_creation_info.surface = surface;
    swap_chain_creation_info.minImageCount = image_count;
    swap_chain_creation_info.imageFormat = surface_format.format;
    swap_chain_creation_info.imageColorSpace = surface_format.colorSpace;
    swap_chain_creation_info.imageExtent = extent;
    swap_chain_creation_info.imageArrayLayers = 1;
    swap_chain_creation_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[2] = {indices.graphics_family, indices.present_family};
    if (indices.graphics_family != indices.present_family) {
        swap_chain_creation_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_creation_info.queueFamilyIndexCount = 2;
        swap_chain_creation_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        swap_chain_creation_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swap_chain_creation_info.queueFamilyIndexCount = 0;
        swap_chain_creation_info.pQueueFamilyIndices = NULL;
    }

    swap_chain_creation_info.preTransform = swap_chain_support.capabilities.currentTransform;
    swap_chain_creation_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_creation_info.presentMode = present_mode;
    swap_chain_creation_info.clipped = VK_TRUE;
    swap_chain_creation_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swap_chain = {0};
    if (vkCreateSwapchainKHR(device, &swap_chain_creation_info, NULL, &swap_chain) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a swapchain!\n");
        return -1;
    }

    vkGetSwapchainImagesKHR(device, swap_chain, &num_swap_chain_images, NULL);
    swap_chain_images = malloc(num_swap_chain_images * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swap_chain, &num_swap_chain_images, swap_chain_images);

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;

    // Create image views.
    create_image_views(device);

    // Create render passes.
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_creation_info = {0};
    render_pass_creation_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_creation_info.attachmentCount = 1;
    render_pass_creation_info.pAttachments = &color_attachment;
    render_pass_creation_info.subpassCount = 1;
    render_pass_creation_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &render_pass_creation_info, NULL, &render_pass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a render pass!\n");
        return -1;
    }

    // Read shaders and crate shader modules.
    size_t vert_shader_size = 0;
    char *vert_shader_code = read_shader_file("build/shaders/simple.vert.spv", &vert_shader_size);

    size_t frag_shader_size = 0;
    char *frag_shader_code = read_shader_file("build/shaders/simple.frag.spv", &frag_shader_size);

    VkShaderModule vert_shader_module = create_shader_module(device, vert_shader_code, vert_shader_size);
    VkShaderModule frag_shader_module = create_shader_module(device, frag_shader_code, frag_shader_size);

    // Create shader stages.
    VkPipelineShaderStageCreateInfo vert_shader_stage_creation_info = {0};
    vert_shader_stage_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_creation_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_creation_info.module = vert_shader_module;
    vert_shader_stage_creation_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_creation_info = {0};
    frag_shader_stage_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_creation_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_creation_info.module = frag_shader_module;
    frag_shader_stage_creation_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_creation_info, frag_shader_stage_creation_info};
    (void)shader_stages;

    // More graphics pipeline:
    VkPipelineVertexInputStateCreateInfo vertex_input_creation_info = {0};
    vertex_input_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_creation_info.vertexBindingDescriptionCount = 0;
    vertex_input_creation_info.pVertexBindingDescriptions = NULL;
    vertex_input_creation_info.vertexAttributeDescriptionCount = 0;
    vertex_input_creation_info.pVertexAttributeDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_creation_info = {0};
    input_assembly_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_creation_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_creation_info.primitiveRestartEnable = VK_FALSE; 

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)(swap_chain_extent.width);
    viewport.height = (float)(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_creation_info = {0};
    viewport_state_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_creation_info.viewportCount = 1;
    viewport_state_creation_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer_creation_info = {0};
    rasterizer_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_creation_info.depthClampEnable = VK_FALSE;
    rasterizer_creation_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_creation_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_creation_info.lineWidth = 1.0f;
    rasterizer_creation_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_creation_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_creation_info.depthBiasEnable = VK_FALSE;
    rasterizer_creation_info.depthBiasConstantFactor = 0.0f;
    rasterizer_creation_info.depthBiasClamp = 0.0f;
    rasterizer_creation_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_creation_info = {0};
    multisampling_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_creation_info.sampleShadingEnable = VK_FALSE;
    multisampling_creation_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_creation_info.minSampleShading = 1.0f;
    multisampling_creation_info.pSampleMask = NULL;
    multisampling_creation_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_creation_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // Enabling blend + resetting some values can enable alpha blending!
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_creation_info = {0};
    color_blend_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_creation_info.logicOpEnable = VK_FALSE;
    color_blend_creation_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_creation_info.attachmentCount = 1;
    color_blend_creation_info.pAttachments = &color_blend_attachment;
    color_blend_creation_info.blendConstants[0] = 0.0f;
    color_blend_creation_info.blendConstants[1] = 0.0f;
    color_blend_creation_info.blendConstants[2] = 0.0f;
    color_blend_creation_info.blendConstants[3] = 0.0f;

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    uint32_t dynamic_state_count = 2;

    VkPipelineDynamicStateCreateInfo dynamic_state_creation_info = {0};
    dynamic_state_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_creation_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_creation_info.pDynamicStates = dynamic_states;
 
    VkPipelineLayoutCreateInfo pipeline_layout_creation_info = {0};
    pipeline_layout_creation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_creation_info.setLayoutCount = 0;
    pipeline_layout_creation_info.pSetLayouts = NULL;
    pipeline_layout_creation_info.pushConstantRangeCount = 0;
    pipeline_layout_creation_info.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(device, &pipeline_layout_creation_info, NULL, &pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline!\n");
        return -1;
    }

    // Actually create the actual pipeline object!
    VkGraphicsPipelineCreateInfo pipeline_creation_info = {0};
    pipeline_creation_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_creation_info.stageCount = 2;
    pipeline_creation_info.pStages = shader_stages;
    pipeline_creation_info.pVertexInputState = &vertex_input_creation_info;
    pipeline_creation_info.pInputAssemblyState = &input_assembly_creation_info;
    pipeline_creation_info.pViewportState = &viewport_state_creation_info;
    pipeline_creation_info.pRasterizationState = &rasterizer_creation_info;
    pipeline_creation_info.pMultisampleState = &multisampling_creation_info;
    pipeline_creation_info.pDepthStencilState = NULL;
    pipeline_creation_info.pColorBlendState = &color_blend_creation_info;
    pipeline_creation_info.pDynamicState = &dynamic_state_creation_info;
    pipeline_creation_info.layout = pipeline_layout;
    pipeline_creation_info.renderPass = render_pass;
    pipeline_creation_info.subpass = 0;
    pipeline_creation_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_creation_info.basePipelineIndex = -1;

    VkPipeline graphics_pipeline = {0};
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_creation_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline!\n");
        return -1;
    }

    vkDestroyShaderModule(device, vert_shader_module, NULL);
    vkDestroyShaderModule(device, frag_shader_module, NULL);

    // The render loop:
    bool is_running = true;
    while (is_running == true) {
        SDL_Event event;

        // Poll events.
        while (SDL_PollEvent(&event) == true) {
            if (event.type == SDL_EVENT_QUIT) {
                is_running = false;
            }
        }
        //printf("Looping!\n");
    }

    // Shutdown and clean up Vulkan.
    //SDL_free(extensions); Figure out where exactly this should go?
    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    for (int i = 0; i < (int)num_swap_chain_image_views; i++) {
        vkDestroyImageView(device, swap_chain_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swap_chain, NULL);
    vkDestroyDevice(device, NULL);
    SDL_Vulkan_DestroySurface(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    // Shutdown SDL and return.
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

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

VkInstanceCreateInfo create_instance_creation_info(
    VkApplicationInfo *app_info,
    VkDebugUtilsMessengerCreateInfoEXT *debug_creation_info) {

    VkInstanceCreateInfo creation_info = {0};
    creation_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    creation_info.pApplicationInfo = app_info;

    // Get the number of extensions for the Vulkan instance.
    uint32_t count_instance_extensions = 0;
    const char * const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
    if (instance_extensions == NULL) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        // Is this something to abort program over?
    }

    // Actually set the app_info struct's extensions.
    int count_extensions = count_instance_extensions + 1;
    const char **extensions = SDL_malloc(count_extensions * sizeof(const char *));
    extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    SDL_memcpy(&extensions[1], instance_extensions, count_instance_extensions * sizeof(const char *));

    creation_info.enabledExtensionCount = count_extensions;
    creation_info.ppEnabledExtensionNames = extensions;

    // TODO: Rename num_validation layers to validation_layers_count to be more consistent?
    // If using validation layers, pNext should use the given debug creation info.
    if (enable_validation_layers == true) {
        creation_info.enabledLayerCount = num_validation_layers;
        creation_info.ppEnabledLayerNames = validation_layers;
        creation_info.pNext = debug_creation_info;
    }
    else {
        creation_info.enabledLayerCount = 0;
        creation_info.ppEnabledLayerNames = NULL;
        creation_info.pNext = NULL;
    }

    // SDL_free(extensions)?
    return creation_info;
}

bool are_validation_layers_supported(void) {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    if (layer_count == 0) {
        return true;
    }

    VkLayerProperties *available_layers = malloc(layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (int i = 0; i < num_validation_layers; i++) {
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

    return true;
}

// TODO: Actually rank devices based on characteristics and return the highest one.
// TODO: That probably wouldn't be implemented in this function. But still.
// TODO: We should prefer physical devices where the graphics family and present family are the same.
bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties device_properties = {0};
    vkGetPhysicalDeviceProperties(device, &device_properties);
    VkPhysicalDeviceFeatures device_features = {0};
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // TODO: This shouldn't actually be a literal hard requirement.
    if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        return false;
    }

    QueueFamilyIndices family_indices = find_queue_families(device, surface);
    if (family_indices.graphics_family_found == false) {
        return false;
    }

    if (check_device_extension_support(device) == false) {
        return false;
    }
    else {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device, surface);
        if ((swap_chain_support.num_formats == 0) || (swap_chain_support.num_present_modes == 0)) {
            return false;
        }
    }

    return true;
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = {0};

    // Get the number of families and each family's struct.
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    // Iterate through the array of families and set indices accordingly.
    for (int i = 0; i < (int)queue_family_count; i++) {
        VkQueueFamilyProperties curr_queue_family = queue_families[i];
        if ((curr_queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 1) {
            indices.graphics_family = i;
            indices.graphics_family_found = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support == true) {
            indices.present_family = i;
            indices.present_family_found = true;
        }
    }

    // free(queue_families)?
    return indices;
}

bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    if (extension_count == 0) {
        return false;
    }

    VkExtensionProperties *available_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);

    for (int i = 0; i < num_device_extensions; i++) {
        bool extension_found = false;
        const char *curr_extension = device_extensions[i];

        for (int j = 0; j < (int)extension_count; j++) {
            VkExtensionProperties curr_available_extension = available_extensions[j];
            if (strcmp(curr_extension, curr_available_extension.extensionName) == 0) {
                extension_found = true;
                break;
            }
        }

        if (extension_found == false) {
            return false;
        }
    }

    return true;
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &(details.capabilities));

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
    if (format_count != 0) {
        details.num_formats = format_count;
        details.formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats);
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
    if (present_mode_count != 0) {
        details.num_present_modes = present_mode_count;
        details.present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes);
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR *formats, uint32_t num_formats) {
    for (int i = 0; i < (int)num_formats; i++) {
        VkSurfaceFormatKHR curr_format = formats[i];
        if ((curr_format.format == VK_FORMAT_B8G8R8A8_SRGB) && (curr_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return curr_format;
        }
    }

    return formats[0];
}

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes, uint32_t num_present_modes) {
    for (int i = 0; i < (int)num_present_modes; i++) {
        VkPresentModeKHR curr_present_mode = present_modes[i];
        if (curr_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return curr_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

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

void create_image_views(VkDevice device) {
    num_swap_chain_image_views = num_swap_chain_images;
    swap_chain_image_views = malloc(num_swap_chain_image_views * sizeof(VkImageView));
    
    for (int i = 0; i < (int)num_swap_chain_images; i++) {
        VkImageViewCreateInfo curr_image_view_creation_info = {0};

        curr_image_view_creation_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        curr_image_view_creation_info.image = swap_chain_images[i];
        curr_image_view_creation_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        curr_image_view_creation_info.format = swap_chain_image_format;

        curr_image_view_creation_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        curr_image_view_creation_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        curr_image_view_creation_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        curr_image_view_creation_info.subresourceRange.baseMipLevel = 0;
        curr_image_view_creation_info.subresourceRange.levelCount = 1;
        curr_image_view_creation_info.subresourceRange.baseArrayLayer = 0;
        curr_image_view_creation_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &curr_image_view_creation_info, NULL, &swap_chain_image_views[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create an image view!\n");
        }
    }
}

VkShaderModule create_shader_module(VkDevice device, const char *code, size_t size) {
    VkShaderModuleCreateInfo shader_module_creation_info = {0};
    shader_module_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_creation_info.codeSize = size;
    shader_module_creation_info.pCode = (uint32_t *)code;

    VkShaderModule shader_module = {0};
    if (vkCreateShaderModule(device, &shader_module_creation_info, NULL, &shader_module) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a shader module!\n");
    }

    return shader_module;
}

