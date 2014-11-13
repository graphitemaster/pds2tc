#ifndef S2TC_HDR
#define S2TC_HDR
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*!__cplusplus*/

#define S2TC_OOM -1 /** Out of memory */

/**
 * Type: s2tc_dxt_mode_t
 *  The DXT mode to use for a context
 */
typedef enum {
    S2TC_DXT1,
    S2TC_DXT3,
    S2TC_DXT5
} s2tc_dxt_mode_t;

/**
 * Type: s2tc_dist_mode_t
 *  The color distance mode to use for a context.
 */
typedef enum {
    S2TC_RGB,
    S2TC_SRGB,
    S2TC_YUV,
    S2TC_AVG
} s2tc_dist_mode_t;

typedef void *(*s2tc_malloc_t)(size_t);
typedef void (*s2tc_free_t)(void *);

/**
 * Type: s2tc_t
 *  Opaque handle to a context.
 */
typedef struct s2tc_s s2tc_t;

/**
 * Function: s2tc_init
 *  Initialize a context
 *
 * Parameters:
 *  ctx  - context
 *  dxt  - dxt
 *  dist - color distance mode
 */
void s2tc_init(s2tc_t *ctx, s2tc_dxt_mode_t dxt, s2tc_dist_mode_t dist);

/**
 * Function: s2tc_set_memory_function
 *  Set the memory functions used for S2TC
 *
 * Parameters:
 *  ctx     - context
 *  alloc   - malloc-like function
 *  dealloc - free-like function
 */
void s2tc_set_memory_functions(s2tc_t *ctx, s2tc_malloc_t alloc, s2tc_free_t dealloc);

/**
 * Function: s2tc_set_mode_dist
 *  Set the color distance mode used
 *
 * Parameters:
 *  ctx  - context
 *  mode - the mode
 */
void s2tc_set_mode_dist(s2tc_t *ctx, s2tc_dist_mode_t dist);

/**
 * Function: s2tc_set_mode_dxt
 *  Set the DXT mode used
 *
 * Parameters:
 *  ctx  - context
 *  mode - the mode
 */
void s2tc_set_mode_dxt(s2tc_t *ctx, s2tc_dxt_mode_t dxt);

/**
 * Function: s2tc_encode_block
 *  Encode a block of RGBA5658.
 *
 * Parameters:
 *  ctx  - context
 *  out  - 16 byte storage for the block
 *  rgba - pointer to RGBA data for this block
 *  iw   - image width
 *  w    - width
 *  h    - height
 *
 * Returns:
 *  Zero on success, *S2TC_OOM* on out-of-memory.
 */
int s2tc_encode_block(s2tc_t *ctx, unsigned char *out, const unsigned char *rgba, size_t iw, size_t w, size_t h);

/* Missing refinement and rgb565-ification */

#ifdef __cplusplus
}
#endif /*!__cplusplus*/

#endif
