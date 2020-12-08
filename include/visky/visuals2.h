#ifndef VKL_VISUALS_HEADER
#define VKL_VISUALS_HEADER

#include "array.h"
#include "context.h"
#include "graphics.h"
#include "vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_GRAPHICS_PER_VISUAL 256
#define VKL_MAX_COMPUTES_PER_VISUAL 256
#define VKL_MAX_VISUAL_GROUPS       16384
#define VKL_MAX_VISUAL_SOURCES      256
#define VKL_MAX_VISUAL_RESOURCES    256
#define VKL_MAX_VISUAL_PROPS        256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Pipeline types
typedef enum
{
    VKL_PIPELINE_GRAPHICS,
    VKL_PIPELINE_COMPUTE,
} VklPipelineType;



// Prop types.
typedef enum
{
    VKL_PROP_NONE,
    VKL_PROP_POS,
    VKL_PROP_COLOR,
    VKL_PROP_TYPE,
} VklPropType;



// Source types.
typedef enum
{
    VKL_SOURCE_NONE,
    VKL_SOURCE_VERTEX,
    VKL_SOURCE_INDEX,
    VKL_SOURCE_UNIFORM,
    VKL_SOURCE_STORAGE,
    VKL_SOURCE_TEXTURE,
} VklSourceType;



// Data source origin.
typedef enum
{
    VKL_SOURCE_ORIGIN_NONE, // not set
    VKL_SOURCE_ORIGIN_LIB,  // the GPU buffer or texture is handled by visky's visual module
    VKL_SOURCE_ORIGIN_USER, // the GPU buffer or texture is handled by the user
} VklSourceOrigin;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVisual VklVisual;
typedef struct VklProp VklProp;

typedef struct VklSourceBuffer VklSourceBuffer;
typedef struct VklSourceTexture VklSourceTexture;
typedef union VklSourceUnion VklSourceUnion;
typedef struct VklSource VklSource;
typedef struct VklDataCoords VklDataCoords;

typedef struct VklVisualFillEvent VklVisualFillEvent;
typedef struct VklVisualDataEvent VklVisualDataEvent;

typedef uint32_t VklIndex;



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

typedef void (*VklVisualFillCallback)(VklVisual* visual, VklVisualFillEvent ev);
/*
called by the scene event callback in response to a REFILL event
default fill callback: viewport, bind vbuf, ibuf, etc. bind the first graphics only and no
compute...
*/



typedef void (*VklVisualDataCallback)(VklVisual* visual, VklVisualDataEvent ev);
/*
called by the scene event callback in response to a DATA event
baking process
visual data sources, item count, groups ==> bindings, vertex buffer, index buffer
enqueue data transfers
*/



/*************************************************************************************************/
/*  Source structs                                                                               */
/*************************************************************************************************/

struct VklDataCoords
{
    dvec4 data; // (blx, bly, trx, try)
    vec4 gpu;   // (blx, bly, trx, try)
};



struct VklSourceBuffer
{
    VklBufferRegions br;
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct VklSourceTexture
{
    VklTexture* texture;

    // TODO: not implemented yet:
    uvec3 offset;
    uvec3 shape;
};

union VklSourceUnion
{
    VklSourceBuffer b;
    VklSourceTexture t;
};

struct VklSource
{
    // Identifier of the prop
    VklPipelineType pipeline;  // graphics or compute pipeline?
    uint32_t pipeline_idx;     // idx of the pipeline within the graphics or compute pipelines
    VklSourceType source_type; // Vertex, index, uniform, storage, or texture
    uint32_t source_idx;       // idx among all sources of the same type
    uint32_t slot_idx;         // Binding slot, or 0 for vertex/index
    VklArray arr;              // array to be uploaded to that source

    VklSourceOrigin origin; // whether the underlying GPU object is handled by the user or visky
    VklSourceUnion u;
};



struct VklProp
{
    VklPropType prop_type;     // prop type
    uint32_t prop_idx;         // index within all props of that type
    VklSourceType source_type; // Vertex, index, uniform, storage, or texture
    uint32_t source_idx;       // Binding slot, or 0 for vertex/index

    uint32_t field_idx;
    VklDataType dtype;
    VkDeviceSize offset;

    VklArray arr_orig;  // original data array
    VklArray arr_trans; // transformed data array
    // VklArray arr_triang; // triangulated data array

