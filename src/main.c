#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

// Half the trajectory; the (-x, -y, z) mirror provides the other half.
#define MAX_POINTS 10000/4

// Adaptive time step bounds and target arc length for precomputation.
#define DT_MIN 0.001f
#define DT_MAX 0.02f
#define TARGET_ARC_LENGTH 0.1f
// Scale the attractor values into the Q4.4 range.
// 0.15 uses ~75% of int8_t range for Lorenz z (48 * 0.15 * 16 = 115).
#define STORE_SCALE 0.15f

// Q4.4 fixed-point conversion: multiply by 16 to encode.
#define FLOAT_TO_FIXED4_4(F) ((int8_t)((F) * 16.0f))

// Perspective distance in fixed-point units (= 100 * STORE_SCALE * 16).
#define D_FX 240
// Display scale in Q8.8 (1.0 * 256 = 256). Controls on-screen size.
#define DISPLAY_SCALE_Q8 256

// Depth coloring: 8 palette entries for a near-to-far gradient.
#define NUM_DEPTH_COLORS 8
#define DEPTH_PALETTE_START 1

/*
   Pack three 8-bit fixed-point (Q4.4) values into a single 24-bit int.
   We store:
     - x in bits 16-23,
     - y in bits 8-15,
     - z in bits 0-7.
   On the TI-84 CE Plus, ints are natively 24-bit.
*/
inline int pack3(int8_t x, int8_t y, int8_t z) {
    return (((uint8_t)x) << 16) | (((uint8_t)y) << 8) | ((uint8_t)z);
}

// Unpack a 24-bit int into three 8-bit fixed-point values.
inline void unpack3(int packed, int8_t* x, int8_t* y, int8_t* z) {
    *x = (int8_t)((packed >> 16) & 0xFF);
    *y = (int8_t)((packed >> 8) & 0xFF);
    *z = (int8_t)(packed & 0xFF);
}

// Global array to store the Lorenz attractor points as packed 24-bit ints.
static int packedPoints[MAX_POINTS];

// Per-point render body: Y-axis early reject, factored rotation for original
// + mirror (-x, -y, z), depth coloring. Uses outer-scope cosA, sinA,
// perspLUT, xOffset, yOffset, yThreshold, and graphx globals.
#define PROCESS_POINT(idx) do { \
    int8_t fx_, fy_, fz_; \
    unpack3(packedPoints[(idx)], &fx_, &fy_, &fz_); \
    if (fy_ > yThreshold || fy_ < -yThreshold) break; \
    int a_ = fx_ * cosA; \
    int b_ = fz_ * sinA; \
    int c_ = fx_ * sinA; \
    int d_ = fz_ * cosA; \
    int xRot_ = (a_ + b_) >> 8; \
    int zRot_ = (-c_ + d_) >> 8; \
    int zIdx_ = zRot_ < -128 ? -128 : (zRot_ > 127 ? 127 : zRot_); \
    int factor_ = perspLUT[zIdx_ + 128]; \
    int screenX_ = xOffset + ((xRot_ * factor_) >> 8); \
    int screenY_ = yOffset - ((fy_ * factor_) >> 8); \
    if ((unsigned int)screenX_ < GFX_LCD_WIDTH && \
        (unsigned int)screenY_ < GFX_LCD_HEIGHT) { \
        int colorIdx_ = ((zIdx_ + 128) * (NUM_DEPTH_COLORS - 1)) >> 8; \
        gfx_SetColor(DEPTH_PALETTE_START + colorIdx_); \
        gfx_SetPixel(screenX_, screenY_); \
    } \
    int xRotM_ = (-a_ + b_) >> 8; \
    int zRotM_ = (c_ + d_) >> 8; \
    int zIdxM_ = zRotM_ < -128 ? -128 : (zRotM_ > 127 ? 127 : zRotM_); \
    int factorM_ = perspLUT[zIdxM_ + 128]; \
    int screenXM_ = xOffset + ((xRotM_ * factorM_) >> 8); \
    int screenYM_ = yOffset + ((fy_ * factorM_) >> 8); \
    if ((unsigned int)screenXM_ < GFX_LCD_WIDTH && \
        (unsigned int)screenYM_ < GFX_LCD_HEIGHT) { \
        int colorIdxM_ = ((zIdxM_ + 128) * (NUM_DEPTH_COLORS - 1)) >> 8; \
        gfx_SetColor(DEPTH_PALETTE_START + colorIdxM_); \
        gfx_SetPixel(screenXM_, screenYM_); \
    } \
} while (0)

// Lookup tables for trig (Q8.8) and perspective (Q8.8 with display scale folded in).
static int sinLUT[256];
static int cosLUT[256];
static int perspLUT[256];

