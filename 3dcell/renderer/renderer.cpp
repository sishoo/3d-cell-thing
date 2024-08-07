#include "renderer.h"

void cell_renderer_init_instance(cell_renderer_t *const r)
{
        vkb::InstanceBuilder builder;
        auto res = builder
                .set_app_name("cell thing")
                .set_engine_name("bro")
                .require_api_version(1, 3, 0)
                .use_default_debug_messenger()
              //.set_debug_callback(debug_callback)
                .request_validation_layers()
                .build();
        HEPH_NASSERT(res.vk_result(), VK_SUCCESS);
        r->vkb_instance = res.value();
        r->instance = r->vkb_instance.instance;
}

void cell_renderer_init_window(cell_renderer_t *const r, char *const name)
{
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        r->window = glfwCreateWindow(r->window_width, r->window_height, name, NULL, NULL);

        if (!r->window) abort();
}

void cell_renderer_init_surface(cell_renderer_t *const r)
{
        VK_TRY(glfwCreateWindowSurface(r->instance, r->window, NULL, &r->surface));
}

void cell_renderer_init_pdevice(cell_renderer_t *const r)
{
        #warning DONT DELETE THIS UNTIL YOU FIX THE DAMN PICK FIRST DEVICE UNCONDITIONALLY!!!
        vkb::PhysicalDeviceSelector selector(r->vkb_instance);
        auto res = selector
                       .set_surface(r->surface)
                       .select_first_device_unconditionally()
                       .select();
        VK_TRY(res.vk_result());
        r->vkb_pdevice = res.value();
        r->pdevice = r->vkb_pdevice.physical_device;
}

void cell_renderer_find_memory_type_indices(cell_renderer_t *const r)
{
        VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
        vkGetPhysicalDeviceMemoryProperties(r->pdevice, &physical_device_memory_properties);

        /* TODO
                once the engine buffers memory types are different from eachother you must fix this
        */
#warning once the engine buffers memory types are different from eachother you must fix this
#if cell_renderer_GEOMETRY_BUFFER_MEMORY_TYPE_BITFLAGS == cell_renderer_REQUIRED_OBJECT_BUFFER_MEMORY_TYPE_BITFLAGS == cell_renderer_REQUIRED_DRAW_BUFFER_MEMORY_TYPE_BITFLAGS
        for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
        {
                if ((cell_renderer_REQUIRED_GEOMETRY_BUFFER_BITFLAGS & physical_device_memory_properties.memoryTypes[i]) == cell_renderer_REQUIRED_GEOMETRY_BUFFER_BITFLAGS)
                {
                        r->geometry_buffer_memory_type_index = i;
                        r->object_buffer_memory_type_index = i;
                        r->draw_buffer_memory_type_index = i;

                        return;
                }
        }
#endif

        /* TODO do some fallback like required a staging buffer */
        HEPH_ABORT("Unable to find suitable memory types for engine buffers.");
}

void cell_renderer_init_ldevice(cell_renderer_t *const r)
{
        VkPhysicalDeviceSynchronization2Features sync_features = {};
        sync_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        sync_features.synchronization2 = VK_TRUE;

        vkb::DeviceBuilder builder{r->vkb_pdevice};
        auto res = builder
                       .add_pNext(&sync_features)
                       .build();
        VK_TRY(res.vk_result());
        r->vkb_ldevice = res.value();
        r->ldevice = r->vkb_ldevice.device;
}

