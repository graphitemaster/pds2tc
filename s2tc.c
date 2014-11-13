#include <string.h> /* memcpy */
#include <assert.h>

#include "s2tc.h"

/* Min and max macros */
#define S2TC_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define S2TC_MAX(A, B) (((A) > (B)) ? (A) : (B))
/* Right shift and round */
#define RSHIFT(A, N) (((A) + (1 << ((N) - 1))) >> (N))
/* Color subtraction */
#define CSUB(A, B, I) ((A)[(I)] - (B)[(I)])
/* Color less-than predicate */
#define CLT(A, B) \
     (CSUB(A, B, 0) \
        ? (CSUB(A, B, 0) < 0) \
        : (CSUB(A, B, 1) \
            ? (CSUB(A, B, 1) < 0) \
            : (CSUB(A, B, 2) < 0)))
/* Bit set utility */
#define BSET(ARRAY, INDEX, OFFSET, BIT) \
    ((ARRAY)[(INDEX) / 8 + (OFFSET)] |= (BIT << ((INDEX) % 8)))

/* Color distance functions */
typedef signed char s2tc_color_t[3];

static int color_dist_avg(s2tc_color_t a, s2tc_color_t b) {
    int dr = a[0] - b[0];
    int dg = a[1] - b[1];
    int db = a[2] - b[2];
    return dr*dr + dg*dg + db*db;
}

static int color_dist_yuv(s2tc_color_t a, s2tc_color_t b) {
    int dr = a[0] - b[0];
    int dg = a[1] - b[1];
    int db = a[2] - b[2];
    int y = dr * 30*2 + dg * 59 + db * 11*2;
    int u = dr * 202 - y;
    int v = db * 202 - y;
    return ((y*y) << 1) + RSHIFT(u*u, 3) + RSHIFT(v*v, 4);
}

static int color_dist_rgb(s2tc_color_t a, s2tc_color_t b) {
    int dr = a[0] - b[0];
    int dg = a[1] - b[1];
    int db = a[2] - b[2];
    int y = dr * 21*2 + dg * 72 + db * 7*2;
    int u = dr * 202 - y;
    int v = db * 202 - y;
    return ((y*y) << 1) + RSHIFT(u*u, 3) + RSHIFT(v*v, 4);
}

static int color_dist_srgb(s2tc_color_t a, s2tc_color_t b) {
    int dr = a[0] * (int)a[0] - b[0] * (int)b[0];
    int dg = a[1] * (int)a[1] - b[1] * (int)b[1];
    int db = a[2] * (int)a[2] - b[2] * (int)b[2];
    int y = dr * 21*2*2 + dg * 72 + db * 7*2*2;
    int u = dr * 409 - y;
    int v = db * 409 - y;
    int sy = RSHIFT(y, 3) * RSHIFT(y, 4);
    int su = RSHIFT(u, 3) * RSHIFT(u, 4);
    int sv = RSHIFT(v, 3) * RSHIFT(v, 4);
    return RSHIFT(sy, 4) + RSHIFT(su, 8) + RSHIFT(sv, 9);
}

static int (*color_dist_functions[])(s2tc_color_t, s2tc_color_t) = {
    [S2TC_RGB] = &color_dist_rgb,
    [S2TC_SRGB] = &color_dist_srgb,
    [S2TC_AVG] = &color_dist_avg,
    [S2TC_YUV] = &color_dist_yuv
};

static int color_dist(s2tc_t *ctx, s2tc_color_t a, s2tc_color_t b) {
    return color_dist_functions[ctx->dist](a, b);
}

static int alpha_dist(unsigned char a, unsigned char b) {
    return (a - (int)b) * (a - (int)b);
}

