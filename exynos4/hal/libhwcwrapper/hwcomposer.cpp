/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hwcomposer.h"
#include "hwcomposer_vsync.h"

const size_t BURSTLEN_BYTES = 16 * 8;
const size_t MAX_PIXELS = 12 * 1024 * 1000;

template<typename T> inline T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T min(T a, T b) { return (a < b) ? a : b; }

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Exynos4 HWComposer Wrapper",
        author: "The NamelessRom Project",
        methods: &hwc_module_methods,
        .dso = NULL, /* remove compilation warnings */
        .reserved = {0}, /* remove compilation warnings */
    },
};

/******************************************************************************/

static void dump_layer(hwc_layer_1_t const* l, const char *function) {
    private_handle_t *handle = (private_handle_t *) l->handle;

    if (handle) {
        ALOGD("%s \ttype=%d, flags=%08x, handle=%p, format=0x%x, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}", function,
                l->compositionType, l->flags, l->handle, handle->format, l->transform, l->blending,
                l->sourceCrop.left,
                l->sourceCrop.top,
                l->sourceCrop.right,
                l->sourceCrop.bottom,
                l->displayFrame.left,
                l->displayFrame.top,
                l->displayFrame.right,
                l->displayFrame.bottom);
    } else {
        ALOGD("%s \ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}", function,
                l->compositionType, l->flags, l->handle, l->transform, l->blending,
                l->sourceCrop.left,
                l->sourceCrop.top,
                l->sourceCrop.right,
                l->sourceCrop.bottom,
                l->displayFrame.left,
                l->displayFrame.top,
                l->displayFrame.right,
                l->displayFrame.bottom);
    }
}

static int dup_or_warn(int fence)
{
    int dup_fd = dup(fence);
    if (dup_fd < 0)
        ALOGW("fence dup failed: %s", strerror(errno));
    return dup_fd;
}

inline bool intersect(const hwc_rect &r1, const hwc_rect &r2)
{
    return !(r1.left > r2.right ||
        r1.right < r2.left ||
        r1.top > r2.bottom ||
        r1.bottom < r2.top);
}

inline hwc_rect intersection(const hwc_rect &r1, const hwc_rect &r2)
{
    hwc_rect i;
    i.top = max(r1.top, r2.top);
    i.bottom = min(r1.bottom, r2.bottom);
    i.left = max(r1.left, r2.left);
    i.right = min(r1.right, r2.right);
    return i;
}

static bool format_is_supported_by_fimg(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBA_4444:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    //case HAL_PIXEL_FORMAT_YV12: TODO - check, it is YCrCb 4:2:0 Planar
        //ALOGD("%s format=%d true", __FUNCTION__, format);
        return true;

    default:
        ALOGD("%s format=%d false", __FUNCTION__, format);
        return false;
    }
}

static bool format_requires_gscaler(int format)
{
    // TODO - In future, we'll try to use fimg and fimc to perform scaling/crop/csc operations
    // Hence we will have two gscaler units
    return ( (format_is_supported_by_fimg(format) /* || format_is_supported_by_fimc(format)*/ ) &&
             (format != HAL_PIXEL_FORMAT_RGBX_8888) && (format != HAL_PIXEL_FORMAT_RGB_565) );
}

static bool is_transformed(const hwc_layer_1_t &layer)
{
    return layer.transform != 0;
}

static bool is_rotated(const hwc_layer_1_t &layer)
{
    return (layer.transform & HAL_TRANSFORM_ROT_90) ||
            (layer.transform & HAL_TRANSFORM_ROT_180);
}

static bool is_scaled(const hwc_layer_1_t &layer)
{
    return WIDTH(layer.displayFrame) != WIDTH(layer.sourceCrop) ||
            HEIGHT(layer.displayFrame) != HEIGHT(layer.sourceCrop);
}

static uint8_t format_hal_to_bpp(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return 32;

    case HAL_PIXEL_FORMAT_RGB_565:
        return 16;

    default:
        ALOGW("%s: unrecognized pixel format %u", __FUNCTION__, format);
        return 0;
    }
}

static enum s3c_fb_pixel_format format_hal_to_s3c(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return S3C_FB_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return S3C_FB_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return S3C_FB_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return S3C_FB_PIXEL_FORMAT_BGRA_8888;
    default:
        return S3C_FB_PIXEL_FORMAT_MAX;
    }
}

static bool format_is_supported_by_s3c(int format)
{
    return format_hal_to_s3c(format) < S3C_FB_PIXEL_FORMAT_MAX;
}

static bool supports_fimg(const hwc_layer_1_t &layer, int format)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    int max_w = 8000; //taken from kernel: drivers/media/video/samsung/fimg2d4x-exynos4/fimg2d_ctx.c
    int max_h = 8000; //fimg2d_check_params()

    return format_is_supported_by_fimg(format) &&
            handle->stride <= max_w &&
            handle->height <= max_h;
            handle->height <= max_h;
}

static bool requires_gscaler(const hwc_layer_1_t &layer, int format)
{
    bool rc;

    rc = format_requires_gscaler(format) || is_scaled(layer)
            || is_transformed(layer); // TODO - check this -> || !is_x_aligned(layer, format);
    //ALOGD("%s rc=%d", __FUNCTION__, rc);
    return rc;
}

size_t visible_width(const hwc_layer_1_t &layer, int format, struct hwc_context_t *ctx)
{
    int bpp;
    if (requires_gscaler(layer, format))
        bpp = 32;
    else
        bpp = format_hal_to_bpp(format);
    int left = max(layer.displayFrame.left, 0);
    int right = min(layer.displayFrame.right, ctx->xres);

    return (right - left) * bpp / 8;
}

static bool blending_is_supported(int32_t blending)
{
    return blending_to_s3c_blending(blending) < S3C_FB_BLENDING_MAX;
}

bool is_offscreen(const hwc_layer_1_t &layer, struct hwc_context_t *ctx)
{
    return layer.displayFrame.left > ctx->xres ||
            layer.displayFrame.right < 0 ||
            layer.displayFrame.top > ctx->yres ||
            layer.displayFrame.bottom < 0;
}