void cell_renderer_init_queues(cell_renderer_t *const r)
{
        /* First pass to find a queue with everything */
        #warning subject to change
        uint32_t required = cell_renderer_REQUIRED_MAIN_QUEUE_FAMILY_BITFLAGS, i = 0, nqueues = 0;
        for (VkQueueFamilyProperties props : r->vkb_pdevice.get_queue_families())
        {
                VkQueueFlags flags = props.queueFlags;
                if ((flags & required) == required)
                {
                        goto found_all;
                }
                i++;
                nqueues++;
        }

        found_all:
        r->nqueues = 1;
        vkGetDeviceQueue(r->ldevice, i, 0, &r->queue);

#ifdef DONT_USE_THIS
        while (required)
        {
                
        }
        for (VkQueueFamilyProperties props : r->vkb_pdevice.get_queue_families())
        {
                
        }





        /* Second pass to find multiple queues that meet requirements */
        i = 0;
        while (required)
        {
                uint32_t flag = flags & required;
                switch (flag)
                {
                        case 0:
                        {
                                HEPH_ABORT("Unable to find suitable queues.");
                        }
                        case VK_QUEUE_GRAPHICS_BIT:    
                        {
                                nqueues++;
                                heph_gpu_queue_t graphics_queue = {     
                                        .queue_family_index = i
                                };
                                vkGetDeviceQueue(r->ldevice, i, 0, &graphics_queue.handle);
                                break;
                        } 
                        case VK_QUEUE_COMPUTE_BIT: 
                        {
                                nqueues++;
                                heph_gpu_queue_t compute_queue = {
                                        .queue_family_index = i      
                                };
                                vkGetDeviceQueue(r->ldevice, i, 0, &compute_queue.handle);
                                break;
                        }
                        case VK_QUEUE_TRANSFER_BIT:
                        {
                                nqueues++;
                                heph_gpu_queue_t transfer_queue = {
                                        .queue_family_index = i
                                };
                                vkGetDeviceQueue(r->ldevice, i, 0, &transfer_queue.handle);
                                break;
                        }
                }
                required &= !flags;
                i++;
        }

        found_all:


#endif

}

void cell_renderer_init_swapchain(cell_renderer_t *const r)
{
        vkb::SwapchainBuilder builder{r->vkb_ldevice};
        auto res = builder
                       .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                       .build();
        VK_TRY(res.vk_result());
        r->vkb_swapchain = res.value();
        r->swapchain = r->vkb_swapchain.swapchain;
}

void cell_renderer_aquire_swapchain_images(cell_renderer_t *const r)
{
        VK_TRY(vkGetSwapchainImagesKHR(r->ldevice, r->swapchain, &r->nswapchain_images, NULL));

        r->swapchain_images = (VkImage *)calloc(sizeof(VkImage), r->nswapchain_images);

        VK_TRY(vkGetSwapchainImagesKHR(r->ldevice, r->swapchain, &r->nswapchain_images, r->swapchain_images));
}

void cell_renderer_init_command_pool(cell_renderer_t *const r)
{
        /* Main command pool */
        VkCommandPoolCreateInfo main_command_pool_create_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = r->queue_family_index,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
        };

        VK_TRY(vkCreateCommandPool(r->ldevice, &main_command_pool_create_info, NULL, &r->main_command_pool));

        /* Recording thread command pool */
        VkCommandPoolCreateInfo command_buffer_recording_command_pool = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = r->queue_family_index
        };

        VkCommandPool recording_thread_command_pool = VK_NULL_HANDLE;
        VK_TRY(vkCreateCommandPool(r->ldevice, &command_buffer_recording_command_pool, NULL, &r->command_buffer_recording_command_pool));
}

void cell_renderer_init_cell_image(cell_renderer_t *const r)
{
        VkExtent3D cell_image_extent = {
                .width = CELL_CONTAINER_LENGTH,
                .height = CELL_CONTAINER_LENGTH,
                .depth = CELL_CONTAINER_LENGTH
        };

        VkImageCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_3D,
                .format = VK_FORMAT_R8_UINT,
                .extent = cell_image_extent,
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &r->queue_family_index,
                .initialLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        VK_TRY(vkCreateImage(r->ldevice, &create_info, NULL, &r->cell_image));

        VkMemoryAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = CELL_CONTAINER_LENGTH * CELL_CONTAINER_LENGTH * CELL_CONTAINER_LENGTH / 8,
                .memoryTypeIndex = slkfjslfjlsfjlosf
        };

        VK_TRY(vkAllocateMemory(r->ldevice, &allocate_info, NULL, &r->cell_image_memory));

        VK_TRY(vkBindImageMemory(r->ldevice, r->cell_image, r->cell_image_memory, 0));
}