/* Reduce colors inplace */
static void reduce_colors(s2tc_t *ctx, s2tc_color_t *colors, size_t n, size_t m) {
    assert(n <= 16 && m <= 16);
    int dists[16][16];
    size_t i = 0;
    for (; i < n; ++i) {
        dists[i][i] = 0;
        for (size_t j = i + 1; j < n; ++j) {
            const int distance = color_dist(ctx, colors[i], colors[j]);
            dists[i][j] = distance;
            dists[j][i] = distance;
        }
    }
    for (; i < m; ++i) {
        for (size_t j = 0; j < n; ++j)
            dists[i][j] = color_dist(ctx, colors[i], colors[j]);
    }
    size_t besti = 0;
    size_t bestj = 0;
    int bestsum = -1;
    for (i = 0; i < m; ++i) {
        for (size_t j = i + 1; j < m; ++j) {
            int sum = 0;
            for (size_t k = 0; k < n; ++k)
                sum += S2TC_MIN(dists[i][k], dists[j][k]);
            if (bestsum < 0 || sum < bestsum) {
                bestsum = sum;
                besti = i;
                bestj = j;
            }
        }
    }
    if (besti != 0) memcpy(&colors[0], &colors[besti], sizeof(s2tc_color_t));
    if (bestj != 1) memcpy(&colors[1], &colors[bestj], sizeof(s2tc_color_t));
}

static void reduce_colors_alpha(unsigned char *colors, size_t n, size_t m) {
    assert(n <= 16 && m <= 16);
    int dists[16][16+2];
    size_t i = 0;
    for (; i < n; ++i) {
        dists[i][i] = 0;
        for (size_t j = i + 1; j < n; ++j) {
            const int distance = alpha_dist(colors[i], colors[j]);
            dists[i][j] = distance;
            dists[j][i] = distance;
        }
    }
    for (; i < m; ++i)
        for (size_t j = 0; j < n; ++j)
            dists[i][j] = alpha_dist(colors[i], colors[j]);
    for (size_t j = 0; j < n; ++j)
        dists[m][j] = alpha_dist(0, colors[j]);
    for (size_t j = 0; j < n; ++j)
        dists[m+1][j] = alpha_dist(255, colors[j]);
    size_t besti = 0;
    size_t bestj = 1;
    int bestsum = -1;
    for (i = 0; i < m; ++i) {
        for (size_t j = i + 1; j < m; ++j) {
            int sum = 0;
            for (size_t k = 0; k < n; ++k) {
                const int di = dists[i][k];
                const int dj = dists[j][k];
                const int d0 = dists[m][k];
                const int d1 = dists[m+1][k];
                const int m0 = S2TC_MIN(di, dj);
                const int m1 = S2TC_MIN(d0, d1);
                sum += S2TC_MIN(m0, m1);
            }
            if (bestsum < 0 || sum < bestsum) {
                bestsum = sum;
                besti = i;
                bestj = j;
            }
        }
    }
    if (besti != 0) memcpy(&colors[0], &colors[besti], sizeof(s2tc_color_t));
    if (bestj != 1) memcpy(&colors[1], &colors[bestj], sizeof(s2tc_color_t));
}

