#ifndef S2TC_HDR
#define S2TC_HDR
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*!__cplusplus*/

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

/**
 * Type: s2tc_t
 *  A context for S2TC.
 *
 * To be filled out with a DXT and color distance mode before encoding blocks.
 */
typedef struct {
    s2tc_dxt_mode_t dxt; /** DXT mode */
    s2tc_dist_mode_t dist; /** Color distance mode */
} s2tc_t;

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
 */
void s2tc_encode_block(s2tc_t *ctx, unsigned char *out, const unsigned char *rgba, size_t iw, size_t w, size_t h);

/* Missing refinement and rgb565-ification */

#ifdef __cplusplus
}
#endif /*!__cplusplus*/

#endif