void cell_renderer_init_draw_buffer(cell_renderer_t *const r)
{


        VkBufferCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = CELL_CONTAINER_LENGTH * CELL_CONTAINER_LENGTH * CELL_CONTAINER_LENGTH + sizeof(uint32_t),

        };      
}

void cell_renderer_init_cell_image_sampler(cell_renderer_t *const r)
{
        VkSamplerCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0,
                .anisotropyEnable = VK_FALSE,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 1.0,
                .maxLod = 1.0,
                .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                .unormalizedCoordinates = VK_FALSE
        };

        VK_TRY(vkCreateSampler(r->ldevice, &create_info, NULL, &r->cell_image_sampler));
}

void cell_renderer_init_sync_structures(cell_renderer_t *const r)
{
        /* Image acquired */
        VkSemaphoreCreateInfo image_acquired_semaphore_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VK_TRY(vkCreateSemaphore(r->ldevice, &image_acquired_semaphore_info, NULL, &r->image_acquired_semaphore));
}

void cell_renderer_init_frame_render_infos(cell_renderer_t *const r)
{
        r->previous_resource_index = UINT32_MAX;
        r->frame_render_infos = (heph_frame_render_infos_t *)HCALLOC(r->nswapchain_images, sizeof(heph_frame_render_infos_t));

        VkFenceCreateInfo finished_fence_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        VkSemaphoreCreateInfo finished_semaphore_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VkCommandBufferAllocateInfo command_buffer_allocate_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = r->command_buffer_recording_command_pool,
                .commandBufferCount = 1,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        };

        for (uint32_t i = 0; i < r->nswapchain_images; i++)
        {
                VK_TRY(vkCreateFence(r->ldevice, &finished_fence_info, NULL, &r->frame_render_infos[i].finished_fence));
                VK_TRY(vkCreateSemaphore(r->ldevice, &finished_semaphore_info, NULL, &r->frame_render_infos[i].finished_semaphore));
                VK_TRY(vkAllocateCommandBuffers(r->ldevice, &command_buffer_allocate_info, &r->frame_render_infos[i].command_buffer));
        }
}

void cell_renderer_init_shader_modules(cell_renderer_t *const r)
{
        static uint32_t compute_shader[] = {
                #include "" 
        };

        VkShaderModuleCreateInfo fragment_shader_create_info = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = fragment_shader_src.size_bytes,
                .pCode = (uint32_t *)fragment_shader_src.ptr
        };
}