int main(void) {
    int numPoints = 0;

    // Lorenz attractor parameters.
    const float sigma = 10.0f;
    const float beta  = 8.0f / 3.0f;
    const float rho   = 28.0f;

    // Initial conditions.
    float x = 0.1f, y = 0.0f, z = 0.0f;

    // Precompute the Lorenz attractor points.
    // We scale the values by STORE_SCALE and encode as Q4.4 fixed-point.
    for (int i = 0; i < MAX_POINTS; i++) {
        float dx = sigma * (y - x);
        float dy = x * (rho - z) - y;
        float dz = x * y - beta * z;

        // Adaptive time step: smaller steps in fast regions, larger in slow.
        float speed = sqrtf(dx * dx + dy * dy + dz * dz);
        float dt = TARGET_ARC_LENGTH / (speed > 0.001f ? speed : 0.001f);
        dt = dt < DT_MIN ? DT_MIN : (dt > DT_MAX ? DT_MAX : dt);

        x += dt * dx;
        y += dt * dy;
        z += dt * dz;

        // Scale and convert to Q4.4 fixed-point.
        int8_t fx = FLOAT_TO_FIXED4_4(x * STORE_SCALE);
        int8_t fy = FLOAT_TO_FIXED4_4(y * STORE_SCALE);
        int8_t fz = FLOAT_TO_FIXED4_4(z * STORE_SCALE);

        // Pack all three coordinates into one int.
        packedPoints[i] = pack3(fx, fy, fz);
        numPoints++;
    }

    // Build trig lookup tables (256 entries, Q8.8 format).
    for (int i = 0; i < 256; i++) {
        float rad = (float)i * (2.0f * 3.14159265f / 256.0f);
        sinLUT[i] = (int)(sinf(rad) * 256.0f);
        cosLUT[i] = (int)(cosf(rad) * 256.0f);
    }

    // Build perspective lookup table: maps zRot to (D_FX * DISPLAY_SCALE_Q8) / (D_FX + zRot).
    // Index i corresponds to zRot = i - 128.
    for (int i = 0; i < 256; i++) {
        int denom = D_FX + (i - 128);
        if (denom < 1) denom = 1;
        perspLUT[i] = (D_FX * DISPLAY_SCALE_Q8) / denom;
    }

    // Maximum perspective factor occurs at zRot = -128 (closest to viewer).
    // Any point with |fy| above this threshold is off-screen at all depths.
    int maxFactor = perspLUT[0];
    int yThreshold = (((GFX_LCD_HEIGHT / 2) << 8) + maxFactor - 1) / maxFactor;

    // Initialize graphics with double buffering.
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_FillScreen(0); // Clear the back buffer (black background).

    // Depth palette: white (near) to dark blue (far).
    gfx_palette[1] = gfx_RGBTo1555(255, 255, 255);
    gfx_palette[2] = gfx_RGBTo1555(200, 220, 255);
    gfx_palette[3] = gfx_RGBTo1555(150, 180, 255);
    gfx_palette[4] = gfx_RGBTo1555(100, 140, 220);
    gfx_palette[5] = gfx_RGBTo1555(70, 100, 180);
    gfx_palette[6] = gfx_RGBTo1555(50, 70, 140);
    gfx_palette[7] = gfx_RGBTo1555(30, 40, 100);
    gfx_palette[8] = gfx_RGBTo1555(15, 20, 60);

    // Screen center offsets.
    int xOffset = GFX_LCD_WIDTH / 2;
    int yOffset = GFX_LCD_HEIGHT / 2;

    // Rotation state: uint8_t index into 256-entry LUT.
    // Step of 2 per frame ~ 0.049 rad/frame, matching original 0.05.
    // Wraps naturally via uint8_t overflow.
    uint8_t angle_idx = 0;
    uint8_t angle_step = 2;

    uint8_t frameCount = 0;

    // Animation loop: redraw stored points with the current rotation.
    while (true) {
        gfx_FillScreen(0);  // Clear the back buffer.

        int cosA = cosLUT[angle_idx];
        int sinA = sinLUT[angle_idx];

        // Temporal dithering: draw even-indexed points on even frames,
        // odd-indexed on odd frames. Halves per-frame work; LCD persistence
        // makes alternating frames appear continuous.
        // Unrolled 4x to reduce branch overhead on the eZ80.
        int start = frameCount & 1;
        int unrollLimit = numPoints - 7;
        int i = start;
        for (; i <= unrollLimit; i += 8) {
            PROCESS_POINT(i);
            PROCESS_POINT(i + 2);
            PROCESS_POINT(i + 4);
            PROCESS_POINT(i + 6);
        }
        for (; i < numPoints; i += 2) {
            PROCESS_POINT(i);
        }

        gfx_SwapDraw();          // Present the frame.
        angle_idx += angle_step; // Advance rotation (wraps naturally).
        frameCount++;            // Advance frame counter (wraps naturally).

        kb_Scan();
        if (kb_Data[6] & kb_Clear) {  // Exit on [CLEAR].
            break;
        }
    }

    gfx_End();
    return 0;
}