void s2tc_encode_block(s2tc_t *ctx, unsigned char *out, const unsigned char *rgba, size_t iw, size_t w, size_t h) {
    s2tc_color_t colors[16];
    unsigned char alpha[16];
    size_t n = 0;
    size_t m = 0;
    for (size_t x = 0; x < w; ++x) {
        for (size_t y = 0; y < h; ++y, ++n) {
            colors[n][0] = rgba[(x + y * iw) * 4 + 2];
            colors[n][1] = rgba[(x + y * iw) * 4 + 1];
            colors[n][2] = rgba[(x + y * iw) * 4 + 0];
            if (ctx->dxt == S2TC_DXT5)
                alpha[n] = rgba[(x + y * iw) * 4 + 3];
        }
    }
    m = n;
    reduce_colors(ctx, colors, n, m);
    if (ctx->dxt == S2TC_DXT5) {
        reduce_colors_alpha(alpha, n, m);
        if (alpha[1] < alpha[0]) {
            alpha[2] = alpha[0];
            alpha[0] = alpha[1];
            alpha[1] = alpha[2];
        }
    }
    if (CLT(colors[0], colors[1])) {
        memcpy(&colors[2], &colors[0], sizeof(s2tc_color_t));
        memcpy(&colors[0], &colors[1], sizeof(s2tc_color_t));
        memcpy(&colors[1], &colors[2], sizeof(s2tc_color_t));
    }
    memset(out, 0, ctx->dxt == S2TC_DXT1 ? 8 : 16);
    switch (ctx->dxt) {
        case S2TC_DXT5:
            out[0] = alpha[0];
            out[1] = alpha[1];
            /* fall-through */
        case S2TC_DXT3:
            out[8] = ((colors[0][1] & 0x07) << 5) | colors[0][2];
            out[9] = (colors[0][0] << 3) | (colors[0][1] >> 3);
            out[10] = ((colors[1][1] & 0x07) << 5) | colors[1][2];
            out[11] = (colors[1][0] << 3) | (colors[1][1] >> 3);
            break;
        case S2TC_DXT1:
            out[0] = ((colors[0][1] & 0x07) << 5) | colors[0][2];
            out[1] = (colors[0][0] << 3) | (colors[0][1] >> 3);
            out[2] = ((colors[1][1] & 0x07) << 5) | colors[1][2];
            out[3] = (colors[1][0] << 3) | (colors[1][1] >> 3);
            break;
    }
    for (size_t x = 0; x < w; ++x) {
        for (size_t y = 0; y < h; ++y) {
            size_t index = x + y * 4;
            colors[2][0] = rgba[(x + y * iw) * 4 + 2];
            colors[2][1] = rgba[(x + y * iw) * 4 + 1];
            colors[2][2] = rgba[(x + y * iw) * 4 + 0];
            alpha[2] = rgba[(x + y * iw) * 4 + 3];
            int alphadist[4];
            switch (ctx->dxt) {
                case S2TC_DXT5:
                    #define ALPHA_LTEQ(X, Y) (alphadist[(X)] <= alphadist[(Y)])
                    alphadist[0] = alpha_dist(alpha[0], alpha[2]);
                    alphadist[1] = alpha_dist(alpha[1], alpha[2]);
                    alphadist[2] = alpha_dist(0, alpha[2]);
                    alphadist[3] = alpha_dist(255, alpha[2]);
                    if (ALPHA_LTEQ(2, 0) && ALPHA_LTEQ(2, 1) && ALPHA_LTEQ(2, 3)) {
                        BSET(out, index * 3 + 1, 2, 1);
                        BSET(out, index * 3 + 2, 2, 1);
                    } else if (ALPHA_LTEQ(3, 0) && ALPHA_LTEQ(3, 1)) {
                        BSET(out, index * 3 + 0, 2, 1);
                        BSET(out, index * 3 + 1, 2, 1);
                        BSET(out, index * 3 + 2, 2, 1);
                    } else if (ALPHA_LTEQ(0, 1)) {
                        /* nothing */
                    } else {
                        BSET(out, index * 3 + 0, 2, 1);
                    }
                    if (color_dist(ctx, colors[0], colors[2]) > color_dist(ctx, colors[1], colors[2]))
                        BSET(out, index * 2, 12, 1);
                    break;
                case S2TC_DXT3:
                    BSET(out, index * 4, 0, alpha[2]);
                    if (color_dist(ctx, colors[0], colors[2]) > color_dist(ctx, colors[1], colors[2]))
                        BSET(out, index * 2, 12, 1);
                    break;
                case S2TC_DXT1:
                    if (!alpha[2])
                        BSET(out, index * 2, 4, 3);
                    else if (color_dist(ctx, colors[0], colors[2]) > color_dist(ctx, colors[1], colors[2]))
                        BSET(out, index * 2, 4, 1);
                    break;
            }
        }
    }
}