    // bool is_set; // whether the user has set this prop
};



/*************************************************************************************************/
/*  Visual struct                                                                                */
/*************************************************************************************************/

struct VklVisual
{
    VklObject obj;
    VklCanvas* canvas;

    // Graphics.
    uint32_t graphics_count;
    VklGraphics* graphics[VKL_MAX_GRAPHICS_PER_VISUAL];

    // Computes.
    uint32_t compute_count;
    VklCompute* computes[VKL_MAX_COMPUTES_PER_VISUAL];

    // Fill callbacks.
    VklVisualFillCallback callback_fill;

    // Data callbacks.
    VklVisualDataCallback callback_transform;
    // VklVisualDataCallback callback_triangulation;
    VklVisualDataCallback callback_bake;

    // Sources.
    uint32_t source_count; // VERTEX source is mandatory
    VklSource sources[VKL_MAX_VISUAL_SOURCES];

    // Props.
    uint32_t prop_count;
    VklProp props[VKL_MAX_VISUAL_PROPS];

    // User data
    uint32_t group_count;
    uint32_t group_sizes[VKL_MAX_VISUAL_GROUPS];

    // GPU data
    uint32_t vertex_count;
    uint32_t index_count;

    VklBindings bindings[VKL_MAX_GRAPHICS_PER_VISUAL];
    VklBindings bindings_comp[VKL_MAX_COMPUTES_PER_VISUAL];
};



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

// passed to visual callback when it needs to refill the command buffers
struct VklVisualFillEvent
{
    VklCommands* cmds;
    uint32_t cmd_idx;
    VkClearColorValue clear_color;
    VklViewport viewport;
    void* user_data;
};



struct VklVisualDataEvent
{
    VklViewport viewport;
    VklDataCoords coords;
    const void* user_data;
};



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

VKY_EXPORT VklVisual vkl_visual(VklCanvas* canvas);

VKY_EXPORT void vkl_visual_destroy(VklVisual* visual);



// Define a new source. (source, source_idx) completely identifies a source within all pipelines
VKY_EXPORT void vkl_visual_source(
    VklVisual* visual, VklSourceType source, uint32_t source_idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx, uint32_t slot_idx, VkDeviceSize item_size);

VKY_EXPORT void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx,           //
    VklSourceType source, uint32_t source_idx,                   //
    uint32_t field_idx, VklDataType dtype, VkDeviceSize offset); //

VKY_EXPORT void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics);

VKY_EXPORT void vkl_visual_compute(VklVisual* visual, VklCompute* compute);



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size);

VKY_EXPORT void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t count, const void* data);

VKY_EXPORT void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data);



VKY_EXPORT void vkl_visual_buffer(
    VklVisual* visual, VklSourceType source, uint32_t source_idx, VklBufferRegions br);

VKY_EXPORT void vkl_visual_buffer_partial(
    VklVisual* visual, VklSourceType source, uint32_t source_idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size);

VKY_EXPORT void vkl_visual_texture(
    VklVisual* visual, VklSourceType source, uint32_t source_idx, VklTexture* texture);

// NOTE: not implemented yet, would need binding to partial texture
// VKY_EXPORT void vkl_visual_texture_partial(
//     VklVisual* visual, VklPropType type, uint32_t idx, //
//     VklTexture* texture, uvec3 offset, uvec3 shape);



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback);

VKY_EXPORT void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data);



VKY_EXPORT void vkl_visual_callback_transform(VklVisual* visual, VklVisualDataCallback callback);

VKY_EXPORT void vkl_visual_callback_bake(VklVisual* visual, VklVisualDataCallback callback);



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

VKY_EXPORT VklSource* vkl_bake_source(VklVisual* visual, VklSourceType source_type, uint32_t idx);

VKY_EXPORT VklProp* vkl_bake_prop(VklVisual* visual, VklPropType prop_type, uint32_t idx);

VKY_EXPORT VklSource* vkl_bake_prop_source(VklVisual* visual, VklProp* prop);

VKY_EXPORT uint32_t vkl_bake_max_prop_size(VklVisual* visual, VklSource* source);

VKY_EXPORT void vkl_bake_prop_copy(VklVisual* visual, VklProp* prop, uint32_t reps);

VKY_EXPORT void vkl_bake_source_alloc(VklVisual* visual, VklSource* source, uint32_t count);

VKY_EXPORT void vkl_bake_source_fill(VklVisual* visual, VklSource* source);



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data);



#endif
