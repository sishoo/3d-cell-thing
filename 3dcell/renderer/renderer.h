#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_TRY(try)                                                                   \
        do                                                                            \
        {                                                                             \
                if ((try) != VK_SUCCESS)                                                \
                {                                                                     \
                        fprintf(stderr, "Heph Renderer VK_TRY(" #try "); failed.\n"); \
                        abort();                                                      \
                }                                                                     \
        } while (0);

typedef struct
{
        VkFence complete_fence;
        VkSemaphore complete_semaphore, image_acquired_semaphore;
        VkCommandBuffer command_buffer;
} frame_render_infos_t;

typedef struct
{       
        VkInstance instance;
        VkPhysicalDevice pdevice;
        VkDevice ldevice;
        uint32_t queue_family_index;
        VkQueue queue;
        VkCommandPool command_pool;

        VkBuffer cell_buffer;
        VkDescriptorSet cell_buffer_descriptor_set;

        /* Core pipelines */
        VkPipeline cell_draw_pipeline, cell_update_pipeline;
        VkPipelineLayout cell_draw_pipeline_layout,  cell_update_pipeline_layout;
        VkShaderModule vertex_shader_module, fragment_shader_module, compute_shader_module;

        /* Auxillary frame data */
        uint32_t previous_resource_index;
        frame_render_infos_t *frame_render_infos;



        int window_width, window_height;
        GLFWwindow *window;
        VkSurfaceKHR surface;
        VkSwapchainKHR swapchain;
        uint32_t nswapchain_images;
        VkImage *swapchain_images;




        /* VKB */
        vkb::Instance vkb_instance;
        vkb::PhysicalDevice vkb_pdevice;
        vkb::Device vkb_ldevice;
        vkb::Swapchain vkb_swapchain;
} heph_renderer_t;
          

void heph_renderer_init(heph_renderer_t *const r, char *const window_name, int width, int height);
void heph_renderer_render_frame(heph_renderer_t *const r);
void heph_renderer_destroy(heph_renderer_t *const r);

void heph_renderer_init_instance(heph_renderer_t *const r);
void heph_renderer_init_surface(heph_renderer_t *const r);
void heph_renderer_init_pdevice(heph_renderer_t *const r);
void heph_renderer_init_ldevice(heph_renderer_t *const r); 
void heph_renderer_init_queue(heph_renderer_t *const r);
void heph_renderer_init_swapchain(heph_renderer_t *const r);
void heph_renderer_aquire_swapchain_images(heph_renderer_t *const r);
void heph_renderer_init_command_pools(heph_renderer_t *const r);
void heph_renderer_allocate_main_command_buffer(heph_renderer_t *const r);
void heph_renderer_init_sync_structures(heph_renderer_t *const r);
void heph_renderer_init_frame_render_infos(heph_renderer_t *const r);

void heph_renderer_resize(heph_renderer_t *const r, uint32_t width, uint32_t height);

void heph_renderer_rebuild_swapchain(heph_renderer_t *const r, int width, int height);


