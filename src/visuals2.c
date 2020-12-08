#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklBindings* _get_bindings(VklVisual* visual, VklSource* source)
{
    ASSERT(source != NULL);
    if (source->pipeline == VKL_PIPELINE_GRAPHICS)
        return &visual->bindings[source->pipeline_idx];
    else if (source->pipeline == VKL_PIPELINE_COMPUTE)
        return &visual->bindings_comp[source->pipeline_idx];
    return NULL;
}



/*************************************************************************************************/
/*  Default callbacks                                                                            */
/*************************************************************************************************/

static void _default_visual_fill(VklVisual* visual, VklVisualFillEvent ev)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    VklCommands* cmds = ev.cmds;
    uint32_t idx = ev.cmd_idx;
    VkViewport viewport = ev.viewport.viewport;

    ASSERT(viewport.width > 0);
    ASSERT(viewport.height > 0);
    ASSERT(is_obj_created(&visual->graphics[0]->obj));
    ASSERT(is_obj_created(&visual->bindings[0].obj));

    VklSource* vertex_source = vkl_bake_source(visual, VKL_SOURCE_VERTEX, 0);
    VklBufferRegions* vertex_buf = &vertex_source->u.b.br;
    ASSERT(vertex_buf != NULL);
    ASSERT(vertex_buf->count > 0);

    uint32_t vertex_count = visual->vertex_count;
    ASSERT(vertex_count > 0);

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(cmds, idx, viewport);
    vkl_cmd_bind_vertex_buffer(cmds, idx, vertex_buf, 0);
    // TODO: index buffer
    vkl_cmd_bind_graphics(cmds, idx, visual->graphics[0], &visual->bindings[0], 0);
    vkl_cmd_draw(cmds, idx, 0, vertex_count);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



static void _default_visual_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    // The default baking function assumes all props have the same number of items, which
    // also corresponds to the number of vertices.

    ASSERT(visual != NULL);

    // VERTEX source.
    VklSource* source = vkl_bake_source(visual, VKL_SOURCE_VERTEX, 0);

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (source->origin != VKL_SOURCE_ORIGIN_LIB)
        return;

    // Check that all props for VERTEX buffer source have the same number of items.
    // TODO: or take the MAX?
    VklProp* prop = NULL;
    VklArray* arr = NULL;
    uint32_t item_count = 0;
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        prop = &visual->props[i];
        if (prop->source_type == VKL_SOURCE_VERTEX)
        {
            arr = &prop->arr_orig;
            ASSERT(arr != NULL);
            if (item_count == 0)
                item_count = arr->item_count;
            ASSERT(arr->item_count == item_count);
        }
    }
    visual->vertex_count = item_count;

    // TODO: INDEX source
    visual->index_count = 0;

    // Resize the vertex source.
    arr = &source->arr;
    ASSERT(is_obj_created(&arr->obj));
    vkl_array_resize(arr, item_count);

    // Copy all associated props to the VERTEX source array.
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        prop = &visual->props[i];
        // TODO: multiple VERTEX sources
        if (prop->source_type == VKL_SOURCE_VERTEX && prop->source_idx == 0)
        {
            vkl_bake_prop_copy(visual, prop, 1);
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};
    visual.canvas = canvas;

    // Default callbacks.
    visual.callback_fill = _default_visual_fill;
    visual.callback_bake = _default_visual_bake;

    obj_created(&visual.obj);
    return visual;
}



void vkl_visual_destroy(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Free the props.
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        vkl_array_destroy(&visual->props[i].arr_orig);
        vkl_array_destroy(&visual->props[i].arr_trans);
        // vkl_array_destroy(&visual->props[i].arr_triang);
    }

    // Free the data sources.
    for (uint32_t i = 0; i < visual->source_count; i++)
        vkl_array_destroy(&visual->sources[i].arr);

    obj_destroyed(&visual->obj);
}



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