bool supports_overlay(struct hwc_context_t *ctx, const hwc_layer_1_t &layer, size_t i)
{
    if (layer.flags & HWC_SKIP_LAYER) {
        ALOGD("\tlayer %u: skipping", i);
        return false;
    }

    private_handle_t *handle = (private_handle_t *) layer.handle;

    if (!handle) {
        ALOGD("\tlayer %u: handle is NULL", i);
        return false;
    }

    if (requires_gscaler(layer, handle->format)) {
        if (!supports_fimg(layer, handle->format)) {
            ALOGD("\tlayer %u: gscaler required but not supported format(0x%x)", i, handle->format);
            return false;
        }
    } else {
        if (!format_is_supported_by_s3c(handle->format)) {
            ALOGD("\tlayer %u: pixel format %u not supported", i, handle->format);
            return false;
        }
    }
    if (visible_width(layer, handle->format, ctx) < BURSTLEN_BYTES) {
        ALOGD("\tlayer %u: visible area is too narrow", i);
        return false;
    }
    if (!blending_is_supported(layer.blending)) {
        ALOGD("\tlayer %u: blending %d not supported", i, layer.blending);
        return false;
    }
    if (UNLIKELY(is_offscreen(layer, ctx))) {
        ALOGW("\tlayer %u: off-screen", i);
        return false;
    }

    return true;
}

void determineSupportedOverlays(hwc_context_t *ctx, hwc_display_contents_1_t *contents)
{
    private_handle_t* hnd = NULL;
    bool force_fb = ctx->force_gpu;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        ctx->win[i].layer_index = -1;

    ctx->fb_needed = false;
    ctx->first_fb = 0;
    ctx->last_fb = 0;
    ctx->hwc_layer = 0;

    // find unsupported overlays
    for (size_t i=0 ; i < contents->numHwLayers ; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        //dump_layer(&displays[0]->hwLayers[i]);

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) { //exynos5
            //ALOGD("\tlayer %u: framebuffer target", i);
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND && !force_fb) { //exynos5
            ALOGV("\tlayer %u: background supported", i);
            //dump_layer(&contents->hwLayers[i]);
            continue;
        }

        if (supports_overlay(ctx, contents->hwLayers[i], i) && !force_fb) { //exynos5
            //ALOGD("\tlayer %u: overlay supported", i);
            layer.compositionType = HWC_OVERLAY;
            ++ctx->hwc_layer;
            //dump_layer(&contents->hwLayers[i], __FUNCTION__);
            continue;
        }

/*      hnd = (private_handle_t*) layer.handle;
        if (hnd) {
            if (is_yuv_format(hnd->format)) {
                ALOGD("%s video format", __FUNCTION__);
                ++ctx->yuv_layer;
                layer.compositionType = HWC_OVERLAY;
                continue;
            }
        }*/

        if (!ctx->fb_needed) {
            ctx->first_fb = i;
            ctx->fb_needed = true;
        }
        ctx->last_fb = i;
        layer.compositionType = HWC_FRAMEBUFFER;
    }

    // odroid has this, check it
    ctx->first_fb = min(ctx->first_fb, (size_t)NUM_HW_WINDOWS-1);

    // can't composite overlays sandwiched between framebuffers
    if (ctx->fb_needed)
        for (size_t i = ctx->first_fb; i < ctx->last_fb; i++) {
            ALOGD("%s layer %d sandwiched between FB, set to FB too", __FUNCTION__, i);
            contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
        }
}

void determineBandwidthSupport(hwc_context_t *ctx, hwc_display_contents_1_t *contents)
{
    // Incrementally try to add our supported layers to hardware windows.
    // If adding a layer would violate a hardware constraint, force it
    // into the framebuffer and try again.  (Revisiting the entire list is
    // necessary because adding a layer to the framebuffer can cause other
    // windows to retroactively violate constraints.)
    bool changed;
    bool gsc_used;

    // TODO - ctx->mBypassSkipStaticLayer = false;

    do {
        android::Vector<hwc_rect> rects;
        android::Vector<hwc_rect> overlaps;
        size_t pixels_left, windows_left;

        gsc_used = false;

        if (ctx->fb_needed) {
            hwc_rect_t fb_rect;
            fb_rect.top = fb_rect.left = 0;
            fb_rect.right = ctx->xres - 1;
            fb_rect.bottom = ctx->yres - 1;
            pixels_left = MAX_PIXELS - ctx->xres * ctx->yres;
            windows_left = NUM_HW_WINDOWS - 1;
            rects.push_back(fb_rect);
        } else {
            pixels_left = MAX_PIXELS;
            windows_left = NUM_HW_WINDOWS;
        }

        changed = false;
        ctx->gsc_layer = 0; //taken from odroid
        ctx->current_gsc_index = 0; //taken from odroid

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];

            if ((layer.flags & HWC_SKIP_LAYER) || layer.compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;

            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            // we've already accounted for the framebuffer above
            if (layer.compositionType == HWC_FRAMEBUFFER)
                continue;

            // only layer 0 can be HWC_BACKGROUND, so we can
            // unconditionally allow it without extra checks
            if (layer.compositionType == HWC_BACKGROUND) {
                windows_left--;
                continue;
            }

            size_t pixels_needed = WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
            bool can_compose = windows_left && pixels_needed <= pixels_left;

            // TODO: when we add fimc as gscaler, we should collect here which layers do require
            // gsc and we'll need to properly assign them to fimg/fimc depending on layer/handle
            // and hw characteristics
            bool gsc_required = requires_gscaler(layer, handle->format);
            if (gsc_required)
                can_compose = can_compose && !gsc_used;

            // hwc_rect_t right and bottom values are normally exclusive;
            // the intersection logic is simpler if we make them inclusive
            hwc_rect_t visible_rect = layer.displayFrame;
            visible_rect.right--; visible_rect.bottom--;

            if (can_compose) { //this line is from odroid
                // no more than 2 layers can overlap on a given pixel
                for (size_t j = 0; can_compose && j < overlaps.size(); j++) {
                    if (intersect(visible_rect, overlaps.itemAt(j)))
                        can_compose = false;
                }

                // TODO - if (!can_compose)
                // ctx->mBypassSkipStaticLayer = true;
            }

            if (!can_compose) {
                layer.compositionType = HWC_FRAMEBUFFER;
                if (!ctx->fb_needed) {
                    ctx->first_fb = ctx->last_fb = i;
                    ctx->fb_needed = true;
                } else {
                    ctx->first_fb = min(i, ctx->first_fb);
                    ctx->last_fb = max(i, ctx->last_fb);
                }
                changed = true;
                ctx->first_fb = min(ctx->first_fb, (size_t)NUM_HW_WINDOWS-1); // odroid has this, check it
                break;
            }

            for (size_t j = 0; j < rects.size(); j++) {
                const hwc_rect_t &other_rect = rects.itemAt(j);
                if (intersect(visible_rect, other_rect))
                    overlaps.push_back(intersection(visible_rect, other_rect));
            }

            rects.push_back(visible_rect);
            pixels_left -= pixels_needed;
            //these lines are from odroid, there they have an array with gsc characteristics,
            //maybe we need something similar when using both fimg/fimc
            //win_idx++;
            //win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
            //win_idx = min(win_idx, NUM_HW_WINDOWS - 1);
            windows_left--;
            if (gsc_required) {
                gsc_used = true;
                ctx->gsc_layer++;
            }
        }

        if (changed)
            for (size_t i = ctx->first_fb; i < ctx->last_fb; i++)
                contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
    } while(changed);
}

