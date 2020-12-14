#include "test_builtin_visuals.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/interact.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklMouse* mouse = (VklMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    vkl_mouse_event(mouse, canvas, ev);
}

static void _common_data(VklVisual* visual)
{
    VklCanvas* canvas = visual->canvas;
    VklContext* ctx = canvas->gpu->context;

    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(visual, VKL_PROP_PROJ, 0, 1, id);

    vkl_visual_data_texture(visual, VKL_PROP_COLOR_TEXTURE, 0, 1, 1, 1, NULL);
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(visual, viewport, (VklDataCoords){0}, NULL);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, visual);
}



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_visuals_scatter(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual_builtin(canvas, VKL_VISUAL_SCATTER, 0);

    // GPU sources.
    const uint32_t N = 10000;

    // Vertex data.
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    // Params.
    float param = 5.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    _common_data(&visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}