void vkl_visual_source(
    VklVisual* visual, VklSourceType source, uint32_t source_idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx, uint32_t slot_idx, VkDeviceSize item_size)
{
    ASSERT(visual != NULL);
    ASSERT(visual->source_count < VKL_MAX_VISUAL_SOURCES);
    ASSERT(vkl_bake_source(visual, source, source_idx) == NULL);

    VklSource src = {0};
    src.source_type = source;
    src.source_idx = source_idx;
    src.pipeline = pipeline;
    src.pipeline_idx = pipeline_idx;
    src.slot_idx = slot_idx;
    src.arr = vkl_array_struct(0, item_size);
    src.origin = VKL_SOURCE_ORIGIN_NONE; // source origin (GPU object) not set yet
    visual->sources[visual->source_count++] = src;

    // source.
    // source.prop = VKL_PROP_VERTEX;
    // source.prop_idx = 0;
    // source.dtype = VKL_DTYPE_CUSTOM;
    // source.dtype_size = vertex_size;
    // source.loc = VKL_PROP_LOC_VERTEX_BUFFER;
    // source.binding = VKL_PROP_BINDING_BUFFER;
    // source.is_set = true;
    // visual->vertex_buf = &visual->sources[visual->source_count].u.b.br;
    // visual->source_count++;
    // vkl_visual_source(visual, VKL_SOURCE_VERTEX, 0)
}



void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t prop_idx, //
    VklSourceType source, uint32_t source_idx,              //
    uint32_t field_idx, VklDataType dtype, VkDeviceSize offset)
{
    ASSERT(visual != NULL);
    ASSERT(visual->prop_count < VKL_MAX_VISUAL_PROPS);

    VklProp pr = {0};

    pr.prop_type = prop;
    pr.prop_idx = prop_idx;
    pr.source_type = source;
    pr.source_idx = source_idx;

    pr.field_idx = field_idx;
    pr.dtype = dtype;
    pr.offset = offset;

    visual->props[visual->prop_count++] = pr;
}



void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(is_obj_created(&graphics->obj));
    if (visual->graphics_count >= VKL_MAX_GRAPHICS_PER_VISUAL)
    {
        log_error("maximum number of graphics per visual reached");
        return;
    }
    visual->graphics[visual->graphics_count] = graphics;
    visual->bindings[visual->graphics_count] = vkl_bindings(&graphics->slots, 1);
    visual->graphics_count++;
}



void vkl_visual_compute(VklVisual* visual, VklCompute* compute)
{
    ASSERT(visual != NULL);
    ASSERT(compute != NULL);
    ASSERT(is_obj_created(&compute->obj));
    if (visual->compute_count >= VKL_MAX_COMPUTES_PER_VISUAL)
    {
        log_error("maximum number of computes per visual reached");
        return;
    }
    visual->computes[visual->compute_count++] = compute;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
    if (group_idx >= VKL_MAX_VISUAL_GROUPS)
    {
        log_error("maximum number of groups reached");
        return;
    }
    visual->group_count = MAX(visual->group_count, group_idx + 1);
    visual->group_sizes[group_idx] = size;
}



void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t count, const void* data)
{
    ASSERT(visual != NULL);
    vkl_visual_data_partial(visual, type, idx, 0, count, count, data);
}



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
{
    ASSERT(visual != NULL);
    uint32_t count = first_item + item_count;

    // Get the associated prop.
    VklProp* prop = vkl_bake_prop(visual, type, idx);
    ASSERT(prop != NULL);

    // Get the associated source.
    VklSource* source = vkl_bake_source(visual, prop->source_type, prop->source_idx);
    ASSERT(source != NULL);

    // Make sure the array has the right size.
    vkl_array_resize(&prop->arr_orig, count);

    // Copy the specified array to the prop array.
    vkl_array_data(&prop->arr_orig, first_item, item_count, data_item_count, data);

    source->origin = VKL_SOURCE_ORIGIN_LIB;
    // prop->is_set = true;
}



// Means that no data updates will be done by visky, it is up to the user to update the bound
// buffer
void vkl_visual_buffer(VklVisual* visual, VklSourceType source, uint32_t idx, VklBufferRegions br)
{
    vkl_visual_buffer_partial(visual, source, idx, br, 0, br.size);
}