void assignWindows(hwc_context_t *ctx, hwc_display_contents_1_t *contents)
{
    unsigned int nextWindow = 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (ctx->fb_needed && i == ctx->first_fb) {
            ALOGD("assigning framebuffer to window %u\n", nextWindow);
            ctx->fb_window = nextWindow;
            nextWindow++;
            continue;
        }

        if (layer.compositionType != HWC_FRAMEBUFFER &&
                layer.compositionType != HWC_FRAMEBUFFER_TARGET) {

            //ALOGD("assigning layer %u to window %u", i, nextWindow);
            ctx->win[nextWindow].layer_index = i;

            if (layer.compositionType == HWC_OVERLAY) {
                private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

                layer.hints = HWC_HINT_CLEAR_FB;

                if (requires_gscaler(layer, handle->format)) {
                    //ALOGD("\tusing FIMG");
                    //TODO - this could be the contents of assignGscLayer(layer, i, nextWindow) from odroid

                    ctx->win[nextWindow].gsc.mode = gsc_map_t::FIMG;
                    ctx->current_gsc_index++;

                    //Should ION memory for FIMG operation be re-/allocated?
                    if ((WIDTH(layer.displayFrame) != ctx->win[nextWindow].rect_info.w) ||
                        (HEIGHT(layer.displayFrame) != ctx->win[nextWindow].rect_info.h)) {

                        //ALOGD("%s: window(%d) need to reallocate ION buffers", __FUNCTION__, nextWindow);
                        window_buffer_deallocate(ctx, &ctx->win[nextWindow]);

                        ctx->win[nextWindow].rect_info.x = layer.displayFrame.left;
                        ctx->win[nextWindow].rect_info.y = layer.displayFrame.top;
                        ctx->win[nextWindow].rect_info.w = WIDTH(layer.displayFrame);
                        ctx->win[nextWindow].rect_info.w = HEIGHT(layer.displayFrame);

                        window_buffer_allocate(ctx, &ctx->win[nextWindow]);
                    } else {
                        if (!ctx->win[nextWindow].secion_param[i].size) {
                            //ALOGD("%s: window(%d) needs to allocate ION buffers", __FUNCTION__, nextWindow);
                            window_buffer_allocate(ctx, &ctx->win[nextWindow]);
                        } else {
                            //ALOGD("%s: window(%d) (%d,%d) doen't need to reallocate ION buffers", __FUNCTION__, nextWindow, ctx->win[nextWindow].rect_info.w, ctx->win[nextWindow].rect_info.h);
                        }
                    }
                }
            }
            nextWindow++;
        }
    }
}

static int prepare_fimd(hwc_context_t *ctx, hwc_display_contents_1_t* contents)
{
    // spammy
    // ALOGD("%s", __FUNCTION__);

    //ALOGD("preparing %u layers for FIMD", contents->numHwLayers);

    //if geometry has not changed, there is no need to do any work here
    if (!contents || (!(contents->flags & HWC_GEOMETRY_CHANGED)))
        return 0;

    // TODO: add skip static layers logic (odroid sources look good)

    determineSupportedOverlays(ctx, contents);
    determineBandwidthSupport(ctx, contents);
    assignWindows(ctx, contents);

    //if (!gsc_used)
    //    exynos5_cleanup_gsc_m2m(ctx);

    if (!ctx->fb_needed)
        ctx->fb_window = NO_FB_NEEDED;

    return 0;
}

static int hwc_prepare(hwc_composer_device_1_t *dev, size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];

    if (fimd_contents) {
        int err = prepare_fimd(ctx, fimd_contents);
        if (err)
            return err;
    }

    return 0;
}