void cell_renderer_init_graphics_pipeline(cell_renderer_t *const r)
{
        /* Pipeline rendering create info */
        VkFormat color_attachment_format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .depthAttachmentFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &color_attachment_format,
                .stencilAttachmentFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .viewMask = 0
        };

        /* Grahpics pipeline create info */
        VkPipelineShaderStageCreateInfo *shader_stage_create_infos = (VkPipelineShaderStageCreateInfo *)calloc(2 * sizeof(VkPipelineShaderStageCreateInfo));
        
        /* Vertex shader*/
        shader_stage_create_infos[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .flags = VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = r->vertex_shader_module,
                .pName = "main"
        };
        /* Fragment shader */
        shader_stage_create_infos[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .flags = VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = r->fragment_shader_module,
                .pName = "main"
        };

        /* Vertex input state infos */
        VkVertexInputAttributeDescription vertex_input_attribute_descriptions[2];
        VkVertexInputBindingDescription vertex_input_binding_descriptions[2];

        /* Vertex input ATTRIBUTE descriptions */
        /* Position */
        vertex_input_attribute_descriptions[0] = {
                .binding = cell_renderer_VERTEX_INPUT_ATTR_DESC_BINDING_POSITION,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .location = 0,
                .offset = offsetof(heph_vertex_t, position)
        };
        /* U/V */
        vertex_input_attribute_descriptions[1] = {
                .binding = ,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = ,
                .offset = offsetof(heph_vertex_t, uv)
        };

        /* Vertex input BINDING descriptions */
        /* Position */
        vertex_input_binding_descriptions[0] = {
                .binding = cell_renderer_VERTEX_INPUT_ATTR_DESC_BINDING_POSITION,
                .stride = sizeof(heph_vector3_t),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        /* U/V */
        vertex_input_binding_descriptions[1] = {
                .binding = ,
                .stride = sizeof(heph_vector2_t),
                .inputeRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 2,
                .pVertexBindingDescriptions = vertex_input_binding_descriptions,
                .vertexAttributeDescriptionCount = 2,
                .pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
        };

        /* Input assembly state info */
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
        };

        /* Tessellation */
        VkPipelineTessellationStateCreateInfo tessellation_state_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
                .patchControlPoints = 0
        };

        /* Viewport state */
        VkViewport viewport = {
                .x = 0.0,
                .y = 0.0,
                .width = 1920.0,
                .height = 1080.0,
                .minDepth = 0.0,
                .maxDepth = 1.0
        };

        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {1920, 1080}
        };

        VkPipelineViewportStateCreateInfo viewport_state_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = cell_renderer_PIPELINE_VIEWPORT_COUNT,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor
        };
       

        /* Rasterization State */
        #warning this is janky come backe and properly do this (EX: cull mode)
        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_FRONT_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .lineWidth = 1.0
        };

        /* Multisample state */
        // VkPipelineMultisampleStateCreateInfo multisample_state = {
        //         .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        //         .flags = 
        // };

        /* Final grahpics pipeline */
        VkGraphicsPipelineCreateInfo pipeline_create_info = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &pipeline_rendering_create_info,
                .stageCount = 2,
                .pStages = shader_stage_create_infos,
                .pVertexInputState = &vertex_input_state_create_info,
                .pInputAssemblyState = &input_assembly_state_create_info,

        };

        VkPipeline pipeline;
        HEPH_NASSERT(vkCreateGraphicsPipelines(r->ldevice, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline), VK_SUCCESS);
}

void cell_renderer_init_compute_pipeline(cell_renderer_t *const r)
{
        heph_string_t compute_shader_src = {};
        HEPH_NASSERT(heph_file_read_to_string(&compute_shader_src, "shader/compute.comp"), true);
        cell_renderer_preprocess_compute_shader(r, &compute_shader_src);
        cell_renderer_compile_compute_shader(r, &compute_shader_src);

        /* Shader Module */
        VkShaderModuleCreateInfo shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = compute_shader_src.size_bytes,
            .pCode = (uint32_t *)compute_shader_src.ptr
        };

        VkShaderModule shader_module = {};
        VK_TRY(vkCreateShaderModule(r->ldevice, &shader_module_create_info, NULL, &shader_module));

        /* Shader stage */
        VkPipelineShaderStageCreateInfo shader_stage_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = "main"
        };

        /* Push constant range */
        VkPushConstantRange push_constant_range = {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = sizeof(uint32_t) + sizeof(float[6]) + sizeof(float[16])
        };

        /* Descriptor sets */
        VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[];

        /* Object buffer descriptor set */
        descriptor_set_layout_bindings[0] = {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        };

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT,
                .bindingCount = 1,
                .pBinding = &descriptor_set_layout_bindings[0]
        };

        VkDescriptorSetLayout descriptor_set_layout = {};
        VK_TRY(vkCreateDescriptorSetLayout(r->ldevice, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout));

        /* Pipeline layout */
        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 3,
                .pSetLayouts = descriptor_set_layout,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &push_constant_range
        };

        VK_TRY(vkCreatePipelineLayout(r->ldevice, &pipeline_layout_create_info, NULL, &r->compute_pipeline_layout));

        /* Create compute pipeline */
        VkComputePipelineCreateInfo compute_pipeline_create_info = {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = shader_stage_create_info,
                .layout =,
                .basePipelineHandle =,
                .basePipelineIndex = 
        };

        VK_TRY(vkCreateComputePipelines(r->ldevice, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &r->compute_pipeline));
}