void vkl_visual_buffer_partial(
    VklVisual* visual, VklSourceType source, uint32_t idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* src = vkl_bake_source(visual, source, idx);
    if (src == NULL)
    {
        log_error("Data source for source %d #%d could not be found", source, idx);
        return;
    }
    ASSERT(src != NULL);
    if (size == 0)
        size = br.size;
    ASSERT(size > 0);
    ASSERT(br.buffer != VK_NULL_HANDLE);

    src->u.b.br = br;
    src->u.b.offset = offset;
    src->u.b.size = size;
    src->origin = VKL_SOURCE_ORIGIN_USER;

    VklBindings* bindings = _get_bindings(visual, src);
    ASSERT(br.buffer != VK_NULL_HANDLE);
    vkl_bindings_buffer(bindings, src->slot_idx, src->u.b.br);
}



void vkl_visual_texture(VklVisual* visual, VklSourceType source, uint32_t idx, VklTexture* texture)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* src = vkl_bake_source(visual, source, idx);
    if (src == NULL)
    {
        log_error("Data source for source %d #%d could not be found", source, idx);
        return;
    }
    ASSERT(src != NULL);
    ASSERT(texture != NULL);

    src->u.t.texture = texture;
    src->origin = VKL_SOURCE_ORIGIN_USER;

    VklBindings* bindings = _get_bindings(visual, src);
    ASSERT(texture->image != NULL);
    ASSERT(texture->sampler != NULL);
    vkl_bindings_texture(bindings, src->slot_idx, texture->image, texture->sampler);
}

// vkl_visual_texture_partial(visual, type, idx, texture, (uvec3){0}, (uvec3){0});
// }
// void vkl_visual_texture_partial(
//     VklVisual* visual, VklPropType type, uint32_t idx, //
//     VklTexture* texture, uvec3 offset, uvec3 shape)
// {
// ASSERT(visual != NULL);
// ASSERT(visual != NULL);
// VklSource* source = vkl_bake_source(visual, type, idx);
// if (source == NULL)
//     log_error("Data source for prop %d #%d could not be found", type, idx);
// ASSERT(source != NULL);
// if (shape[0] == 0)
//     shape[0] = texture->image->width;
// if (shape[1] == 0)
//     shape[1] = texture->image->height;
// if (shape[2] == 0)
//     shape[2] = texture->image->depth;

// source->is_set = true;
// source->binding = VKL_PROP_BINDING_TEXTURE;
// source->u.t.texture = texture;
// TODO: partial texture binding

// for (uint32_t i = 0; i < 3; i++)
// {
//     source->u.t.offset[i] = offset[i];
//     source->u.t.shape[i] = shape[i];
// }



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

void vkl_visual_callback_transform(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->callback_transform = callback;
}



void vkl_visual_callback_bake(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->callback_bake = callback;
}



void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    visual->callback_fill = callback;
}



void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data)
{
    // Called in a REFILL canvas callback.

    ASSERT(visual != NULL);
    ASSERT(visual->callback_fill != NULL);

    VklVisualFillEvent ev = {0};
    ev.clear_color = clear_color;
    ev.cmds = cmds;
    ev.cmd_idx = cmd_idx;
    ev.viewport = viewport;
    ev.user_data = user_data;

    visual->callback_fill(visual, ev);
    visual->canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

VklSource* vkl_bake_source(VklVisual* visual, VklSourceType source_type, uint32_t idx)
{
    ASSERT(visual != NULL);
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        if (visual->sources[i].source_type == source_type && visual->sources[i].source_idx == idx)
            return &visual->sources[i];
    }
    return NULL;
}



VklProp* vkl_bake_prop(VklVisual* visual, VklPropType prop_type, uint32_t idx)
{
    ASSERT(visual != NULL);
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        if (visual->props[i].prop_type == prop_type && visual->props[i].prop_idx == idx)
            return &visual->props[i];
    }
    log_error("prop with type %d #%d not found", prop_type, idx);
    return NULL;
}



VklSource* vkl_bake_prop_source(VklVisual* visual, VklProp* prop)
{
    ASSERT(visual != NULL);
    ASSERT(prop != NULL);

    return vkl_bake_source(visual, prop->source_type, prop->source_idx);
}



uint32_t vkl_bake_max_prop_size(VklVisual* visual, VklSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    VklProp* prop = NULL;
    VklArray* arr = NULL;
    uint32_t item_count = 0;
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        prop = &visual->props[i];
        if (prop->source_type == source->source_type && prop->source_idx == source->source_idx)
        {
            arr = &prop->arr_orig;
            ASSERT(arr != NULL);
            item_count = MAX(item_count, arr->item_count);
        }
    }
    ASSERT(item_count > 0);
    return item_count;
}