static int perform_fimg(hwc_context_t *ctx, const hwc_layer_1_t &layer, struct hwc_win_info_t &win, int dst_format)
{
    //ALOGD("%s configuring FIMG", __FUNCTION__);

    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    //buffer_handle_t dst_buf;
    //private_handle_t *dst_handle;
    int ret = 0;

    int retry = 0;
    enum color_format g2d_format = CF_XRGB_8888;
    enum pixel_order pixel_order = AX_RGB;
    unsigned int src_bpp = 0, dst_bpp = 0;
    struct fimg2d_image g2d_src_img, g2d_dst_img, g2d_temp_img;
    struct fimg2d_blit fimg_cmd; //--> now win has the fimg_command to cache them between calls
    enum rotation l_rotate;

    memset(&fimg_cmd, 0, sizeof(struct fimg2d_blit));
    memset(&g2d_src_img, 0, sizeof(struct fimg2d_image));
    memset(&g2d_dst_img, 0, sizeof(struct fimg2d_image));
    memset(&g2d_temp_img, 0, sizeof(struct fimg2d_image));

    fimg_cmd.op = BLIT_OP_SRC;
    fimg_cmd.seq_no = SEQ_NO_BLT_HWC_NOSEC;
    fimg_cmd.sync = BLIT_ASYNC;
    fimg_cmd.param.g_alpha = 255;

    if (is_scaled(layer)) {
        fimg_cmd.param.scaling.mode = SCALING_BILINEAR;
        fimg_cmd.param.scaling.src_w = WIDTH(layer.sourceCrop);
        fimg_cmd.param.scaling.src_h = HEIGHT(layer.sourceCrop);
        fimg_cmd.param.scaling.dst_w = WIDTH(layer.displayFrame);
        fimg_cmd.param.scaling.dst_h = HEIGHT(layer.displayFrame);
    }

    //TODO: handle src_handle->paddr case
    if (src_handle->base) {
        formatValueHAL2G2D(src_handle->format, &g2d_format, &pixel_order, &src_bpp);
        //ALOGD("%s: src fmt(%d) g2d_format(%d) pixel_order(%d) src_bpp(%d)", __FUNCTION__, src_handle->format, g2d_format, pixel_order, src_bpp);

        g2d_src_img.addr.type = ADDR_USER;
        g2d_src_img.addr.start = src_handle->base;

        if (src_bpp == 1) {
            g2d_src_img.plane2.type = ADDR_USER;
            g2d_src_img.plane2.start = src_handle->base + src_handle->uoffset;
        }

        g2d_src_img.width = src_handle->width; //WIDTH(layer.sourceCrop);
        g2d_src_img.height = src_handle->height; //HEIGHT(layer.sourceCrop);
        g2d_src_img.stride = g2d_src_img.width * src_bpp;
        //ALOGD("%s src w(%d) h(%d) stride(%d)", __FUNCTION__, g2d_src_img.width, g2d_src_img.height, g2d_src_img.stride);
        g2d_src_img.order = (enum pixel_order) pixel_order;
        g2d_src_img.fmt = (enum color_format) g2d_format;
        g2d_src_img.rect.x1 = layer.sourceCrop.left;
        g2d_src_img.rect.y1 = layer.sourceCrop.top;
        g2d_src_img.rect.x2 = layer.sourceCrop.right;
        g2d_src_img.rect.y2 = layer.sourceCrop.bottom;
        g2d_src_img.need_cacheopr = false;

        fimg_cmd.src = &g2d_src_img;
    }

    struct fimg2d_rect dst_rect;
    //struct private_handle_t *dst_handle = private_handle_t::dynamicCast(win.dst_buf[win.current_buf]);
    uint32_t dst_addr = (uint32_t) win.secion_param[win.current_buf].memory; //win.addr[win.current_buf];

    if (dst_addr) {
        formatValueHAL2G2D(dst_format, &g2d_format, &pixel_order, &dst_bpp);
        //ALOGD("%s: dst fmt(%d) g2d_format(%d) pixel_order(%d) src_bpp(%d) dst_addr(0x%x)", __FUNCTION__, dst_format, g2d_format, pixel_order, dst_bpp, dst_addr);

        g2d_dst_img.addr.type = ADDR_USER;
        g2d_dst_img.addr.start = dst_addr;

        g2d_dst_img.width = WIDTH(layer.displayFrame); //dst_handle->f_w;
        g2d_dst_img.height = HEIGHT(layer.displayFrame); //dst_handle->f_h;

        if (dst_bpp == 1) {
            g2d_dst_img.plane2.type = ADDR_USER;
            g2d_dst_img.plane2.start = dst_addr + (EXYNOS4_ALIGN(g2d_dst_img.width, 16) * EXYNOS4_ALIGN(g2d_dst_img.height, 16));
        }

        g2d_dst_img.stride = g2d_dst_img.width * dst_bpp;
        //ALOGD("%s dst stride(%d)", __FUNCTION__, g2d_dst_img.stride);
        g2d_dst_img.order = (enum pixel_order) pixel_order;
        g2d_dst_img.fmt = (enum color_format) g2d_format;
        g2d_dst_img.rect.x1 = layer.displayFrame.left;
        g2d_dst_img.rect.y1 = layer.displayFrame.top;
        g2d_dst_img.rect.x2 = layer.displayFrame.right;
        g2d_dst_img.rect.y2 = layer.displayFrame.bottom;
        g2d_dst_img.need_cacheopr = false;

        fimg_cmd.dst = &g2d_dst_img;
    }

    // before fimgapistretch
    if (layer.acquireFenceFd >= 0) {
        sync_wait(layer.acquireFenceFd, 1000);

        //usleep(3000);
    }

    int cnt = 0;
    ret = stretchFimgApi(&fimg_cmd);
    while (ret < 0) {
        cnt++;
        ret = stretchFimgApi(&fimg_cmd);
        usleep(10);
        if (cnt == 20000)
            ALOGE("%s: stretch failed", __func__);
    }

    // remember layer.handle
    /* Here is the reason: https://android.googlesource.com/platform/hardware/libhardware/+/android-6.0.1_r22/include/hardware/hwcomposer.h
       If the layer's handle is unchanged across two consecutive
     * prepare calls and the HWC_GEOMETRY_CHANGED flag is not set
     * for the second call then the HWComposer implementation may
     * assume that the contents of the buffer have not changed. */

    if (layer.acquireFenceFd >= 0) {
        close(layer.acquireFenceFd);
    }

    return ret;

/*err_alloc:
    if (layer.acquireFenceFd >= 0)
        close(layer.acquireFenceFd);

    for (size_t i = 0; i < NUM_OF_WIN_BUF; i++) {
       if (win.dst_buf[i]) {
           ctx->alloc_device->free(ctx->alloc_device, win.dst_buf[i]);
           win.dst_buf[i] = NULL;
       }

       if (win.dst_buf_fence[i] >= 0) {
           close(win.dst_buf_fence[i]);
           win.dst_buf_fence[i] = -1;
       }
    }

    return ret;*/
}

static void config_overlay(hwc_context_t *ctx, hwc_layer_1_t *layer, s3c_fb_win_config &cfg)
{
    private_handle_t* hnd = (private_handle_t*) layer->handle;

    if (layer->compositionType == HWC_BACKGROUND) {
        hwc_color_t color = layer->backgroundColor;
        cfg.state = cfg.S3C_FB_WIN_STATE_COLOR;
        cfg.color = (color.r << 16) | (color.g << 8) | color.b;
        cfg.x = 0;
        cfg.y = 0;
        cfg.w = ctx->xres;
        cfg.h = ctx->yres;
    } else if (layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
        //ALOGD("%s HWC_FRAMEBUFFER_TARGET", __FUNCTION__);
        config_handle(ctx, hnd, layer->sourceCrop, layer->displayFrame, layer->blending, layer->acquireFenceFd, cfg);
        cfg.format = S3C_FB_PIXEL_FORMAT_BGRA_8888;

        cfg.phys_addr = hnd->paddr;

        if (layer->acquireFenceFd >= 0) {
            if (sync_wait(layer->acquireFenceFd, 1000) < 0)
                ALOGW("sync_wait error");

            //usleep(3000);
            close(layer->acquireFenceFd);
            layer->acquireFenceFd = -1;

            cfg.fence_fd = -1; //??
        }
    }
}