void cell_renderer_rebuild_swapchain(cell_renderer_t *const r, int width, int height)
{
        /*
                TODO
                using vkb for the swapchain recreation might not be good i dont know
        */
        vkDeviceWaitIdle(r->ldevice);

        vkb::destroy_swapchain(r->vkb_swapchain);

        vkb::SwapchainBuilder builder{r->vkb_ldevice};
        auto res = builder.build();

        VK_TRY(res.vk_result());

        r->vkb_swapchain = res.value();
        r->swapchain = r->vkb_swapchain.swapchain;

        cell_renderer_aquire_swapchain_images(r);
}

void cell_renderer_handle_window_resize(cell_renderer_t *const r, int width, int height)
{
        r->window_width = width;
        r->window_height = height;
        cell_renderer_rebuild_swapchain(r, width, height);
        cell_renderer_recalculate_projection_matrix(r);
}

void cell_renderer_render(cell_renderer_t *const r)
{
        #warning gotta do something with nviewports here
        for (uint32_t c = 0; c < r->context.scene->ncameras; c++)
        {
                r->context.camera = &r->context.scene->cameras[c];

                cell_renderer_render_frame(r);
        }
}

void cell_renderer_bind_persistant_descriptor_sets(cell_renderer_t *const r)
{
        /* Pepare the command buffer for recording */
        VkCommandBufferBeginInfo command_buffer_begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        frame_infos_t *frame_infos = r->frame_render_infos;

        VK_TRY(vkResetCommandBuffer(frame_infos->command_buffer, 0));
        VK_TRY(vkBeginCommandBuffer(frame_infos->command_buffer, &command_buffer_begin_info));
        
        /* Cell updates */
        vkCmdBindDescriptorSets(frame_infos->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, r->cell_update_pipeline_layout, 0, 3, r->cell_buffer_descriptor_set, 0, NULL);
  
        VK_TRY(vkEndCommandBuffer(frame_infos->command_buffer));

        /* Submit the main command buffer */
        VkCommandBufferSubmitInfoKHR command_buffer_submit_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = frame_infos->command_buffer
        };

        VkSubmitInfo2 submit_info = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &command_buffer_submit_info,
        };

        VK_TRY(vkQueueSubmit2(r->queue, 1, &submit_info, frame_infos->finished_fence));
}