void vkl_bake_prop_copy(VklVisual* visual, VklProp* prop, uint32_t reps)
{
    ASSERT(prop != NULL);

    VklSource* source = vkl_bake_source(visual, prop->source_type, prop->source_idx);
    ASSERT(source != NULL);

    VkDeviceSize col_size = _get_dtype_size(prop->dtype);
    ASSERT(col_size > 0);

    ASSERT(prop->arr_orig.data != NULL);
    ASSERT(source->arr.data != NULL);
    ASSERT(prop->arr_orig.item_count <= source->arr.item_count);
    uint32_t item_count = prop->arr_orig.item_count;

    vkl_array_column(
        &source->arr, prop->offset, col_size, 0, item_count, item_count, prop->arr_orig.data);
}



void vkl_bake_source_alloc(VklVisual* visual, VklSource* source, uint32_t count)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    // Resize the source source.
    VklArray* arr = &source->arr;
    ASSERT(is_obj_created(&arr->obj));
    vkl_array_resize(arr, count);
}



void vkl_bake_source_fill(VklVisual* visual, VklSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    VklProp* prop = NULL;
    // Copy all associated props to the source array.
    for (uint32_t i = 0; i < visual->prop_count; i++)
    {
        prop = &visual->props[i];
        if (prop->source_type == source->source_type && prop->source_idx == source->source_idx)
        {
            vkl_bake_prop_copy(visual, prop, 1);
        }
    }
}



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

void vkl_visual_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data)
{
    ASSERT(visual != NULL);
    VklVisualDataEvent ev = {0};
    ev.viewport = viewport;
    ev.coords = coords;
    ev.user_data = user_data;

    if (visual->callback_transform != NULL)
    {
        log_trace("visual transform callback");
        // This callback updates some props data_trans
        visual->callback_transform(visual, ev);
    }

    // if (visual->callback_triangulation != NULL)
    // {
    //     log_trace("visual triangulation callback");
    //     // This callback updates some props data_triang
    //     visual->callback_triangulation(visual, ev);
    // }

    if (visual->callback_bake != NULL)
    {
        log_trace("visual bake callback");
        // This callback does the following:
        // 1. Determine vertex count and index count
        // 2. Resize the VERTEX and INDEX array sources accordingly.
        // 3. Possibly resize other sources.
        // 4. Take the props and fill the array sources.
        visual->callback_bake(visual, ev);
    }

    // Here, we assume that all sources are correctly allocated, which includes VERTEX and INDEX
    // arrays, and that they have their data ready for upload.

    // Upload the buffers and textures
    VklSource* source = NULL;
    VklArray* arr = NULL;
    VklBufferRegions* br = NULL;
    VklTexture* texture = NULL;
    VklContext* ctx = visual->canvas->gpu->context;
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        source = &visual->sources[i];
        arr = &source->arr;
        if (source->source_type == VKL_SOURCE_TEXTURE)
        {
            texture = source->u.t.texture;

            // Only upload if the library is managing the GPU object, otherwise the user
            // is expected to do it manually
            if (source->origin == VKL_SOURCE_ORIGIN_LIB)
            {
                // TODO: create or resize as a function of the data array size
                vkl_upload_texture(ctx, texture, arr->item_count * arr->item_size, arr->data);
            }
        }
        else
        {
            br = &source->u.b.br;

            // Only upload if the library is managing the GPU object, otherwise the user
            // is expected to do it manually
            if (source->origin == VKL_SOURCE_ORIGIN_LIB)
            {
                // TODO: create or resize as a function of the data array size
                vkl_upload_buffers(ctx, *br, source->u.b.offset, source->u.b.size, arr->data);
            }
        }
    }

    // Update the bindings that need to be updated.
    for (uint32_t i = 0; i < visual->graphics_count; i++)
        if (visual->bindings[i].obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            vkl_bindings_update(&visual->bindings[i]);
    for (uint32_t i = 0; i < visual->compute_count; i++)
        if (visual->bindings_comp[i].obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            vkl_bindings_update(&visual->bindings_comp[i]);
}