static int post_fimd(hwc_context_t *ctx, hwc_display_contents_1_t* contents)
{
    private_handle_t* hnd = NULL;
    size_t i;
    unsigned int window;
    struct s3c_fb_win_config_data win_data;
    struct s3c_fb_win_config *config = win_data.config;

//    ALOGD("%s fb_window=%d hwc_layer=%d",__FUNCTION__, ctx->fb_window, ctx->hwc_layer);

//    if ((ctx->fb_window != NO_FB_NEEDED) || (ctx->hwc_layer > 0)) {

    memset(config, 0, sizeof(win_data.config));

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        config[i].fence_fd = -1;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        struct hwc_win_info_t &win = ctx->win[i];
        int layer_idx = win.layer_index; //ctx->win[i].layer_index;

        //ALOGD("i=%d layer_idx=%d", i, layer_idx);

        if (i == 3) //TODO: check if we can remove this
            window = 4;
        else
            window = i;

        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            private_handle_t* hnd = private_handle_t::dynamicCast(layer.handle);

            //ALOGD("%s i=%d hnd.usage=0x%x hnd.paddr=0x%x hnd->format=0x%x", __FUNCTION__,
            //        i, hnd->usage, hnd->paddr, hnd->format);

            if (win.gsc.mode == gsc_map_t::FIMG) {
                int err = perform_fimg(ctx, layer, win, HAL_PIXEL_FORMAT_BGRA_8888);
                if (err < 0) {
                    ALOGE("failed to perform FIMG for layer %u", i);
                    win.gsc.mode = gsc_map_t::NONE;
                    continue;
                }

               layer.acquireFenceFd = -1;

                if (ctx->win[window].secion_param[win.current_buf].memory) {
                    hwc_rect_t sourceCrop = { 0, 0, WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame) };

                    //use geometry of layer.handle and overwrite the rest
                    private_handle_t *dst_handle = private_handle_t::dynamicCast(layer.handle);
                    config_handle(ctx, dst_handle, sourceCrop, layer.displayFrame, layer.blending, layer.acquireFenceFd, config[window]);

                    config[window].format = S3C_FB_PIXEL_FORMAT_BGRA_8888;
                    config[window].phys_addr = ctx->win[window].secion_param[win.current_buf].physaddr; //dst_handle->paddr;
                    config[window].offset = 0;
                    config[window].stride = EXYNOS4_ALIGN(config[window].w,16) * 4;

                    //ALOGD("%s: window(%x) FIMG hndpaddr(0x%x) hndbase(0x%x)", __FUNCTION__, window, dst_handle->paddr, dst_handle->base);
                }
            } else {
                config_overlay(ctx, &layer, config[window]);
            }

            if (window == 0 && config[window].blending != S3C_FB_BLENDING_NONE) {
                //ALOGD("blending not supported on window 0; forcing BLENDING_NONE");
                config[window].blending = S3C_FB_BLENDING_NONE;
            }

            if ( (WIDTH(layer.sourceCrop) <= 15) ||
                 (HEIGHT(layer.sourceCrop) <= 7) ||
                 (WIDTH(layer.displayFrame) <= 31) ||
                 (HEIGHT(layer.displayFrame) <= 31) ) {
                config[window].state = config->S3C_FB_WIN_STATE_DISABLED;
            }
        }
    }

//            switch (ctx->win[window].hw_device.mode) {
//            case hw_device_map_t::FIMG:

/*                reconfigure = gsc_src_cfg_changed(src_cfg, gsc_data->src_cfg) ||
                              gsc_dst_cfg_changed(dst_cfg, gsc_data->dst_cfg);
                if (reconfigure) {
                    int w = ALIGN(dst_cfg.w, GSC_DST_W_ALIGNMENT_RGB888);
                    int h = ALIGN(dst_cfg.h, GSC_DST_H_ALIGNMENT_RGB888);
        for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
            if (gsc_data->dst_buf[i]) {
                alloc_device->free(alloc_device, gsc_data->dst_buf[i]);
                gsc_data->dst_buf[i] = NULL;
            }
            if (gsc_data->dst_buf_fence[i] >= 0) {
                close(gsc_data->dst_buf_fence[i]);
                gsc_data->dst_buf_fence[i] = -1;
            }
            int ret = alloc_device->alloc(alloc_device, w, h,
                    HAL_PIXEL_FORMAT_RGBX_8888, usage, &gsc_data->dst_buf[i],
                    &dst_stride);
            if (ret < 0) {
                ALOGE("failed to allocate destination buffer: %s",
                        strerror(-ret));
                goto err_alloc;
            }
        }
        gsc_data->current_buf = 0;
    }

                err = perform_fimg(layer, ctx, dst_format);
                if (err < 0) {
                    ALOGE("failed to perform FIMG for layer %u", i);
                    ctx->win[window].hw_device.mode = hw_device_map_t::NONE;
                    continue;
                }*/

/*buffer_handle_t dst_buf = gsc.dst_buf[gsc.current_buf];
                private_handle_t *dst_handle =
                        private_handle_t::dynamicCast(dst_buf);

hwc_rect_t sourceCrop = { 0, 0,
                        WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame) };

                int fence = gsc.dst_cfg.releaseFenceFd;
                exynos4_config_handle(hnd, sourceCrop,
                        layer.displayFrame, layer.blending, fence, config[window],
                        ctx);
*/

//            default:
//                config_overlay(ctx, contents->hwLayers[layer_idx], config[window]);
//            }

/*            if (layer.compositionType == HWC_BACKGROUND) {
                hwc_color_t color = layer.backgroundColor;
                config[window].state = config[window].S3C_FB_WIN_STATE_COLOR;
                config[window].color = (color.r << 16) | (color.g << 8) | color.b;
                config[window].x = 0;
                config[window].y = 0;
                config[window].w = ctx->xres;
                config[window].h = ctx->yres;
            } else if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                ALOGD("%s HWC_FRAMEBUFFER_TARGET", __FUNCTION__);
                private_handle_t::validate(hnd);

                exynos4_config_handle(hnd, layer.sourceCrop, layer.displayFrame, layer.blending, layer.acquireFenceFd, config[window], ctx);
                //config[window].format = S3C_FB_PIXEL_FORMAT_BGRA_8888;
            } else {
                ALOGD("%s layer.compositionType=%d", __FUNCTION__, layer.compositionType);
                //if (ctx->win[window].buf_index != -1)
                //{
                //    if (ctx->win[window].secion_param[ctx->win[window].buf_index].memory)
                //    {

                if (layer.acquireFenceFd >= 0) {
                    if ( sync_wait(layer.acquireFenceFd, 1000) < 0 )
                        ALOGW("sync_wait error");

                    usleep(3000);
                    close(layer.acquireFenceFd);
                    layer.acquireFenceFd = -1;
                    config[window].fence_fd = -1;
                }

                        //if (private_handle_t::validate(hnd)) {
                        //    ALOGD("handle is valid");
                            exynos4_config_handle(hnd, layer.sourceCrop, layer.displayFrame, layer.blending, layer.acquireFenceFd, config[window], ctx);
                            //config[window].format = S3C_FB_PIXEL_FORMAT_BGRA_8888;

                            if (hnd->flags & GRALLOC_USAGE_HW_TEXTURE)
                                config[window].phys_addr = hnd->base;
                            else
                                config[window].phys_addr = hnd->paddr;

                            ALOGD("%s base=0x%x paddr=0x%x", __FUNCTION__);
                            //config[window].phys_addr = ctx->win[window].secion_param[ctx->win[window].buf_index].physaddr;
                            //config[window].offset = 0;
                            //config[window].stride = EXYNOS4_ALIGN(config[window].w, 16) * 4;
                        //} else
                        //    ALOGD("handle is not valid");
                //    }
                //}
            }

            //ALOGD("%s layer.compositionType=%d before config[%d].state=%d", __FUNCTION__, layer.compositionType, window, config[window].state);
            if ( (WIDTH(layer.sourceCrop) <= 15) ||
                 (HEIGHT(layer.sourceCrop) <= 7) ||
                 (WIDTH(layer.displayFrame) <= 31) ||
                 (HEIGHT(layer.displayFrame) <= 31) ) {
                config[window].state = config->S3C_FB_WIN_STATE_DISABLED;
            }
            //ALOGD("%s layer.compositionType=%d config[%d].state=%d", __FUNCTION__, layer.compositionType, window, config[window].state);

            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET)
                config[window].phys_addr = hnd->paddr;

            // TODO wfd and hdmi cable status
            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                if (layer.acquireFenceFd >= 0) {
                    if ( sync_wait(layer.acquireFenceFd, 1000) < 0 )
                        ALOGW("sync_wait error");

                    usleep(3000);
                    close(layer.acquireFenceFd);
                    layer.acquireFenceFd = -1;
                    config[window].fence_fd = -1;
                }

                // SecHdmiClient::blit2Hdmi() comes here

                /*if (config[window].blending != S3C_FB_BLENDING_NONE)
                    config[window].blending = S3C_FB_BLENDING_NONE;*/
    /*        }
        }

        if (window == 0 && config[window].blending != S3C_FB_BLENDING_NONE) {
            //ALOGD("blending not supported on window 0; forcing BLENDING_NONE");
            config[window].blending = S3C_FB_BLENDING_NONE;
        }
    }*/

    //dump_fb_win_cfg(win_data);

    int wincfg_err = ioctl(ctx->win_fb0.fd, S3CFB_WIN_CONFIG, &win_data);

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);
    }

    if (wincfg_err < 0) {
        ALOGD("%s S3CFB_WIN_CONFIG failed: %s", __FUNCTION__, strerror(errno));
    } else {
        memcpy(ctx->last_config, &win_data.config, sizeof(win_data.config));
        ctx->last_fb_window = ctx->fb_window;

        for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
            int layer_idx = ctx->win[i].layer_index;

            if (i == 3)
                window = 4;
            else
                window = i;

            if (layer_idx != -1) {
                hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
                ctx->last_handles[window] = layer.handle;
            }
        }
    }

    return win_data.fence;
}

