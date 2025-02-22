#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#define MAX_POINTS 10000/2
#define DT 0.005f
// Scale the attractor values into the Q4.4 range.
#define STORE_SCALE 0.1f
// Inverse of STORE_SCALE to recover original values.
#define INV_STORE_SCALE (1.0f / STORE_SCALE)

// Q4.4 fixed-point conversion functions.
// Multiply by 16 to convert a float to Q4.4.
#define FLOAT_TO_FIXED4_4(F) (int8_t)(F * 16.0f)
int8_t float_to_fixed4_4(float f) {
    return (int8_t)(f * 16.0f);
}

// Divide by 16 to convert from Q4.4 back to float.
inline float fixed4_4_to_float(int8_t f) {
    return ((float)f) / 16.0f;
}

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

int main(void) {
    int numPoints = 0;
    
    // Lorenz attractor parameters.
    const float sigma = 10.0f;
    const float beta  = 8.0f / 3.0f;
    const float rho   = 28.0f;
    
    // Initial conditions.
    float x = 0.1f, y = 0.0f, z = 0.0f;
    
    // Precompute the Lorenz attractor points.
    // We scale the values by STORE_SCALE before converting to Q4.4 fixed-point.
    for (int i = 0; i < MAX_POINTS; i++) {
        float dx = sigma * (y - x);
        float dy = x * (rho - z) - y;
        float dz = x * y - beta * z;
        
        x += DT * dx;
        y += DT * dy;
        z += DT * dz;
        
        // Scale and convert to Q4.4 fixed-point.
        int8_t fx = (int8_t)(x * STORE_SCALE);
        int8_t fy = (int8_t)(y * STORE_SCALE);
        int8_t fz = (int8_t)(z * STORE_SCALE);
        
        // Pack all three coordinates into one int.
        packedPoints[i] = pack3(fx, fy, fz);
        numPoints++;
    }
    
    // Initialize graphics for direct drawing.
    gfx_Begin();
    gfx_SetDrawScreen();
    gfx_SetColor(255); // Set drawing color to white.
    gfx_FillScreen(0); // Clear the screen (black background).
    
    // Screen mapping parameters.
    int xOffset = GFX_LCD_WIDTH / 2;
    int yOffset = GFX_LCD_HEIGHT / 2;
    float scaleX = 4.0f;
    float scaleY = 4.0f;
    
    // Rotation and perspective parameters.
    float angle = 0.0f;      // Starting rotation angle.
    float rotSpeed = 0.05f;  // Rotation speed (radians per frame).
    float d = 100.0f;        // Perspective distance.
    
    // Animation loop: redraw all stored points with the current rotation.
    while (true) {
        gfx_FillScreen(0);  // Clear the screen.
        
        const float cosTheta = cosf(angle);
        const float sinTheta = sinf(angle);
        
        // Process each stored point.
        for (int i = 0; i < numPoints; i++) {
            int8_t fx, fy, fz;
            unpack3(packedPoints[i], &fx, &fy, &fz);
            
            // Convert fixed-point values back to float and recover original scale.
            float orig_x = (((float)fx) / 16.0f) * INV_STORE_SCALE;
            float orig_y = (((float)fy) / 16.0f) * INV_STORE_SCALE;
            float orig_z = (((float)fz) / 16.0f) * INV_STORE_SCALE;
            
            // Rotate the point around the y-axis.
            float xRot = orig_x * cosTheta + orig_z * sinTheta;
            float zRot = -orig_x * sinTheta + orig_z * cosTheta;
            float yVal = orig_y;  // y remains unchanged.
            
            // Apply a simple perspective projection.
            float factor = d / (d + zRot);
            
            // Map the 3D point to 2D screen coordinates.
            int screenX = xOffset + (int)(xRot * factor * scaleX);
            int screenY = yOffset - (int)(yVal * factor * scaleY);
            
            // Draw the pixel if it's within screen bounds.
            if (screenX >= 0 && screenX < GFX_LCD_WIDTH &&
                screenY >= 0 && screenY < GFX_LCD_HEIGHT) {
                gfx_SetPixel(screenX, screenY);
            }
        }
        
        gfx_SwapDraw();  // Display the frame.
        angle += rotSpeed;  // Update the rotation angle.
        
        kb_Scan();
        if (kb_Data[6] & kb_Clear) {  // Exit on [CLEAR].
            break;
        }
    }
    
    gfx_End();
    return 0;
}
