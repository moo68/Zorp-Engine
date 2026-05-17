#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>

#include "ZorpEngine/Vulkan/instance.h"
#include "ZorpEngine/Vulkan/physical_device.h"
#include "ZorpEngine/shader_utils.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>


// Global variables:
const int validation_layer_count = 1;
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

const int device_extension_count = 1;
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

VkFramebuffer *swap_chain_framebuffers = {0};
uint32_t num_framebuffers = 0;

const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif


// Function prototypes:
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR *formats, uint32_t num_formats);

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes, uint32_t num_present_modes);

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window);

void create_image_views();

VkShaderModule create_shader_module(VkDevice device, const char *code, size_t size);

void create_framebuffers(VkDevice device, VkFramebuffer *framebuffers);

void record_command_buffer(VkCommandBuffer command_buffer, VkPipeline graphics_pipeline, uint32_t image_index);


// Function definitions:
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // TODO: Properly set SDL_Log() function to different severity levels.
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
    if (SDL_ShowWindow(window) == false) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        return -1;
    }

    // Check validation layers.
    if ((enable_validation_layers == true) && (are_validation_layers_supported() == false)) {
        fprintf(stderr, "Could not properly find all Vulkan validation layers!\n");
        return -1;
    }

    // Create a Vulkan instance.
    VkInstance instance = create_instance();

    // TODO: Insert all the stuff to enumerate through extensions here.
    
    // TODO: Setup custom validation layer callback to filter specific errors.

    // Create a surface.
    VkSurfaceKHR surface = {0};
    if (SDL_Vulkan_CreateSurface(window, instance, NULL, &surface) == false) {
        SDL_Log("%s\n", SDL_GetError());
        SDL_ClearError();
        return -1;
    }

    // Choose a physical device.
    VkPhysicalDevice physical_device = pick_physical_device(instance, surface);

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
    device_creation_info.enabledExtensionCount = (uint32_t)device_extension_count;
    device_creation_info.ppEnabledExtensionNames = device_extensions;
    device_creation_info.enabledLayerCount = 0;
    device_creation_info.ppEnabledLayerNames = NULL;

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

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_creation_info = {0};
    render_pass_creation_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_creation_info.attachmentCount = 1;
    render_pass_creation_info.pAttachments = &color_attachment;
    render_pass_creation_info.subpassCount = 1;
    render_pass_creation_info.pSubpasses = &subpass;
    render_pass_creation_info.dependencyCount = 1;
    render_pass_creation_info.pDependencies = &dependency;

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
    rasterizer_creation_info.cullMode = /*VK_CULL_MODE_BACK_BIT*/VK_CULL_MODE_NONE;
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

    // Create framebuffers.
    num_framebuffers = num_swap_chain_image_views;
    swap_chain_framebuffers = malloc(num_framebuffers * sizeof(VkFramebuffer));

    for (int i = 0; i < (int)num_framebuffers; i++) {
        VkImageView attachments[] = { swap_chain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_creation_info = {0};
        framebuffer_creation_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_creation_info.renderPass = render_pass;
        framebuffer_creation_info.attachmentCount = 1;
        framebuffer_creation_info.pAttachments = attachments;
        framebuffer_creation_info.width = swap_chain_extent.width;
        framebuffer_creation_info.height = swap_chain_extent.height;
        framebuffer_creation_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_creation_info, NULL, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create a framebuffer!\n");
            return -1;
        }
    }

    // Create command pool. 
    VkCommandPool command_pool = {0};
    VkCommandPoolCreateInfo command_pool_creation_info = {0};
    command_pool_creation_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_creation_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_creation_info.queueFamilyIndex = indices.graphics_family;
    if (vkCreateCommandPool(device, &command_pool_creation_info, NULL, &command_pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        return -1;
    }

    // Create a command buffer.
    VkCommandBuffer command_buffer = {0};
    VkCommandBufferAllocateInfo buffer_alloc_info = {0};
    buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.commandPool = command_pool;
    buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_alloc_info.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device, &buffer_alloc_info, &command_buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a command buffer!\n");
        return -1;
    }
 
    // Create synchronization objects.
    VkSemaphore image_available_semaphore = {0};
    VkSemaphore render_finished_semaphore = {0};
    VkFence in_flight_fence = {0};

    VkSemaphoreCreateInfo semaphore_creation_info = {0};
    semaphore_creation_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_creation_info = {0};
    fence_creation_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_creation_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphore_creation_info, NULL, &image_available_semaphore) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create semaphore!\n");
        return -1;
    }
    if (vkCreateSemaphore(device, &semaphore_creation_info, NULL, &render_finished_semaphore) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create semaphore!\n");
        return -1;
    }
    if (vkCreateFence(device, &fence_creation_info, NULL, &in_flight_fence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fence!\n");
        return -1;
    }
    
    // The render loop:
    printf("Entering render loop!\n");
    bool is_running = true;
    SDL_Event event;
    while (is_running == true) {
        //SDL_Event event;

        // Poll events.
        while (SDL_PollEvent(&event) == true) {
            if (event.type == SDL_EVENT_QUIT) {
                is_running = false;
            }
        }

        // Draw a frame!!!
        vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &in_flight_fence);

        uint32_t image_index;
        vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

        vkResetCommandBuffer(command_buffer, 0);
        record_command_buffer(command_buffer, graphics_pipeline, image_index);

        VkSubmitInfo submit_info = {0};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore wait_semaphores[] = {image_available_semaphore};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        VkSemaphore signal_semaphores[] = {render_finished_semaphore};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
            fprintf(stderr, "Failed to submit graphics queue!\n");
            return -1;
        }

        VkPresentInfoKHR present_info = {0};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;
        VkSwapchainKHR swapchains[] = {swap_chain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = NULL;

        if (vkQueuePresentKHR(present_queue, &present_info) != VK_SUCCESS) {
            fprintf(stderr, "Failing to present!\n");
        }
    }
    vkDeviceWaitIdle(device);

    // Shutdown and clean up Vulkan.
    //SDL_free(extensions); Figure out where exactly this should go?
    vkDestroySemaphore(device, image_available_semaphore, NULL);
    vkDestroySemaphore(device, render_finished_semaphore, NULL);
    vkDestroyFence(device, in_flight_fence, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);
    for (int i = 0; i < (int)num_framebuffers; i++) {
        vkDestroyFramebuffer(device, swap_chain_framebuffers[i], NULL);
    }
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

void record_command_buffer(VkCommandBuffer command_buffer, VkPipeline graphics_pipeline, uint32_t image_index) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {0};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording command buffer!\n");
    }

    // Start render pass.
    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = swap_chain_framebuffers[image_index];
    render_pass_begin_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_begin_info.renderArea.extent = swap_chain_extent;
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)(swap_chain_extent.width);
    viewport.height = (float)(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record a command buffer!\n");
    }
}