static int set_fimd(hwc_context_t *ctx, hwc_display_contents_1_t* contents)
{
    // spammy
    //ALOGD("%s", __FUNCTION__);

    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (ctx->fb_window != NO_FB_NEEDED) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];

            //dump_layer(&displays[0]->hwLayers[i], __FUNCTION__);
            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                //ALOGD("HWC_FRAMEBUFFER_TARGET found");
                ctx->win[ctx->fb_window].layer_index = i;
                fb_layer = &contents->hwLayers[i]; //not in blob, copied from exynos5 hwc
                break;
            }
        }

        if (UNLIKELY(!fb_layer)) {
            ALOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        }
    }

    int fence;
    if (!err) {
        fence = post_fimd(ctx, contents);
        if (fence < 0)
            err = fence;
    }

    if (err) {
        ALOGD("%s error in post_fimd(), about to clear window", __FUNCTION__);
        fence = window_clear(ctx);
    }

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        int layer_idx = ctx->win[i].layer_index;

        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            int dup_fd = dup_or_warn(fence);

            if (ctx->win[i].gsc.mode == gsc_map_t::FIMG) {
                if (ctx->win[i].dst_buf_fence[ctx->win[i].current_buf] >= 0)
                    close(ctx->win[i].dst_buf_fence[ctx->win[i].current_buf]);

                ctx->win[i].dst_buf_fence[ctx->win[i].current_buf] = dup_fd;
                ctx->win[i].current_buf = (ctx->win[i].current_buf + 1) % NUM_OF_WIN_BUF;

                layer.releaseFenceFd = -1;
            } else {
                layer.releaseFenceFd = dup_fd;
            }

/*            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                layer.releaseFenceFd = dup_fd;
            } else {
                //layer.releaseFenceFd = dup_fd; //provisory, check below code

                if (ctx->win[i].buf_index > 2) {
                    ALOGW("%s window %d invalid buf_index %d", __FUNCTION__, i, ctx->win[i].buf_index);
                } else {
                    if (ctx->win[i].fence[ctx->win[i].buf_index] >= 0)
                        close(ctx->win[i].fence[ctx->win[i].buf_index]);

                    ctx->win[i].fence[ctx->win[i].buf_index] = dup_fd;
                    ctx->win[i].buf_index = (ctx->win[i].buf_index + 1) % NUM_OF_WIN_BUF;
                }

                layer.releaseFenceFd = -1;
            }*/

        }
    }

    contents->retireFenceFd = fence;

    return err;
}

static int hwc_set(hwc_composer_device_1_t *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
    int fimd_err = 0;

    if (fimd_contents)
        fimd_err = set_fimd(ctx, fimd_contents);

    if (fimd_err)
        return fimd_err;

    return 0;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int i = 0;

    //ALOGD("%s", __FUNCTION__);

    // Close V4L2 for FIMC
    v4l2_close(ctx);

    // Close FB0
    if (window_close(&ctx->win_fb0) < 0) {
        ALOGE("%s: Error closing FB0 window", __FUNCTION__);
    }

    // Close all windows
    for (i = 0; i < NUM_HW_WINDOWS; i++) {
        if (window_close(&ctx->win[i]) < 0) {
            ALOGE("%s: window_close(%d) failed", __FUNCTION__, i);
        }
    }

    if (ctx) {
        free(ctx);
    }
    return 0;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, __unused int dpy,
        int event, int enabled)
{
    int val = 0, rc = 0;
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        val = enabled;
        ALOGD("%s: HWC_EVENT_VSYNC, enabled=%d", __FUNCTION__, val);

        rc = ioctl(ctx->win_fb0.fd, S3CFB_SET_VSYNC_INT, &val);
        if (rc < 0) {
            ALOGE("%s: could not set vsync using ioctl: %s", __FUNCTION__,
                strerror(errno));
            return -errno;
        }
        return rc;
    }
    return -EINVAL;
}

static int hwc_blank(struct hwc_composer_device_1 *dev, int dpy, int blank)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int fence = 0;

    ALOGD("%s blank=%d", __FUNCTION__, blank);

    fence = window_clear(ctx);
    if (fence != -1)
        close(fence);

    if (ioctl(ctx->win_fb0.fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK) < 0) {
        ALOGD("%s Error %s in FBIOBLANK blank=%d", __FUNCTION__, strerror(errno), blank);
    }

    return 0;
}

static int hwc_query(struct hwc_composer_device_1* dev,
        int what, int* value)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    ALOGD("%s", __FUNCTION__);

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // stock blob do support background layer
        value[0] = 1;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = _VSYNC_PERIOD / ctx->gralloc_module->fps;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    ctx->procs = const_cast<hwc_procs_t *>(procs);
}