void cell_renderer_render_frame(cell_renderer_t *const r)
{
        uint32_t resource_index = r->previous_resource_index;
        if (++resource_index == r->nswapchain_images)
        {
                resource_index = 0;
        }

        frame_render_infos_t *frame_infos = &r->frame_render_infos[resource_index];

        /* Make sure the fence we are going to use is not in use */
        VK_TRY(vkWaitForFences(r->ldevice, 1, &frame_infos->finished_fence, VK_TRUE, 1000));
        VK_TRY(vkResetFences(r->ldevice, 1, &frame_infos->finished_fence));

        /* Get target image */
        uint32_t image_index;
        VK_TRY(vkAcquireNextImageKHR(r->ldevice, r->swapchain, cell_renderer_MAX_TIMEOUT_NS, r->image_acquired_semaphore, NULL, &image_index));
        VkImage target_image = r->swapchain_images[image_index];

        /* Pepare the frame command buffer for recording */
        VkCommandBufferBeginInfo command_buffer_begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        VK_TRY(vkResetCommandBuffer(frame_infos->command_buffer, 0));
        VK_TRY(vkBeginCommandBuffer(frame_infos->command_buffer, &command_buffer_begin_info));

        /* Cell updates */
        vkCmdBindPipeline(frame_infos->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, r->cell_update_pipeline);
        vkCmdDispatch(frame_infos->command_buffer, ceil( / 16), 1, 1);

        VkBufferMemoryBarrier2 draw_buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                .srcQueueFamilyIndex = r->queue_family_index,
                .dstQueueFamilyIndex = r->queue_family_index,
                .buffer = scene->draw_buffer.handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
        };

        /* Translate target image: UNDEFINED -> COLOR_ATTACHMENT */
        VkImageSubresourceRange target_image_subresource_range = {
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        };

        VkImageMemoryBarrier2 target_image_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE, // None is sufficient, aquiring the image is synced
                .dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = target_image,
                .subresourceRange = target_image_subresource_range
        };

        /* Barrier for image transition UNDEFINED -> COLOR_ATTACHMENT and cell_update */
        VkDependencyInfo barriers_dependency_info = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers = &draw_buffer_memory_barrier,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &target_image_memory_barrier
        };

        vkCmdPipelineBarrier2(frame_infos->command_buffer, &barriers_dependency_info);

        /* Bind required 'graphics' resources */
        vkCmdBindVertexBuffers(frame_infos->command_buffer, 0, 1, &scene->geometry_buffer.handle, (VkDeviceSize[]){0});
        vkCmdBindIndexBuffer(frame_infos->command_buffer, scene->geometry_buffer.handle, scene->meshes_size_bytes, VK_INDEX_TYPE_UINT32);
        vkCmdBindPipeline(frame_infos->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, r->graphics_pipeline);

        /* PushConstant the view / projection matrices */
        vkCmdPushConstants(
                frame_infos->command_buffer,
                r->graphics_pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                sizeof(camera->view_matrix) + sizeof(uint32_t),
                sizeof(scene->projection_matrix),
                scene->projection_matrix
        );

        vkCmdBindDescriptorSets(
                frame_infos->command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                r->graphics_pipeline_layout,
                0,
                1,
                &scene->texture_buffer.descriptor_set,
                0,
                NULL
        );

        #warning this will do for now because we arnt batching draws but change later
        uint32_t ndraws = 0;
        for (uint32_t i = 0; i < scene->nobjects; i++)
        {
                heph_object_t *object = &((heph_object_t *)scene->object_buffer.mapped_ptr)[i];
                if (!object->is_visible)
                {
                        continue;
                }
                ++ndraws;
                vkCmdPushConstants(
                    frame_infos->command_buffer,
                    r->graphics_pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    sizeof(camera->view_matrix) + sizeof(scene->projection_matrix) + sizeof(uint32_t),
                    sizeof(uint32_t),
                    &i
                );
        }
        vkCmdDrawIndexedIndirect(frame_infos->command_buffer, scene->draw_buffer.handle, 0, ndraws, sizeof(VkDrawIndexedIndirectCommand));


        /* Translate target image: COLOR_ATTACHMENT -> PRESENTABLE */
        VkImageMemoryBarrier2 target_image_memory_barrier2 = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE, // None is sufficient, acquiring the image is synced
                .dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = target_image,
                .subresourceRange = target_image_subresource_range
        };

        /* Barrier for image transition COLOR_ATTACHMENT -> PRESENTABLE */
        // TODO bad name
        VkDependencyInfo presentation_dependency_info = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &target_image_memory_barrier2
        };

        vkCmdPipelineBarrier2(frame_infos->command_buffer, &presentation_dependency_info);

        VK_TRY(vkEndCommandBuffer(frame_infos->command_buffer));

        /* Submit the main command buffer */
        VkCommandBufferSubmitInfoKHR command_buffer_submit_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = frame_infos->command_buffer
        };

        VkSemaphoreSubmitInfo submit_info_wait_semaphores[2] = {};
        /* Wait on image acquired semaphore */
        submit_info_wait_semaphores[0] = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = r->image_acquired_semaphore
        };
        /* Wait on previous frame done semaphore */
        submit_info_wait_semaphores[1] = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = frame_infos->finished_semaphore
        };

        VkSubmitInfo2 submit_info = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &command_buffer_submit_info,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &frame_infos->finished_semaphore_submit_info,
                .waitSemaphoreInfoCount = (uint32_t)2 - (r->previous_resource_index == UINT32_MAX),
                .pWaitSemaphoreInfos = submit_info_wait_semaphores
        };

        VK_TRY(vkQueueSubmit2(r->queue, 1, &submit_info, frame_infos->finished_fence));

        /* Present the frame to the screen */
        VkPresentInfoKHR present_info = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .swapchainCount = 1,
                .pSwapchains = &r->swapchain,
                .waitSemaphoreCount = 1,
                .pImageIndices = &image_index,
                .pWaitSemaphores = &frame_infos->finished_semaphore,
        };

        VK_TRY(vkQueuePresentKHR(r->queue, &present_info));

        r->previous_resource_index = resource_index;
}

