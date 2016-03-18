#ifndef ANDROID_HWCOMPOSER_UTILS_H
#define ANDROID_HWCOMPOSER_UTILS_H

#include "FimgApi.h"

bool is_yuv_format(unsigned int color_format);
uint8_t format_to_bpp(int format);
enum s3c_fb_pixel_format format_to_s3c_format(int format);
enum s3c_fb_blending blending_to_s3c_blending(int32_t blending);
unsigned int formatValueHAL2G2D(int hal_format,
        color_format *g2d_format,
        pixel_order *g2d_order,
        uint32_t *g2d_bpp);

#endif //ANDROID_HWCOMPOSER_UTILS_H