static int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
    uint32_t* configs, size_t* numConfigs)
{
    //ALOGD("%s", __FUNCTION__);

    if (*numConfigs == 0)
        return 0;

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    }

    return -EINVAL;
}

static int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
    __unused uint32_t config, const uint32_t* attributes, int32_t* values)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int i = 0;

    ALOGD("%s", __FUNCTION__);

    while(attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch(disp) {
        case 0:

            switch(attributes[i]) {
            case HWC_DISPLAY_VSYNC_PERIOD: /* The vsync period in nanoseconds */
                values[i] = ctx->vsync_period;
                break;

            case HWC_DISPLAY_WIDTH: /* The number of pixels in the horizontal and vertical directions. */
                values[i] = ctx->xres;
                break;

            case HWC_DISPLAY_HEIGHT:
                values[i] = ctx->yres;
                break;

            case HWC_DISPLAY_DPI_X:
                values[i] = ctx->xdpi;
                break;

            case HWC_DISPLAY_DPI_Y:
                values[i] = ctx->ydpi;
                break;

            // deprecated 
            //case HWC_DISPLAY_SECURE:
                /* Indicates if the display is secure
                 * For HDMI/WFD if the sink supports HDCP, it will be true
                 * Primary panel is always considered secure
                 */
            //    break;

            case HWC_DISPLAY_COLOR_TRANSFORM:
                values[i] = 0;
                break;

            default:
                ALOGE("%s:unknown display attribute %d", __FUNCTION__, attributes[i]);
                return -EINVAL;
            }
            break;

        case 1:
            // TODO: no hdmi at the moment
            break;

        default:
            ALOGE("%s:unknown display %d", __FUNCTION__, disp);
            return -EINVAL;
        }

        i++;
    }
    return 0;
}

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;
    struct hwc_context_t *dev = NULL;
    struct hwc_win_info_t *win = NULL;
    int refreshRate = 0;
    unsigned int i = 0, j = 0;

    //ALOGD("%s", __FUNCTION__);

    if (!strcmp(name, HWC_HARDWARE_COMPOSER)) {
        dev = (hwc_context_t*)malloc(sizeof(*dev));

        // initialize our state here
        memset(dev, 0, sizeof(*dev));

        // get gralloc module
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &dev->gralloc_module) != 0) {
            ALOGE("failed to get gralloc hw module");
            status = -EINVAL;
            goto err_get_module;
        }

        if (gralloc_open((const hw_module_t *)dev->gralloc_module, &dev->alloc_device)) {
            ALOGE("failed to open gralloc");
            status = -EINVAL;
            goto err_get_module;
        }

        // initialize the procs
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = HWC_DEVICE_API_VERSION_1_1;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = hwc_device_close;

        dev->device.prepare = hwc_prepare;
        dev->device.set = hwc_set;
        dev->device.eventControl = hwc_eventControl;
        dev->device.blank = hwc_blank;
        dev->device.query = hwc_query;
        dev->device.registerProcs = hwc_registerProcs;
        //dev->device.dump
        dev->device.getDisplayConfigs = hwc_getDisplayConfigs;
        dev->device.getDisplayAttributes = hwc_getDisplayAttributes;

        *device = &dev->device.common;


        // property to disable HWC taken from exynos5
        char value[PROPERTY_VALUE_MAX];
        property_get("debug.hwc.force_gpu", value, "0");
        dev->force_gpu = atoi(value);

        // Init Vsync
        init_vsync_thread(dev);

        // Initialize ION
        dev->ion_hdl.client = -1;
        dev->ion_hdl.param.client = -1;
        dev->ion_hdl.param.buffer = -1;
        dev->ion_hdl.param.size = 0;
        dev->ion_hdl.param.memory = NULL;
        dev->ion_hdl.param.physaddr = NULL;

        dev->ion_hdl.client = ion_client_create();


        /* open all windows */
        for (i = 0; i < NUM_HW_WINDOWS; i++) {
            if (window_open(&(dev->win[i]), i) < 0) {
                ALOGE("%s: Failed to open window %d device ", __FUNCTION__, i);
                status = -EINVAL;
                goto err;
            }
        }

        // query LCD info
        if (window_open(&dev->win_fb0, 2) < 0) {
            ALOGE("%s: Failed to open window %d device ", __FUNCTION__, 2);
            status = -EINVAL;
            goto err;
        }

        if (window_get_var_info(dev->win_fb0.fd, &dev->lcd_info) < 0) {
            ALOGE("%s: window_get_var_info is failed : %s",
                    __FUNCTION__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        memcpy(&dev->win_fb0.lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
        memcpy(&dev->win_fb0.var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

        refreshRate = 1000000000000LLU /
            (
                uint64_t( dev->lcd_info.upper_margin + dev->lcd_info.lower_margin + dev->lcd_info.yres)
                * ( dev->lcd_info.left_margin  + dev->lcd_info.right_margin + dev->lcd_info.xres)
                * dev->lcd_info.pixclock
            );

        if (refreshRate == 0) {
            ALOGD("%s: Invalid refresh rate, using 60 Hz", __FUNCTION__);
            refreshRate = 60;  /* 60 Hz */
        }

        dev->vsync_period = _VSYNC_PERIOD / refreshRate;

        dev->xres = dev->lcd_info.xres;
        dev->yres = dev->lcd_info.yres;
        dev->xdpi = (dev->lcd_info.xres * 25.4f * 1000.0f) / dev->lcd_info.width;
        dev->ydpi = (dev->lcd_info.yres * 25.4f * 1000.0f) / dev->lcd_info.height;

        ALOGI("using\nxres         = %d px\nyres         = %d px\nwidth        = %d mm (%f dpi)\nheight       = %d mm (%f dpi)\nrefresh rate = %d Hz",
                dev->xres, dev->yres, dev->lcd_info.width, dev->xdpi / 1000.0f, dev->lcd_info.height, dev->ydpi / 1000.0f, refreshRate);

        dev->win_fb0.rect_info.x = 0;
        dev->win_fb0.rect_info.y = 0;
        dev->win_fb0.rect_info.w = dev->lcd_info.xres;
        dev->win_fb0.rect_info.h = dev->lcd_info.yres;

        if (window_get_info(&dev->win_fb0) < 0) {
            ALOGE("%s: window_get_info is failed : %s", __FUNCTION__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        dev->win_fb0.size = dev->win_fb0.var_info.yres * dev->win_fb0.fix_info.line_length;
        dev->win_fb0.power_state = 1;

        dev->win_fb0.current_buf = 0;

        for(i = 0; i < NUM_OF_WIN_BUF; i++) {
            dev->win_fb0.addr[i] = dev->win_fb0.fix_info.smem_start + (dev->win_fb0.size * i);
            ALOGE("%s: win-%d addr[%d] = 0x%x\n", __FUNCTION__, 2, i, dev->win_fb0.addr[i]);
        }

        if (DEBUG) {
            dump_win(&dev->win_fb0);
        }


        // initialize the window context */
        for (i = 0; i < NUM_HW_WINDOWS; i++) {
            win = &dev->win[i];
            memcpy(&win->lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
            memcpy(&win->var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

            win->rect_info.x = 0;
            win->rect_info.y = 0;
            win->rect_info.w = win->var_info.xres;
            win->rect_info.h = win->var_info.yres;

            for (j = 0; j < NUM_OF_WIN_BUF; j++) {
                win->secion_param[j].buffer = -1;
                win->secion_param[j].size = 0;
                win->secion_param[j].client = dev->ion_hdl.client;
                win->secion_param[j].memory = NULL;
                win->secion_param[j].physaddr = NULL;

                win->dst_buf_fence[j] = -1;
            }
        }


        /* query fbdev1 info */
        if (window_open(&dev->win_fbdev1, 5) < 0) {
            ALOGE("%s:: Failed to open window %d device ", __func__, 5);
        } else {
            if (window_get_var_info(dev->win_fbdev1.fd, &dev->fbdev1_info) < 0) {
                ALOGE("%s window_get_var_info is failed : %s", __func__, strerror(errno));
            } else {
                memcpy(&dev->win_fbdev1.lcd_info, &dev->fbdev1_info, sizeof(struct fb_var_screeninfo));
                memcpy(&dev->win_fbdev1.var_info, &dev->fbdev1_info, sizeof(struct fb_var_screeninfo));

                dev->win_fbdev1.rect_info.x = 0;
                dev->win_fbdev1.rect_info.y = 0;
                dev->win_fbdev1.rect_info.w = dev->win_fbdev1.var_info.xres;
                dev->win_fbdev1.rect_info.h = dev->win_fbdev1.var_info.yres;

                //struct_1960.xres = 0;
                //struct_1960.yres = 0;
                //struct_1960.field_10 = 0;
                //struct_1960.field_1c = 0;

                //dev->str1980_yres = 0;

                if (window_get_info(&dev->win_fbdev1) < 0) {
                   ALOGE("%s::window_get_info is failed : %s", __func__, strerror(errno));
                } else {
                    if (dev->win_fbdev1.fix_info.smem_start != NULL) {
                        dev->win_fbdev1_needs_buffer = 0;
                        //struct_1980.field_19 = 0;

                        if (window_set_pos(&dev->win_fbdev1) < 0) {
                            ALOGE("%s::window_set_pos is failed : %s", __func__, strerror(errno));
                        } else {
                            dev->win_fbdev1.current_buf = 0;
                            dev->win_fbdev1.power_state = 0;

                            dev->win_fbdev1.size = dev->win_fbdev1.var_info.yres * dev->win_fbdev1.fix_info.line_length;
                            dev->win_fbdev1.addr[0] = dev->win_fbdev1.fix_info.smem_start;
                            dev->win_fbdev1.addr[1] = dev->win_fbdev1.fix_info.smem_start + dev->win_fbdev1.size;
                            dev->win_fbdev1.addr[2] = dev->win_fbdev1.fix_info.smem_start + (dev->win_fbdev1.size * 2);

                            for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
                                dev->win_fbdev1.dst_buf_fence[j] = -1;
                            }

                            dump_win(&dev->win_fbdev1);
                        }
                    } else {
            	        dev->win_fbdev1_needs_buffer = 1;
                        //struct_1980.field_19 = 0;

                        dev->win_fbdev1.current_buf = 0;
                        dev->win_fbdev1.power_state = 0;
                        dev->win_fbdev1.addr[0] = NULL;
                        dev->win_fbdev1.addr[1] = NULL;
                        dev->win_fbdev1.addr[2] = NULL;

                        for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
                            dev->win_fbdev1.secion_param[j].buffer = -1;
                            dev->win_fbdev1.secion_param[j].size = 0;
                            dev->win_fbdev1.secion_param[j].client = dev->ion_hdl.client;
                            dev->win_fbdev1.secion_param[j].memory = NULL;
                            dev->win_fbdev1.secion_param[j].physaddr = NULL;

                            dev->win_fbdev1.dst_buf_fence[j] = -1;
                        }
                    }
                }
            }
        }

        ALOGD("%s win_fbdev1_needs_buffer=%d", __FUNCTION__, dev->win_fbdev1_needs_buffer);

        // Initialize V4L2 for FIMC
        if (v4l2_open(dev) < 0)
            ALOGE("%s Failed to initialize FIMC", __FUNCTION__);

        /* doesn't fix fimg
        //Init fimg
        struct fimg2d_image fimg_tmp_img;
        struct fimg2d_blit fimg_cmd;

        dev->fimg_tmp_ion_hdl.client = dev->ion_hdl.client;
        dev->fimg_tmp_ion_hdl.param.client = -1;
        dev->fimg_tmp_ion_hdl.param.buffer = -1;
        dev->fimg_tmp_ion_hdl.param.size = 0;
        dev->fimg_tmp_ion_hdl.param.memory = NULL;
        dev->fimg_tmp_ion_hdl.param.physaddr = NULL;

        createIONMem(&dev->fimg_tmp_ion_hdl.param, 12*1024, ION_HEAP_EXYNOS_CONTIG_MASK | 0x800000); // Bit 23 is also active, but don't know the meaning

        //ALOGE("%s fimg_tmp.param.memory(0x%x)", __FUNCTION__, dev->fimg_tmp_ion_hdl.param.memory);
        memset(dev->fimg_tmp_ion_hdl.param.memory, 0, 12*1024);

        fimg_tmp_img.addr.start = (unsigned long) dev->fimg_tmp_ion_hdl.param.memory + 8192;
        fimg_cmd.tmp = &fimg_tmp_img;
        fimg_cmd.seq_no = SEQ_NO_CMD_SET_DBUFFER;

        if (stretchFimgApi(&fimg_cmd) < 0) {
           ALOGE("%s: stretchFimgApi(SEQ_NO_CMD_SET_DBUFFER)", __func__);
           return -1;
        }*/

        status = 0;
    }
    return status;

err:
    if (window_close(&dev->win_fb0) < 0) {
        ALOGE("%s: Error closing FB0 window", __FUNCTION__);
    }

    for (i = 0; i < NUM_HW_WINDOWS; i++) {
        if (window_close(&dev->win[i]) < 0) {
            ALOGE("%s: window_close(%d) failed", __FUNCTION__, i);
        }
    }

err_get_module:
    if (dev) {
        free(dev);
    }

    return status;
}