void cell_renderer_init(cell_renderer_t *const r, char *const window_name, int width, int height)
{
        cell_renderer_init_instance(r);
        r->window_width = width;
        r->window_height = height;

        /* Vulkan Core Constructs */
        cell_renderer_init_window(r, window_name);
        cell_renderer_init_surface(r);
        cell_renderer_init_pdevice(r);
        cell_renderer_find_memory_type_indices(r);
        cell_renderer_init_ldevice(r);
        cell_renderer_init_queue(r);
        cell_renderer_init_swapchain(r);
        cell_renderer_aquire_swapchain_images(r);
        cell_renderer_init_command_pool(r);

        /* Engine Specifics */
        cell_renderer_init_cell_image(r);
        cell_renderer_init_draw_buffer(r);
        cell_renderer_populate_cell_image(r);
        cell_renderer_init_frame_render_infos(r);
        cell_renderer_init_sync_structures(r);
        cell_renderer_init_cell_draw_pipeline(r);
        cell_renderer_init_cell_update_pipeline(r);

}

/* Use only for debug mode. */
void cell_renderer_destroy(cell_renderer_t *const r)
{
#if HEPH_DEBUG
        vkDeviceWaitIdle(r->ldevice);

        glfwDestroyWindow(r->window);
        glfwTerminate();

        /* Destroy shader modules and free src arrays*/
        #warning use for loop proabably
        vkDestroyShaderModule(r->ldevice, r->vertex_shader_module, NULL);
        vkDestroyShaderModule(r->ldevice, r->fragment_shader_module, NULL);
        vkDestroyShaderModule(r->ldevice, r->compute_shader_module, NULL);
        HFREE(r->vertex_shader_src);
        HFREE(r->fragment_shader_src);
        HFREE(r->compute_shader_src);

        for (uint32_t i = 0; i < r->nswapchain_images; i++)
        {
                vkDestroyFence(r->ldevice, r->frame_render_infos[i].finished_fence, NULL);
                vkDestroySemaphore(r->ldevice, r->frame_render_infos[i].finished_semaphore, NULL);
        }
        HFREE(r->frame_render_infos);

        /* Do not change ordering */
        for (uint32_t i = 0; i < r->nswapchain_images; i++)
        {
                vkDestroyImage(r->ldevice, r->swapchain_images[i], NULL);
        }
        HFREE(r->swapchain_images);


        #warning is there even supposed to be a main command pool????
        vkDestroyCommandPool(r->ldevice, r->main_command_pool, NULL);



        /* Destroy main buffers */
        uint32_t nmain_buffers = sizeof(r->context.scene->main_buffers) / sizeof(r->context.scene->main_buffers[0]);
        for (uint32_t i = 0; i < nmain_buffers; i++)
        {
                vkFreeMemory(r->ldevice, r->context.scene->main_buffers[i].device_memory, NULL);
                vkDestroyBuffer(r->ldevice, r->context.scene->main_buffers[i].handle, NULL);
        }


        vkb::destroy_swapchain(r->vkb_swapchain);
        vkb::destroy_surface(r->vkb_instance, r->surface);
        vkb::destroy_device(r->vkb_ldevice);
        vkb::destroy_instance(r->vkb_instance);
#endif
}