/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012 The CyanogenMod Project <http://www.cyanogenmod.org>
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

/*
 *
 * @author Rama, Meka(v.meka@samsung.com)
           Sangwoo, Park(sw5771.park@samsung.com)
           Jamie Oh (jung-min.oh@samsung.com)
 * @date   2011-03-11
 *
 */

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <EGL/egl.h>
#include <fcntl.h>
#include <hardware_legacy/uevent.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#include "SecHWCUtils.h"

#include "gralloc_priv.h"
#ifdef HWC_HWOVERLAY
#include <GLES/gl.h>
#endif
#if defined(BOARD_USES_HDMI)
#include "SecHdmiClient.h"
#include "SecTVOutService.h"

#include "SecHdmi.h"

//#define CHECK_EGL_FPS
#ifdef CHECK_EGL_FPS
extern void check_fps();
#endif

static int lcd_width, lcd_height;
static int prev_usage = 0;

#define CHECK_TIME_DEBUG 0
#define SUPPORT_AUTO_UI_ROTATE
#endif
int testRenderNum =0;

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 2,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Samsung S5PC21X hwcomposer module",
        author: "SAMSUNG",
        methods: &hwc_module_methods,
    }
};

/*****************************************************************************/

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }

static void dump_layer(hwc_layer_1_t const* l) {
    SEC_HWC_Log(HWC_LOG_DEBUG,"\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
            "{%d,%d,%d,%d}, {%d,%d,%d,%d}",
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

void calculate_rect(hwc_layer_1_t *cur, struct hwc_context_t *ctx, sec_rect *rect)
{
    rect->x = cur->displayFrame.left;
    rect->y = cur->displayFrame.top;
    rect->w = cur->displayFrame.right - cur->displayFrame.left;
    rect->h = cur->displayFrame.bottom - cur->displayFrame.top;

    if (rect->x < 0) {
        if ((unsigned int) (rect->w + rect->x) > ctx->lcd_info.xres)
            rect->w = ctx->lcd_info.xres;
        else
            rect->w = rect->w + rect->x;
        rect->x = 0;
    } else {
        if ((unsigned int) (rect->w + rect->x) > ctx->lcd_info.xres)
            rect->w = ctx->lcd_info.xres - rect->x;
    }
    if (rect->y < 0) {
        if ((unsigned int) (rect->h + rect->y) > ctx->lcd_info.yres)
            rect->h = ctx->lcd_info.yres;
        else
            rect->h = rect->h + rect->y;
        rect->y = 0;
    } else {
        if ((unsigned int) (rect->h + rect->y) > ctx->lcd_info.yres)
            rect->h = ctx->lcd_info.yres - rect->y;
    }
}

int set_src_dst_img_rect(hwc_layer_1_t *cur,
		struct hwc_context_t *ctx,
        struct hwc_win_info_t *win,
        struct sec_img *src_img,
        struct sec_img *dst_img,
        struct sec_rect *src_rect,
        struct sec_rect *dst_rect,
        int win_idx)
{
    private_handle_t *prev_handle = (private_handle_t *)(cur->handle);
    sec_rect rect;

    /* 1. Set src_img from prev_handle */
    src_img->f_w     = prev_handle->stride;
    src_img->f_h     = prev_handle->height;
    src_img->w       = prev_handle->width;
    src_img->h       = prev_handle->height;
    src_img->format  = prev_handle->format;
    src_img->base    = (uint32_t)prev_handle->base;
    src_img->offset  = prev_handle->offset;
    src_img->mem_id  = prev_handle->fd;
    src_img->paddr  = prev_handle->paddr;
    src_img->usage  = prev_handle->usage;
    src_img->uoffset  = prev_handle->uoffset;
    src_img->voffset  = prev_handle->voffset;

    src_img->mem_type = HWC_VIRT_MEM_TYPE;

    switch (src_img->format) {
    case HAL_PIXEL_FORMAT_YV12:  //0x32315659           /* To support video editor */
    case HAL_PIXEL_FORMAT_YCbCr_420_P: //0x101     /* To support SW codec     */
    // this is not in stock blob -> case HAL_PIXEL_FORMAT_YCrCb_420_SP: //0x11
    case HAL_PIXEL_FORMAT_YCbCr_420_SP: //0x105
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP: //0x110
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP: //0x111
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED: //0x112
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP: //0x113
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP: //0x114
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:  //0x115
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I: //0x116
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I: //0x117
    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I: //0x118
        src_img->f_w = EXYNOS4_ALIGN(src_img->f_w, 16);
        src_img->f_h = EXYNOS4_ALIGN(src_img->f_h, 2);
        break;
    default:
        break;
    }

    /* 2. Set dst_img from window(lcd) */
    calculate_rect(cur, ctx, &rect);
    dst_img->f_w = EXYNOS4_ALIGN(rect.w, 16);
    dst_img->f_h = rect.h;
    dst_img->w = EXYNOS4_ALIGN(rect.w, 16);
    dst_img->h = rect.h;

    switch (win->lcd_info.bits_per_pixel) {
    case 32:
        dst_img->format = HAL_PIXEL_FORMAT_BGRA_8888;
        break;
    default:
        dst_img->format = HAL_PIXEL_FORMAT_RGB_565;
        break;
    }

    dst_img->base     = win->addr[win->buf_index];
    dst_img->offset   = 0;
    dst_img->mem_id   = 0;
    dst_img->mem_type = HWC_PHYS_MEM_TYPE;

    /* 3. Set src_rect(crop rect) */
    switch(cur->transform) {
    case 0:
    case HWC_TRANSFORM_FLIP_H:
    case HWC_TRANSFORM_FLIP_V:
    case HWC_TRANSFORM_ROT_180:

        if (cur->displayFrame.left < 0) {
            src_rect->x = (0 - cur->displayFrame.left) * src_img->w / (cur->displayFrame.right - cur->displayFrame.left);

            if ((unsigned int) cur->displayFrame.right > ctx->lcd_info.xres) {
                src_rect->w =
                    (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x -
                    ( (cur->displayFrame.right - ctx->lcd_info.xres) * (src_img->w)
                      / (cur->displayFrame.right - cur->displayFrame.left) );
            } else {
                src_rect->w = (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x;
            }
        } else {
            src_rect->x = cur->sourceCrop.left;

            if ((unsigned int) cur->displayFrame.right > ctx->lcd_info.xres) {
                src_rect->w =
                    (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x -
                    ( (cur->displayFrame.right - ctx->lcd_info.xres) * (src_img->w)
                      / (cur->displayFrame.right - cur->displayFrame.left) );
            } else {
                src_rect->w = (cur->sourceCrop.right - cur->sourceCrop.left);
            }
    	}

        if (cur->displayFrame.top < 0) {
            src_rect->y = (0 - cur->displayFrame.top) * src_img->h / (cur->displayFrame.bottom - cur->displayFrame.top);

            if ((unsigned int) cur->displayFrame.bottom > ctx->lcd_info.yres) {
                src_rect->h =
                    (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y -
                    ( (cur->displayFrame.bottom - ctx->lcd_info.yres) * (src_img->h)
                      / (cur->displayFrame.bottom - cur->displayFrame.top) );
            } else {
                src_rect->h = (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y;
            }
        } else {
            src_rect->y = cur->sourceCrop.top;

            if ((unsigned int) cur->displayFrame.bottom > ctx->lcd_info.yres) {
                src_rect->h =
                    (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y -
                    ( (cur->displayFrame.bottom - ctx->lcd_info.yres) * (src_img->h)
                      / (cur->displayFrame.bottom - cur->displayFrame.top) );
            } else {
                src_rect->h =
                    (cur->sourceCrop.bottom - cur->sourceCrop.top);
            }
        }
        break;


    case HWC_TRANSFORM_ROT_90:
    case HWC_TRANSFORM_FLIP_H | HWC_TRANSFORM_ROT_90:
    case HWC_TRANSFORM_FLIP_V | HWC_TRANSFORM_ROT_90:
    case HAL_TRANSFORM_ROT_270:

        if (cur->displayFrame.left < 0) {
            src_rect->y = (0 - cur->displayFrame.left) * src_img->h / (cur->displayFrame.right - cur->displayFrame.left);

            if ((unsigned int) cur->displayFrame.right > ctx->lcd_info.xres) {
               src_rect->h =
                   (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y -
                   ( (cur->displayFrame.right - ctx->lcd_info.xres) * (src_img->h)
                     / (cur->displayFrame.right - cur->displayFrame.left) );
            } else {
                src_rect->h = (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y;
            }
        } else {
            src_rect->y = cur->sourceCrop.top;

            if ((unsigned int) cur->displayFrame.right > ctx->lcd_info.xres) {
                src_rect->h =
                   (cur->sourceCrop.bottom - cur->sourceCrop.top) - src_rect->y -
                   ( (cur->displayFrame.right - ctx->lcd_info.xres) * (src_img->h)
                     / (cur->displayFrame.right - cur->displayFrame.left) );
            } else {
                src_rect->h = (cur->sourceCrop.bottom - cur->sourceCrop.top);
            }
        }

        if (cur->displayFrame.top < 0) {
            src_rect->x = (0 - cur->displayFrame.top) * src_img->w / (cur->displayFrame.bottom - cur->displayFrame.top);

            if ((unsigned int) cur->displayFrame.bottom > ctx->lcd_info.yres) {
               src_rect->w =
                   (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x -
                   ( (cur->displayFrame.bottom - ctx->lcd_info.yres) * (src_img->w)
                     / (cur->displayFrame.bottom - cur->displayFrame.top) );
           } else {
               src_rect->w = (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x;
           }
       } else {
            src_rect->x = cur->sourceCrop.left;

            if ((unsigned int) cur->displayFrame.bottom > ctx->lcd_info.yres) {
               src_rect->w =
                   (cur->sourceCrop.right - cur->sourceCrop.left) - src_rect->x -
                   ( (cur->displayFrame.bottom - ctx->lcd_info.yres) * (src_img->w)
                     / (cur->displayFrame.bottom - cur->displayFrame.top) );
           } else {
               src_rect->w = (cur->sourceCrop.right - cur->sourceCrop.left);
           }
       }
       break;
    }

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "crop information()::"
            "sourceCrop left(%d),top(%d),right(%d),bottom(%d),"
            "src_rect x(%d),y(%d),w(%d),h(%d),"
            "prev_handle w(%d),h(%d)",
            cur->sourceCrop.left,
            cur->sourceCrop.top,
            cur->sourceCrop.right,
            cur->sourceCrop.bottom,
            src_rect->x, src_rect->y, src_rect->w, src_rect->h,
            prev_handle->width, prev_handle->height);

    src_rect->x = SEC_MAX(src_rect->x, 0);
    src_rect->y = SEC_MAX(src_rect->y, 0);
    src_rect->w = SEC_MAX(src_rect->w, 0);
    src_rect->w = SEC_MIN(src_rect->w, prev_handle->width - src_rect->x);
    src_rect->h = SEC_MAX(src_rect->h, 0);
    src_rect->h = SEC_MIN(src_rect->h, prev_handle->height - src_rect->y);

    /* 4. Set dst_rect(fb or lcd)
     *    fimc dst image will be stored from left top corner
     */
    dst_rect->x = 0;
    dst_rect->y = 0;
    dst_rect->w = win->rect_info.w;
    dst_rect->h = win->rect_info.h;

    /* Summary */
    SEC_HWC_Log(HWC_LOG_DEBUG,
            "set_src_dst_img_rect()::"
            "SRC w(%d),h(%d),f_w(%d),f_h(%d),fmt(0x%x),"
            "base(0x%x),offset(%d),paddr(0x%X),mem_id(%d),mem_type(%d)=>\r\n"
            "   DST w(%d),h(%d),f(0x%x),base(0x%x),"
            "offset(%d),mem_id(%d),mem_type(%d),"
            "rot(%d),win_idx(%d)"
            "   SRC_RECT x(%d),y(%d),w(%d),h(%d)=>"
            "DST_RECT x(%d),y(%d),w(%d),h(%d)",
            src_img->w, src_img->h, src_img->f_w, src_img->f_h, src_img->format,
            src_img->base, src_img->offset, src_img->paddr, src_img->mem_id, src_img->mem_type,
            dst_img->w, dst_img->h,  dst_img->format, dst_img->base,
            dst_img->offset, dst_img->mem_id, dst_img->mem_type,
            cur->transform, win_idx,
            src_rect->x, src_rect->y, src_rect->w, src_rect->h,
            dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);

    return 0;
}

static int get_hwc_compos_decision(hwc_layer_1_t* cur, int iter, int win_cnt)
{
    if(cur->flags & HWC_SKIP_LAYER  || !cur->handle) {
        SEC_HWC_Log(HWC_LOG_DEBUG, "%s::is_skip_layer  %d  cur->handle %x ",
                __func__, cur->flags & HWC_SKIP_LAYER, cur->handle);

        return HWC_FRAMEBUFFER;
    }

    private_handle_t *prev_handle = (private_handle_t *)(cur->handle);
    int compositionType = HWC_FRAMEBUFFER;

    if (iter == 0) {
    /* check here....if we have any resolution constraints */
        if (((cur->sourceCrop.right - cur->sourceCrop.left + 1) < 16) ||
            ((cur->sourceCrop.bottom - cur->sourceCrop.top + 1) < 8))
            return compositionType;

        if ((cur->transform == HAL_TRANSFORM_ROT_90) ||
            (cur->transform == HAL_TRANSFORM_ROT_270)) {
            if (((cur->displayFrame.right - cur->displayFrame.left + 1) < 4) ||
                ((cur->displayFrame.bottom - cur->displayFrame.top + 1) < 8))
                return compositionType;
        } else if (((cur->displayFrame.right - cur->displayFrame.left + 1) < 8) ||
                   ((cur->displayFrame.bottom - cur->displayFrame.top + 1) < 4)) {
            return compositionType;
        }

        switch (prev_handle->format) {
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
            compositionType = HWC_OVERLAY;
            break;
        case HAL_PIXEL_FORMAT_YV12:                 /* YCrCb_420_P */
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            if ((prev_handle->usage & GRALLOC_USAGE_HWC_HWOVERLAY) &&
                 (cur->blending == HWC_BLENDING_NONE))
                compositionType = HWC_OVERLAY;
            else
                compositionType = HWC_FRAMEBUFFER;
            break;
        default:
            compositionType = HWC_FRAMEBUFFER;
            break;
        }
    }

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "%s::compositionType(%d)=>0:FB,1:OVERLAY \r\n"
            "   format(0x%x),magic(0x%x),flags(%d),size(%d),offset(%d)"
            "b_addr(0x%x),usage(%d),w(%d),h(%d),bpp(%d)",
            "get_hwc_compos_decision()", compositionType,
            prev_handle->format, prev_handle->magic, prev_handle->flags,
            prev_handle->size, prev_handle->offset, prev_handle->base,
            prev_handle->usage, prev_handle->width, prev_handle->height,
            prev_handle->bpp);

    return  compositionType;
}

static void reset_win_rect_info(hwc_win_info_t *win)
{
    win->rect_info.x = 0;
    win->rect_info.y = 0;
    win->rect_info.w = 0;
    win->rect_info.h = 0;
    return;
}


static int assign_overlay_window(struct hwc_context_t *ctx, hwc_layer_1_t *cur,
        int win_idx, int layer_idx)
{
    struct hwc_win_info_t   *win;
    sec_rect   rect;

    win = &ctx->win[win_idx];

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "%s:: left(%d),top(%d),right(%d),bottom(%d),transform(%d)"
            "lcd_info.xres(%d),lcd_info.yres(%d)",
            "++assign_overlay_window()",
            cur->displayFrame.left, cur->displayFrame.top,
            cur->displayFrame.right, cur->displayFrame.bottom, cur->transform,
            win->lcd_info.xres, win->lcd_info.yres);

    calculate_rect(cur, ctx, &rect);

    if ((rect.x != win->rect_info.x) || (rect.y != win->rect_info.y) ||
        (rect.w != win->rect_info.w) || (rect.h != win->rect_info.h)){
        win->rect_info.x = rect.x;
        win->rect_info.y = rect.y;
        win->rect_info.w = rect.w;
        win->rect_info.h = rect.h;
        win->field8c = 0;
        win->field90 = 1;
    }

    if ((rect.w == win->rect_info.w) && (rect.h == win_info_t.rect_info.h)) {
        if (!win_info_t.secion_param.size) {
            window_buffer_allocate(ctx, win, rect.x, rect.y, rect.w, rect.h);
            win->field90 = 1;
        }
    } else {
        window_buffer_deallocate(ctx, win);
        window_buffer_allocate(ctx, win, rect.x, rect.y, rect.w, rect.h);
    }

    ctx->str16c0_field4 += (unsigned int) (win->var_info.xres * win->var_info.yres * win->lcd_info.bits_per_pixel) / 8;

    win->layer_index = layer_idx;
    win->status = HWC_WIN_RESERVED;

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "%s:: win_x %d win_y %d win_w %d win_h %d  lay_idx %d win_idx %d\n",
            "--assign_overlay_window()",
            win->rect_info.x, win->rect_info.y, win->rect_info.w,
            win->rect_info.h, win->layer_index, win_idx );

    return 0;
}

int runCompositor(hwc_context_t *ctx, sec_img *src_img, sec_rect *src_rect,
        sec_img *dst_img, sec_rect *dst_rect, unsigned int rotation,
        unsigned int global_alpha, unsigned long solid_color,
        enum blit_op blit_op, addr_space adr_space_src, addr_space adr_space_dst)
{
    int retry = 0;
    unsigned int g2d_format = 0, pixel_order = 0, src_bpp = 0, dst_bpp = 0;
    struct fimg2d_image g2d_src_img, g2d_dst_img, g2d_temp_img;
    struct fimg2d_blit fimg_cmd;
    enum rotation l_rotate;

    fimg_cmd.op = blit_op;
    fimg_cmd.param.g_alpha = global_alpha;

    fimg_cmd.param.premult = PREMULTIPLIED;
    fimg_cmd.param.dither = 0;

    fimg_cmd.param.rotate = (enum rotation) rotateValueHAL2G2D(rotation);

    fimg_cmd.param.repeat.mode = NO_REPEAT;
    fimg_cmd.param.solid_color = solid_color;
    fimg_cmd.param.repeat.pad_color = 0;
    fimg_cmd.param.bluscr.mode = OPAQUE;
    fimg_cmd.param.bluscr.bs_color = 0;
    fimg_cmd.param.bluscr.bg_color = 0;

    if (fimg_cmd.param.rotate != ROT_90 && fimg_cmd.param.rotate != ROT_270) {
        if (src_img->paddr) {
            if ((src_rect->w != dst_rect->w) || (src_rect->h != dst_rect->h)) {
                fimg_cmd.param.scaling.mode = SCALING_BILINEAR;
            	fimg_cmd.param.scaling.src_w = src_rect->w;
                fimg_cmd.param.scaling.src_h = src_rect->h;
                fimg_cmd.param.scaling.dst_w = dst_rect->w;
                fimg_cmd.param.scaling.dst_h = dst_rect->h;
            } else {
                fimg_cmd.param.scaling.mode = NO_SCALING;
            }
        } else {
            fimg_cmd.param.scaling.mode = NO_SCALING;
        }
    } else if (src_img->paddr) {
        if ((src_rect->w != dst_rect->h) || (src_rect->h != src_rect->w)) {
            fimg_cmd.param.scaling.mode = SCALING_BILINEAR;
            fimg_cmd.param.scaling.src_w = src_rect->w;
            fimg_cmd.param.scaling.src_h = src_rect->h;
            fimg_cmd.param.scaling.dst_h = dst_rect->w;
            fimg_cmd.param.scaling.dst_w = dst_rect->h;
        } else {
            fimg_cmd.param.scaling.mode = NO_SCALING;
        }
    } else {
        fimg_cmd.param.scaling.mode = NO_SCALING;
    }

    if (src_img->paddr) {
        formatValueHAL2G2D(src_img->format, &g2d_format, &pixel_order, &src_bpp);

        g2d_src_img.addr.type = adr_space_src;
        g2d_src_img.addr.start = src_img->paddr;

        if (src_bpp == 1) {
        	g2d_src_img.plane2.type = adr_space_src;
        	g2d_src_img.plane2.start = src_img->paddr + src_img->uoffset;
        }

        g2d_src_img.width = src_img->f_w;
        g2d_src_img.height = src_img->f_h;
        g2d_src_img.stride = g2d_src_img.width * src_bpp;
        g2d_src_img.order = (enum pixel_order) pixel_order;
        g2d_src_img.fmt = (enum color_format) g2d_format;
        g2d_src_img.rect.x1 = src_rect->x;
        g2d_src_img.rect.y1 = src_rect->y;
        g2d_src_img.rect.x2 = src_rect->x + src_rect->w;
        g2d_src_img.rect.y2 = src_rect->y + src_rect->h;
        g2d_src_img.need_cacheopr = false;

        fimg_cmd.src = &g2d_src_img;
    } else {
        fimg_cmd.src = NULL;
        fimg_cmd.param.scaling.mode = NO_SCALING;
    }

    if (dst_img->paddr) {
        formatValueHAL2G2D(dst_img->format, &g2d_format, &pixel_order, &dst_bpp);

        g2d_dst_img.addr.type = adr_space_dst;
        g2d_dst_img.addr.start = dst_img->paddr;

        if (dst_bpp == 1) {
            g2d_dst_img.plane2.type = adr_space_dst;
            g2d_dst_img.plane2.start = dst_img->paddr + dst_img->uoffset;

            dst_rect->x = EXYNOS4_ALIGN(dst_rect->x, 2);
            dst_rect->y = EXYNOS4_ALIGN(dst_rect->y, 2);
            dst_rect->w = EXYNOS4_ALIGN(dst_rect->w, 2);
            dst_rect->h = EXYNOS4_ALIGN(dst_rect->h, 2);
        }

        g2d_dst_img.width = dst_img->f_w;
        g2d_dst_img.height = dst_img->f_h;
        g2d_dst_img.stride = g2d_dst_img.width * dst_bpp;
        g2d_dst_img.order = (enum pixel_order) pixel_order;
        g2d_dst_img.fmt = (enum color_format) g2d_format;
        g2d_dst_img.rect.x1 = dst_rect->x;
        g2d_dst_img.rect.y1 = dst_rect->y;
        g2d_dst_img.rect.x2 = dst_rect->x + dst_rect->w;
        g2d_dst_img.rect.y2 = dst_rect->y + dst_rect->h;
        g2d_dst_img.need_cacheopr = false;

        fimg_cmd.dst = &g2d_dst_img;
        fimg_cmd.param.clipping.enable = false;
    } else  {
        fimg_cmd.dst = NULL;
    }

    fimg_cmd.msk = NULL;
    fimg_cmd.tmp = NULL;

    if (ctx->fimg_secure)
        fimg_cmd.seq_no = SEQ_NO_BLT_HWC_SEC;
    else
        fimg_cmd.seq_no = SEQ_NO_BLT_HWC_NOSEC;

    if (adr_space_dst == ADDR_USER) {
        for(retry = 0; stretchFimgApi(&fimg_cmd) < 0; ++retry) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s:: stretchFimgApi error, retry=%d", __func__, retry);
            usleep(10);

            if (retry == 20000)
                return -1;
        }

    } else if (fimg_cmd.param.scaling.mode == SCALING_BILINEAR &&
               (fimg_cmd.param.rotate == ROT_90 || fimg_cmd.param.rotate == ROT_270) &&
               dst_bpp == 1 ) {
        if (ctx->fimg_ion_hdl.param.buffer == -1)
            createIONMem(&ctx->fimg_ion_hdl.param, dst_rect->w * dst_rect->h * dst_bpp, ION_HEAP_EXYNOS_CONTIG_MASK); // Bit 23 is also active, but don't know the meaning

        //keep a copy of g2d_dst_img for a second fimg operation
        memcpy(&g2d_temp_img, &g2d_dst_img, sizeof(struct fimg2d_image));

        l_rotate = fimg_cmd.param.rotate;

        fimg_cmd.dst->width = dst_rect->h;
        fimg_cmd.dst->height = dst_rect->w;
        fimg_cmd.dst->stride = fimg_cmd.dst->width * src_bpp;
        fimg_cmd.dst->rect.x1 = 0;
        fimg_cmd.dst->rect.y1 = 0;
        fimg_cmd.dst->rect.x2 = dst_rect->h;
        fimg_cmd.dst->rect.y2 = dst_rect->w;
        fimg_cmd.dst->fmt = fimg_cmd.src->fmt;
        fimg_cmd.dst->order = fimg_cmd.src->order;

        fimg_cmd.param.rotate = ORIGIN;

        fimg_cmd.dst->addr.start = ctx->fimg_ion_hdl.param.physaddr;

        if (stretchFimgApi(&fimg_cmd) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s:: stretchFimgApi error, op 1", __func__);
            return -1;
        }

        memcpy(&g2d_src_img, &g2d_dst_img, sizeof(struct fimg2d_image));
        memcpy(&g2d_dst_img, &g2d_temp_img, sizeof(struct fimg2d_image));

        fimg_cmd.param.rotate = l_rotate;
        fimg_cmd.param.scaling.mode = NO_SCALING;

        if (stretchFimgApi(&fimg_cmd) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s:: stretchFimgApi error, op 2", __func__);
            return -1;
        }
    } else  if (stretchFimgApi(&fimg_cmd) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s:: stretchFimgApi error", __func__);
        return -1;
    }

    return 0;
}

int clearBufferbyG2D(hwc_context_t *ctx, sec_img *dst_img)
{
	sec_rect src_rect, dst_rect;
    sec_img src_img;
    int rc = 0;

    memset(&src_rect, 0, sizeof(sec_rect));

    src_rect.w = dst_img->f_w;
    src_rect.h = dst_img->f_h;

    memset(&dst_rect, 0, sizeof(sec_rect));

    src_img.paddr = 0;

    dst_rect.w = src_rect.w;
    dst_rect.h = src_rect.h;

    rc = runCompositor(ctx, &src_img, &src_rect, dst_img, &dst_rect, ORIGIN,
              G2D_ALPHA_VALUE_MAX, 0xFF000000u, BLIT_OP_CLR, ADDR_PHYS, ADDR_PHYS);
    if (rc < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s error : ret=%d\n", __func__);

    return rc;
}

void clearAllExtFb(hwc_context_t *ctx, sec_img dst_img, int unknown)
{
    int i = 1;

    dst_img.base = dst_img.paddr;

    if (unknown)
        clearBufferbyFimc(ctx, &dst_img);
    else
        clearBufferbyG2D(ctx, &dst_img);

    do {
        dst_img.paddr = ctx->fbdev1_win.addr[(ctx->fbdev1_win.buf_index + i) % 3];
        dst_img.base = dst_img.paddr;

        if (unknown)
            clearBufferbyFimc(ctx, &dst_img);
        else
            clearBufferbyG2D(ctx, &dst_img);

        ++i;
    } while (i < NUM_OF_WIN_BUF);
}

int compose_rgb_overlay_layer(hwc_context_t *ctx, hwc_layer_1 *layer, hwc_win_info_t *win, int)
{
	size_t size = 0;
	int result = 0;
    sec_img src_img, dst_img;
    sec_rect src_rect, dst_rect;
    private_handle_t *handle = (private_handle_t *) layer->handle;
    unsigned int mem_type;

    if (layer->acquireFenceFd >= 0) {
        if (sync_wait(layer->acquireFenceFd, 1000) < 0)
            SEC_HWC_Log(HWC_LOG_WARNING, "%s::sync_wait acquire fence error", __func__);

        usleep(3000);
        close(layer->acquireFenceFd);
        layer->acquireFenceFd = -1;
    }

    win->buf_index = (win->buf_index + 1) % 3;

    size = EXYNOS4_ALIGN(win->rect_info.h, 2)
      * EXYNOS4_ALIGN(win->rect_info.w, 16)
      * win->lcd_info.bits_per_pixel * 8;

    if (win->secion_param[win->buf_index].size != size) {
        destroyIONMem(&win->secion_param[win->buf_index]);

        if (!win->secion_param[win->buf_index].size)
            createIONMem(&win->secion_param[win->buf_index], size, ION_HEAP_EXYNOS_CONTIG_MASK); // Bit 23 is also active, but don't know the meaning

        if (win->secion_param[win->buf_index].size)
            win->addr[win->buf_index] = win->secion_param[win->buf_index].physaddr;
    }

    set_src_dst_img_rect(layer, ctx, win, &src_img, &dst_img, &src_rect, &dst_rect, win->buf_index);

    if (win->fence[win->buf_index] >= 0) {
        if (sync_wait(win->fence[win->buf_index], 1000) < 0)
            SEC_HWC_Log(HWC_LOG_WARNING, "%s::sync_wait release fence error", __func__);

        close(win->fence[win->buf_index]);
        win->fence[win->buf_index] = -1;
    }

    if (handle->usage & GRALLOC_USAGE_HW_ION) {
        mem_type = HWC_PHYS_MEM_TYPE;
        dst_img.paddr = win->secion_param[win->buf_index].physaddr;
    } else {
        mem_type = HWC_VIRT_MEM_TYPE;
        src_img.paddr = src_img.base;
        dst_img.paddr = (uint32_t) win->secion_param[win->buf_index].memory;

        if (!layer->transform)
            dst_img.offset = win->secion_param[win->buf_index].physaddr;
    }

    src_img.mem_type = mem_type;
    dst_img.mem_type = mem_type;

    if (!layer->transform)
        result = runCompositor(ctx, &src_img, &src_rect, &dst_img, &dst_rect, layer->transform, G2D_ALPHA_VALUE_MAX, 0, BLIT_OP_SRC, ADDR_USER, ADDR_USER_RSVD);
    else
        result = runCompositor(ctx, &src_img, &src_rect, &dst_img, &dst_rect, 0, G2D_ALPHA_VALUE_MAX, 0, BLIT_OP_SRC, ADDR_USER, ADDR_USER);

    return result;
}

int SurfaceMerge(hwc_context_t *ctx, hwc_display_contents_1 *disp)
{
    struct timeval start, end;
    struct hwc_win_info_t *win = &ctx->fbdev1_win;
    sec_img l_src_img;
    struct s3cfb_extdsp_time_stamp fb_ts;

    gettimeofday(&start, NULL);

    win->var_info.activate &= ~FB_ACTIVATE_MASK;
    win->var_info.activate |= FB_ACTIVATE_FORCE;

    if ( (win->lcd_info.xres != win->var_info.xres_virtual) || (win->var_info.yres_virtual != (win->lcd_info.yres * 3)) ||
            (win->lcd_info.xres != win->var_info.xres) || (win->var_info.yres != (win->lcd_info.yres * 3)) ) {
        win->var_info.xres = win->lcd_info.xres;
        win->var_info.xres_virtual = win->lcd_info.xres;
        win->var_info.yres = win->lcd_info.yres * 3;

        if (ioctl(win->fd, FBIOPUT_VSCREENINFO, &(win->var_info)) < 0) {
            //TODO: detect what else has been stored at this point in blob
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPUT_VSCREENINFO(%d, %d) fail",
                __func__, win->rect_info.w, win->rect_info.h);
            dump_win(win);
            return -1;
        }

        win->fix_info.line_length = (win->var_info.xres_virtual * win->lcd_info.bits_per_pixel ) / 8;
        win->size = win->fix_info.line_length * win->var_info.yres;

        if (!ctx->str1980_field18) {
            win->addr[0] = win->fix_info.smem_start;
            win->addr[1] = win->fix_info.smem_start + win->size;
            win->addr[2] = win->fix_info.smem_start + (win->size * 2);
        }

        l_src_img.format = HAL_PIXEL_FORMAT_RGBX_8888;

        if (ctx->str1980_field18) {
            win->buf_index = get_user_ptr_buf_index(win);
        } else {
            win->buf_index = get_buf_index(win);
        }

        fb_ts.phys_addr = win->addr[win->buf_index];

        if (ioctl(win->fd, S3CFB_EXTDSP_PUT_TIME_STAMP, &fb_ts) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s:: S3CFB_EXTDSP_PUT_TIME_STAMP fail", __func__);
            return -1;
        }

        if (!start.tv_sec) {
            ioctl(win->fd, S3CFB_EXTDSP_SET_WIN_ADDR, (unsigned long *) &fb_ts.phys_addr);
        }

        ctx->str1960_fieldc == -1; //retireFence?
        //TODO: sub_29B4
        //TODO: clearAllExtFb(hwc_context_t *,sec_img,int)
    }

    return 0;
}

#ifdef SKIP_DUMMY_UI_LAY_DRAWING
static void get_hwc_ui_lay_skipdraw_decision(struct hwc_context_t* ctx,
                               hwc_display_contents_1_t* list)
{
    private_handle_t *prev_handle;
    hwc_layer_1_t* cur;
    int num_of_fb_lay_skip = 0;
    int fb_lay_tot = ctx->num_of_fb_layer + ctx->num_of_fb_lay_skip;

    if (fb_lay_tot > NUM_OF_DUMMY_WIN)
        return;

    if (fb_lay_tot < 1)
        return;

    if (ctx->fb_lay_skip_initialized) {
        for (int cnt = 0; cnt < fb_lay_tot; cnt++) {
            cur = &list->hwLayers[ctx->win_virt[cnt].layer_index];

            if ( (ctx->win_virt[cnt].layer_prev_buf == (uint32_t)cur->handle) &&
                 (ctx->win_virt[cnt].crop_info.x == cur->sourceCrop.left) &&
                 (ctx->win_virt[cnt].crop_info.y == cur->sourceCrop.top) &&
                 (ctx->win_virt[cnt].crop_info.w == cur->sourceCrop.right - cur->sourceCrop.left) &&
                 (ctx->win_virt[cnt].crop_info.h == cur->sourceCrop.bottom - cur->sourceCrop.top) &&
                 (ctx->win_virt[cnt].display_frame.x == cur->displayFrame.left) &&
                 (ctx->win_virt[cnt].display_frame.y == cur->displayFrame.top) &&
                 (ctx->win_virt[cnt].display_frame.w == cur->displayFrame.right - cur->displayFrame.left) &&
                 (ctx->win_virt[cnt].display_frame.h == cur->displayFrame.bottom - cur->displayFrame.top) )
                num_of_fb_lay_skip++;
            }
        }

        if (num_of_fb_lay_skip != fb_lay_tot) {
            ctx->num_of_fb_layer = fb_lay_tot;
            ctx->num_of_fb_lay_skip = 0;

            for (int cnt = 0; cnt < fb_lay_tot; cnt++) {
                cur = &list->hwLayers[ctx->win_virt[cnt].layer_index];

                ctx->win_virt[cnt].layer_prev_buf = (uint32_t)cur->handle;
                ctx->win_virt[cnt].crop_info.x = cur->sourceCrop.left;
                ctx->win_virt[cnt].crop_info.y = cur->sourceCrop.top;
                ctx->win_virt[cnt].crop_info.w = cur->sourceCrop.right - cur->sourceCrop.left;
                ctx->win_virt[cnt].crop_info.h = cur->sourceCrop.bottom - cur->sourceCrop.top;

                ctx->win_virt[cnt].display_frame.x = cur->displayFrame.left;
                ctx->win_virt[cnt].display_frame.y = cur->displayFrame.top;
                ctx->win_virt[cnt].display_frame.w = cur->displayFrame.right - cur->displayFrame.left;
                ctx->win_virt[cnt].display_frame.h = cur->displayFrame.bottom - cur->displayFrame.top;

                cur->compositionType = HWC_FRAMEBUFFER;
                ctx->win_virt[cnt].status = HWC_WIN_FREE;
            }
        } else {
            ctx->num_of_fb_layer = 0;
            ctx->num_of_fb_lay_skip = fb_lay_tot;

            for (int cnt = 0; cnt < fb_lay_tot; cnt++) {
                cur = &list->hwLayers[ctx->win_virt[cnt].layer_index];
                cur->compositionType = HWC_OVERLAY;
                ctx->win_virt[cnt].status = HWC_WIN_RESERVED;
            }
        }
    } else {
        ctx->num_of_fb_lay_skip = 0;

        for (int i = 0; i < list->numHwLayers ; i++) {
            if(num_of_fb_lay_skip >= NUM_OF_DUMMY_WIN)
                break;

            cur = &list->hwLayers[i];
            if (cur->handle) {
                prev_handle = (private_handle_t *)(cur->handle);

                if ( (cur->compositionType == HWC_FRAMEBUFFER) &&
                     (!prev_handle->flags & GRALLOC_USAGE_PRIVATE_0) ) {

                    ++num_of_fb_lay_skip;
                    ctx->win_virt[num_of_fb_lay_skip].layer_prev_buf = (uint32_t)cur->handle;
                    ctx->win_virt[num_of_fb_lay_skip].layer_index = i;
                    ctx->win_virt[num_of_fb_lay_skip].status = HWC_WIN_FREE;

                    ctx->win_virt[num_of_fb_lay_skip].crop_info.x = cur->sourceCrop.left;
                    ctx->win_virt[num_of_fb_lay_skip].crop_info.y = cur->sourceCrop.top;
                    ctx->win_virt[num_of_fb_lay_skip].crop_info.w = cur->sourceCrop.right - cur->sourceCrop.left;
                    ctx->win_virt[num_of_fb_lay_skip].crop_info.h = cur->sourceCrop.bottom - cur->sourceCrop.top;

                    ctx->win_virt[num_of_fb_lay_skip].display_frame.x = cur->displayFrame.left;
                    ctx->win_virt[num_of_fb_lay_skip].display_frame.y = cur->displayFrame.top;
                    ctx->win_virt[num_of_fb_lay_skip].display_frame.w = cur->displayFrame.right - cur->displayFrame.left;
                    ctx->win_virt[num_of_fb_lay_skip].display_frame.h = cur->displayFrame.bottom - cur->displayFrame.top;
                }
            } else {
                cur->compositionType = HWC_FRAMEBUFFER;
            }
        }

        if (num_of_fb_lay_skip == fb_lay_tot)
            ctx->fb_lay_skip_initialized = 1;
    }

    return;

}
#endif

static int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
    uint32_t* configs, size_t* numConfigs)
{
    return 0;
}

static int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
    uint32_t config, const uint32_t* attributes, int32_t* values)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int i=0;

    while(attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch(disp) {
        case 0:

            switch(attributes[i]){
            case HWC_DISPLAY_VSYNC_PERIOD: /* The vsync period in nanoseconds */
                values[i] = ctx->vsync_period;
                break;

            case HWC_DISPLAY_WIDTH: /* The number of pixels in the horizontal and vertical directions. */
            	values[i] = ctx->width;
                break;

            case HWC_DISPLAY_HEIGHT:
            	values[i] = ctx->height;
                break;

            case HWC_DISPLAY_DPI_X:
            	values[i] = ctx->xdpi;
            	break;

            case HWC_DISPLAY_DPI_Y:
            	values[i] = ctx->ydpi;
            	break;


            case HWC_DISPLAY_SECURE:
                /* Indicates if the display is secure
                 * For HDMI/WFD if the sink supports HDCP, it will be true
                 * Primary panel is always considered secure
                 */
            	break;

            default:
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::unknown display attribute %d", __func__, attributes[i]);
                return -EINVAL;
            }
            break;

        case 1:
            //TODO: no hdmi at the moment
            break;

        default:
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::unknown display %d", __func__, disp);
            return -EINVAL;
        }

        i++;
    }
    return 0;
}

static int hwc_prepare(hwc_composer_device_1_t *dev, size_t numDisplays, hwc_display_contents_1_t** displays)
{

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int overlay_win_cnt = 0;
    int compositionType = 0;
    int ret;
    int needs_change = 0;
#if defined(BOARD_USES_HDMI)
    int hdmiCableStatus;
    int hdmiMirrorWithVideoMode;
    int hdmiForceMirrorMode;
    int l_str1980_field1c;
#endif

    // Compat
    hwc_display_contents_1_t* list = NULL;
    hwc_display_contents_1_t* ext_disp = NULL;

    if ((numDisplays == 0) || (displays == NULL))
        return 0;

    if (numDisplays > 0) {
        list = displays[0];
        ext_disp = displays[1];
    }

#if defined(BOARD_USES_HDMI)
    hdmiMirrorWithVideoMode = mHdmiClient->getMirrorWithVideoMode();

    /* not here
    android::SecHdmiClient *mHdmiClient = android::SecHdmiClient::getInstance(); */
    hdmiCableStatus = mHdmiClient->getHdmiCableStatus();

    // not yet ctx->hdmiCableStatus = hdmiCableStatus;

    hdmiForceMirrorMode = mHdmiClient->getForceMirrorMode();
    ctx->hdmiVideoTransform = mHdmiClient->getVideoTransform();

    l_str1980_field1c = ctx->str1980_field1c;

    if (hdmiMirrorWithVideoMode != ctx->hdmiMirrorWithVideoMode) {
        ctx->hdmiMirrorWithVideoMode = hdmiMirrorWithVideoMode;
        needs_change = 1;
    } else {
        needs_change = 0;
    }

    hal_format = 0;
    bpp = 0;

    if (ctx->field_ac <= 5)
       ctx->field_ac++;

    if (ctx->field_ac <= 5) {
        ctx->vsync_A6 = 1;
        ctx->vsync_A5 = 1;
    }
#endif

    ctx->avoid_eglSwapBuffers = true;
    ctx->str16a0_field0 = 0;

    if (ctx->wfd_connected != ctx->wfd_connected_last) {
        if (ctx->procs) {
            SEC_HWC_Log(HWC_LOG_DEBUG,"WFD %d connected", 1);
            ctx->procs->hotplug(ctx->procs, 1, ctx->wfd_connected);
        }
        ctx->wfd_connected_last = ctx->wfd_connected;
    }

#if defined(BOARD_USES_HDMI)
    if (ctx->hdmiCableStatus == hdmiCableStatus) {
        ctx->hdmi_cable_status_changed = false;
    } else {
        ctx->hdmiCableStatus = hdmiCableStatus;
        ctx->hdmi_cable_status_changed = true;
        ctx->str1960_fieldc = -1;

        if (ctx->procs) {
            if (hdmiCableStatus == 1 && ctx->wfd_connected == 1) {
                ctx->wfd_connected = 0;
                ctx->wfd_connected_last = 0;
                ctx->procs->hotplug(ctx->procs, 1, 0);
            }

            SEC_HWC_Log(HWC_LOG_DEBUG,"hdmi %d connected", 1);
            ctx->procs->hotplug(ctx->procs, 1, 1);
        }
    }
#endif

/*
#ifdef SKIP_DUMMY_UI_LAY_DRAWING
    if ((list && (!(list->flags & HWC_GEOMETRY_CHANGED))) &&
	(ctx->num_of_hwc_layer > 0)) {
      get_hwc_ui_lay_skipdraw_decision(ctx, list);
      return 0;
    }
    ctx->fb_lay_skip_initialized = 0;
    ctx->num_of_fb_lay_skip = 0;
#ifdef GL_WA_OVLY_ALL
    ctx->ui_skip_frame_cnt = 0;
#endif

    for (int i = 0; i < NUM_OF_DUMMY_WIN; i++) {
        ctx->win_virt[i].layer_prev_buf = 0;
        ctx->win_virt[i].layer_index = -1;
        ctx->win_virt[i].status = HWC_WIN_FREE;
    }
#endif
*/

    int no_handle_or_not_usage_hwc_overlay = 0;

    for (int i = 0; i < list->numHwLayers; i++) {
        hwc_layer_1_t *cur = &list->hwLayers[i];

        if ((cur->compositionType != HWC_BACKGROUND) &&
            (cur->compositionType != HWC_FRAMEBUFFER_TARGET) &&
            (cur->flags & HWC_GEOMETRY_CHANGED)) {

            if (!cur->handle) {
                no_handle_or_not_usage_hwc_overlay = 1;
                break;
            } else if ((cur->handle->usage >= 0) && !(cur->handle->usage & GRALLOC_USAGE_HWC_HWOVERLAY)) {
            	no_handle_or_not_usage_hwc_overlay = 1;
            	break;
            }
        }
    }

    int num_of_protected_layer = 0;

    for (int i = 0; i < list->numHwLayers; i++) {
        hwc_layer_1_t *cur = &list->hwLayers[i];

        if (cur->handle) {
            if (cur->handle->usage & GRALLOC_USAGE_PRIVATE_0) {
                ctx->num_of_private_layer++;
                ctx->max_private_layer = i;
            } else if (cur->handle->usage & GRALLOC_USAGE_PROTECTED) {
                num_of_protected_layer++;
            }
        }
    }

    for (int i = 0; i < list->numHwLayers; i++) {
        hwc_layer_1_t *cur = &list->hwLayers[i];

        if ((cur->compositionType != HWC_BACKGROUND) &&
            (cur->compositionType != HWC_FRAMEBUFFER_TARGET)) {
            if (!cur->handle) {
                if (cur->handle->usage & GRALLOC_USAGE_EXTERNAL_DISP || cur->handle->usage < 0 || cur->handle->usage & GRALLOC_USAGE_HWC_HWOVERLAY) {
                	ctx->num_of_ext_disp_layer++;

                    if (cur->handle->usage & GRALLOC_USAGE_EXTERNAL_DISP || cur->handle->usage < 0 ||
                        check_yuv_format((unsigned int)prev_handle->format) == 1) {
                        ctx->num_of_ext_disp_video_layer++;
                    } else {
                    	 if (cur->handle->usage >= 0) {
                    		 if (cur->handle->usage & GRALLOC_USAGE_HWC_HWOVERLAY)
                    			 ctx->num_of_hwoverlay_layer++;
                    	 } else {
                    		 ctx->num_of_other_layer++;
                    	 }
                    }

                    if (cur->handle->usage & GRALLOC_USAGE_CAMERA)
                        ctx->num_of_camera_layer++;
                }

                if (cur->handle->usage & GRALLOC_USAGE_CURSOR)
                    ctx->num_of_cursor_layer++;
            }
        }
    }










    //if geometry is not changed, there is no need to do any work here
    if (!list || (!(list->flags & HWC_GEOMETRY_CHANGED)))
        return 0;

    //all the windows are free here....
    for (int i = 0 ; i < NUM_HW_WINDOWS; i++) {
        ctx->win[i].status = HWC_WIN_FREE;
        ctx->win[i].buf_index = 0;
    }

    ctx->num_of_hwc_layer = 0;
    ctx->num_of_fb_layer = 0;
    ctx->num_2d_blit_layer = 0;
    ctx->num_of_ext_disp_video_layer = 0;

    for (int i = 0; i < list->numHwLayers; i++) {
        hwc_layer_1_t *cur = &list->hwLayers[i];
        private_handle_t *prev_handle = NULL;
        if (cur->handle) {
            prev_handle = (private_handle_t *)(cur->handle);
            SEC_HWC_Log(HWC_LOG_DEBUG, "prev_handle->usage = %d", prev_handle->usage);
            if (prev_handle->usage & GRALLOC_USAGE_EXTERNAL_DISP) {
                ctx->num_of_ext_disp_layer++;
                if ((prev_handle->usage & GRALLOC_USAGE_EXTERNAL_DISP) ||
                    check_yuv_format((unsigned int)prev_handle->format) == 1) {
                    ctx->num_of_ext_disp_video_layer++;
                }
            }
        }
    }

    for (int i = 0; i < list->numHwLayers ; i++) {
        hwc_layer_1_t* cur = &list->hwLayers[i];
        private_handle_t *prev_handle = (private_handle_t *)(cur->handle);

        if (overlay_win_cnt < NUM_HW_WINDOWS) {
            compositionType = get_hwc_compos_decision(cur, 0, overlay_win_cnt);

            if (compositionType == HWC_FRAMEBUFFER) {
                cur->compositionType = HWC_FRAMEBUFFER;
                ctx->num_of_fb_layer++;
            } else {
                ret = assign_overlay_window(ctx, cur, overlay_win_cnt, i);
                if (ret != 0) {
                    SEC_HWC_Log(HWC_LOG_ERROR, "assign_overlay_window fail, change to frambuffer");
                    cur->compositionType = HWC_FRAMEBUFFER;
                    ctx->num_of_fb_layer++;
                    continue;
                }

                cur->compositionType = HWC_OVERLAY;
                cur->hints = HWC_HINT_CLEAR_FB;
                overlay_win_cnt++;
                ctx->num_of_hwc_layer++;
            }
        } else {
            cur->compositionType = HWC_FRAMEBUFFER;
            ctx->num_of_fb_layer++;
        }
#if defined(BOARD_USES_HDMI)
        SEC_HWC_Log(HWC_LOG_DEBUG, "ext disp vid = %d, cable status = %d, composition type = %d",
                ctx->num_of_ext_disp_video_layer, ctx->hdmiCableStatus, compositionType);
        if (ctx->num_of_ext_disp_video_layer >= 2) {
            if ((ctx->hdmiCableStatus) &&
                    (compositionType == HWC_OVERLAY) &&
                    (prev_handle->usage & GRALLOC_USAGE_EXTERNAL_DISP)) {
                cur->compositionType = HWC_FRAMEBUFFER;
                ctx->num_of_hwc_layer--;
                overlay_win_cnt--;
                ctx->num_of_fb_layer++;
                cur->hints = 0;
            }
        }
#endif
    }

#if defined(BOARD_USES_HDMI)
    mHdmiClient = android::SecHdmiClient::getInstance();
    mHdmiClient->setHdmiHwcLayer(ctx->num_of_hwc_layer);
    if (ctx->num_of_ext_disp_video_layer > 1) {
        mHdmiClient->setExtDispLayerNum(0);
    }
#endif

    if (list->numHwLayers != (ctx->num_of_fb_layer + ctx->num_of_hwc_layer))
        SEC_HWC_Log(HWC_LOG_DEBUG,
                "%s:: numHwLayers %d num_of_fb_layer %d num_of_hwc_layer %d ",
                __func__, list->numHwLayers, ctx->num_of_fb_layer,
                ctx->num_of_hwc_layer);

    if (overlay_win_cnt < NUM_HW_WINDOWS) {
        //turn off the free windows
        for (int i = overlay_win_cnt; i < NUM_HW_WINDOWS; i++) {
            window_hide(&ctx->win[i]);
            reset_win_rect_info(&ctx->win[i]);
        }
    }

    return 0;
}

static int hwc_set(hwc_composer_device_1_t *dev,
                   size_t numDisplays,
                   hwc_display_contents_1_t** displays)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    int skipped_window_mask = 0;
    hwc_layer_1_t* cur;
    struct hwc_win_info_t   *win;
    int ret;
    int pmem_phyaddr;
    struct sec_img src_img;
    struct sec_img dst_img;
    struct sec_rect src_work_rect;
    struct sec_rect dst_work_rect;
    bool need_swap_buffers = ctx->num_of_fb_layer > 0;

    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));
    memset(&src_work_rect, 0, sizeof(src_work_rect));
    memset(&dst_work_rect, 0, sizeof(dst_work_rect));

#if defined(BOARD_USES_HDMI)
    int skip_hdmi_rendering = 0;
    int rotVal = 0;
#endif

    // Only support one display
    hwc_display_contents_1_t* list = displays[0];

    if (!list) {
        //turn off the all windows
        for (int i = 0; i < NUM_HW_WINDOWS; i++) {
            window_hide(&ctx->win[i]);
            reset_win_rect_info(&ctx->win[i]);
            ctx->win[i].status = HWC_WIN_FREE;
        }
        ctx->num_of_hwc_layer = 0;
        need_swap_buffers = true;

        if (list->sur == NULL && list->dpy == NULL) {
#ifdef SKIP_DUMMY_UI_LAY_DRAWING
            ctx->fb_lay_skip_initialized = 0;
#endif
            return HWC_EGL_ERROR;
        }
    }

    if(ctx->num_of_hwc_layer > NUM_HW_WINDOWS)
        ctx->num_of_hwc_layer = NUM_HW_WINDOWS;


    /* vvvvvvvvvv TODO: place on the right place */
    // Detect fb layer
    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (ctx->fb_window != NO_FB_NEEDED) {
        for (i = 0; i < list->numHwLayers; i++) {
            if (list->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET) {
                ctx->overlay_map[ctx->fb_window] = i;
                fb_layer = &contents->hwLayers[i]; //not in blob, copied from exynos5 hwc
                break;
            }
        }

        if (CC_UNLIKELY(!fb_layer)) {
            ALOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        }
    }

    if ((ctx->fb_window != NO_FB_NEEDED) || (ctx->num_of_hwc_layer > 0)) {
        if (!err) {
            // this is post_fimd() in exynos5 hwc

            //struct s3c_fb_win_config_data win_data;
            //struct s3c_fb_win_config *config = win_data.config;

            memset(config, 0, sizeof(win_data.config));
            for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
                config[i].fence_fd = -1;
            }
    }
    /* ^^^^^^^^^TODO put it in the right place */




    /*
     * H/W composer documentation states:
     * There is an implicit layer containing opaque black
     * pixels behind all the layers in the list.
     * It is the responsibility of the hwcomposer module to make
     * sure black pixels are output (or blended from).
     *
     * Since we're using a blitter, we need to erase the frame-buffer when
     * switching to all-overlay mode.
     *
     */
    if (ctx->num_of_hwc_layer &&
        ctx->num_of_fb_layer==0 && ctx->num_of_fb_layer_prev) {
#ifdef SKIP_DUMMY_UI_LAY_DRAWING
        if (ctx->num_of_fb_lay_skip == 0)
#endif
        {
            glDisable(GL_SCISSOR_TEST);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            glEnable(GL_SCISSOR_TEST);
            need_swap_buffers = true;
        }
    }
    ctx->num_of_fb_layer_prev = ctx->num_of_fb_layer;

    //compose hardware layers here
    for (int i = 0; i < ctx->num_of_hwc_layer - ctx->num_2d_blit_layer; i++) {
        win = &ctx->win[i];
        if (win->status == HWC_WIN_RESERVED) {
            cur = &list->hwLayers[win->layer_index];

            if (cur->compositionType == HWC_OVERLAY) {
                if (ctx->layer_prev_buf[i] == (uint32_t)cur->handle) {
                    /*
                     * In android platform, all the graphic buffer are at least
                     * double buffered (2 or more) this buffer is already rendered.
                     * It is the redundant src buffer for FIMC rendering.
                     */

#if defined(BOARD_USES_HDMI)
                    skip_hdmi_rendering = 1;
#endif
                    continue;
                }
                ctx->layer_prev_buf[i] = (uint32_t)cur->handle;
                // initialize the src & dist context for fimc
                set_src_dst_img_rect(cur, ctx, win, &src_img, &dst_img,
                                &src_work_rect, &dst_work_rect, i);

                ret = runFimc(ctx,
                            &src_img, &src_work_rect,
                            &dst_img, &dst_work_rect,
                            cur->transform);

                if (ret < 0) {
                    SEC_HWC_Log(HWC_LOG_ERROR, "%s::runFimc fail : ret=%d\n",
                                __func__, ret);
                    skipped_window_mask |= (1 << i);
                    continue;
                }

                window_pan_display(win);

                win->buf_index = (win->buf_index + 1) % NUM_OF_WIN_BUF;
                if (win->power_state == 0)
                    window_show(win);
            } else {
                SEC_HWC_Log(HWC_LOG_ERROR,
                        "%s:: error : layer %d compositionType should have been"
                        " HWC_OVERLAY ", __func__, win->layer_index);
                skipped_window_mask |= (1 << i);
                continue;
            }
        } else {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s:: error : window status should have "
                    "been HWC_WIN_RESERVED by now... ", __func__);
             skipped_window_mask |= (1 << i);
             continue;
        }
    }

    if (skipped_window_mask) {
        //turn off the free windows
        for (int i = 0; i < NUM_HW_WINDOWS; i++) {
            if (skipped_window_mask & (1 << i)) {
                window_hide(&ctx->win[i]);
                reset_win_rect_info(&ctx->win[i]);
            }
        }
    }

    if (need_swap_buffers) {
#ifdef CHECK_EGL_FPS
        check_fps();
#endif
#ifdef HWC_HWOVERLAY
        unsigned char pixels[4];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
        EGLBoolean sucess = eglSwapBuffers((EGLDisplay)list->dpy, (EGLSurface)list->sur);
        if (!sucess)
            return HWC_EGL_ERROR;
    }

#if defined(BOARD_USES_HDMI)
    android::SecHdmiClient *mHdmiClient = android::SecHdmiClient::getInstance();

    if (skip_hdmi_rendering == 1)
        return 0;

    if (list == NULL) {
        // Don't display unnecessary image
        mHdmiClient->setHdmiEnable(0);
        return 0;
    } else {
        mHdmiClient->setHdmiEnable(1);
    }

#ifdef SUPPORT_AUTO_UI_ROTATE
    cur = &list->hwLayers[0];

    if (cur->transform == HAL_TRANSFORM_ROT_90 || cur->transform == HAL_TRANSFORM_ROT_270)
        mHdmiClient->setHdmiRotate(270, ctx->num_of_hwc_layer);
    else
        mHdmiClient->setHdmiRotate(0, ctx->num_of_hwc_layer);
#endif

    // To support S3D video playback (automatic TV mode change to 3D mode)
    if (ctx->num_of_hwc_layer == 1) {
        if (src_img.usage != prev_usage)
            mHdmiClient->setHdmiResolution(DEFAULT_HDMI_RESOLUTION_VALUE, android::SecHdmiClient::HDMI_2D);    // V4L2_STD_1080P_60

        if ((src_img.usage & GRALLOC_USAGE_PRIVATE_SBS_LR) ||
            (src_img.usage & GRALLOC_USAGE_PRIVATE_SBS_RL))
            mHdmiClient->setHdmiResolution(DEFAULT_HDMI_S3D_SBS_RESOLUTION_VALUE, android::SecHdmiClient::HDMI_S3D_SBS);    // V4L2_STD_TVOUT_720P_60_SBS_HALF
        else if ((src_img.usage & GRALLOC_USAGE_PRIVATE_TB_LR) ||
            (src_img.usage & GRALLOC_USAGE_PRIVATE_TB_RL))
            mHdmiClient->setHdmiResolution(DEFAULT_HDMI_S3D_TB_RESOLUTION_VALUE, android::SecHdmiClient::HDMI_S3D_TB);    // V4L2_STD_TVOUT_1080P_24_TB

        prev_usage = src_img.usage;
    } else {
        if ((prev_usage & GRALLOC_USAGE_PRIVATE_SBS_LR) ||
            (prev_usage & GRALLOC_USAGE_PRIVATE_SBS_RL) ||
            (prev_usage & GRALLOC_USAGE_PRIVATE_TB_LR) ||
            (prev_usage & GRALLOC_USAGE_PRIVATE_TB_RL))
            mHdmiClient->setHdmiResolution(DEFAULT_HDMI_RESOLUTION_VALUE, android::SecHdmiClient::HDMI_2D);    // V4L2_STD_1080P_60

        prev_usage = 0;
    }

    if (ctx->num_of_hwc_layer == 1) {
        if ((src_img.format == HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED)||
                (src_img.format == HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP)) {
            ADDRS * addr = (ADDRS *)(src_img.base);
            mHdmiClient->blit2Hdmi(src_work_rect.w, src_work_rect.h,
                                    src_img.format,
                                    (unsigned int)addr->addr_y, (unsigned int)addr->addr_cbcr, (unsigned int)addr->addr_cbcr,
                                    0, 0,
                                    android::SecHdmiClient::HDMI_MODE_VIDEO,
                                    ctx->num_of_hwc_layer);
        } else if ((src_img.format == HAL_PIXEL_FORMAT_YCbCr_420_SP) ||
                    (src_img.format == HAL_PIXEL_FORMAT_YCrCb_420_SP) ||
                    (src_img.format == HAL_PIXEL_FORMAT_YCbCr_420_P) ||
                    (src_img.format == HAL_PIXEL_FORMAT_YV12)) {
            mHdmiClient->blit2Hdmi(src_work_rect.w, src_work_rect.h,
                                    src_img.format,
                                    (unsigned int)ctx->fimc.params.src.buf_addr_phy_rgb_y,
                                    (unsigned int)ctx->fimc.params.src.buf_addr_phy_cb,
                                    (unsigned int)ctx->fimc.params.src.buf_addr_phy_cr,
                                    0, 0,
                                    android::SecHdmiClient::HDMI_MODE_VIDEO,
                                    ctx->num_of_hwc_layer);
        } else {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s: Unsupported format = %d", __func__, src_img.format);
        }
    }
#endif

    return 0;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    ctx->procs = const_cast<hwc_procs_t *>(procs);
}

static int hwc_query(struct hwc_composer_device_1* dev,
        int what, int* value)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // stock blob do support background layer
        value[0] = 1;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = 1000000000.0 / ctx->gralloc->fps;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, int dpy,
        int event, int enabled)
{
	int val = 0, rc = 0;
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        val = !!enabled;
        rc = ioctl(ctx->global_lcd_win.fd, S3CFB_SET_VSYNC_INT, &val);
        if (rc < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s: S3CFB_SET_VSYNC_INT error", __func__);
            return -errno;
        } else if (rc != 0) {
           ctx->vsync_a6 = 1;
        }

        return 0;
    }
    return -EINVAL;
}

void hwc_fence_clear(struct hwc_display_contents_1 *display)
{
    int i = 0;

    if (!display) {
        for(i = 0; i < display->numHwLayers; i++) {
            if (display->hwLayers[i].acquireFenceFd >= 0)
                close(display->hwLayers[i].acquireFenceFd);

            display->hwLayers[i].acquireFenceFd = -1;
            display->hwLayers[i].releaseFenceFd = -1;
        }
    }
}

static void *fimc_thread(void *data)
{
    hwc_context_t * ctx = (hwc_context_t *)(data);

    while (ctx->fimc_thread_enabled)
    {
        pthread_mutex_lock(&ctx->fimc_thread_mutex);
        pthread_cond_wait(&ctx->fimc_thread_condition, &ctx->fimc_thread_mutex);
        pthread_mutex_unlock(&ctx->fimc_thread_mutex);

        waiting_fimc_csc(ctx->fimc.dev_fd);
        //TODO struct_19c0.field_c = 1;
    }

    return NULL;
}

static void *sync_merge_thread(void *data)
{
	hwc_context_t * ctx = (hwc_context_t *)(data);

    while(ctx->sync_merge_thread_enabled) {
        pthread_mutex_lock(&ctx->sync_merge_thread_mutex);

        ctx->sync_merge_thread_running = 1;

        pthread_cond_wait(&ctx->sync_merge_condition, &ctx->sync_merge_thread_mutex);
        pthread_mutex_unlock(&ctx->sync_merge_thread_mutex);
	    //TODO SurfaceMerge(ctx, str19a0.display_contents);
    }

    ctx->sync_merge_thread_running = 0;

    return NULL;
}

void sync_merge_thread_start(hwc_context_t *ctx) //createG2DMerge
{
    if (!ctx->sync_merge_thread_created) {
        ctx->sync_merge_thread_enabled = 1;

        pthread_cond_init(&ctx->sync_merge_condition, (const pthread_condattr_t *)&ctx->sync_merge_thread_created);
        pthread_mutex_init(&ctx->sync_merge_thread_mutex, NULL);

        if (pthread_create(&ctx->sync_merge_thread, NULL, sync_merge_thread, (void *)ctx) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s: Unable to create thread", __func__);
            ctx->sync_merge_thread_created = 0;
        } else {
            ctx->sync_merge_thread_created = 1;
            ctx->str1960_fieldc = -1; //retireFence?
            do
                usleep(1000);
            while(!ctx->sync_merge_thread_running);

            SEC_HWC_Log(HWC_LOG_DEBUG, "%s: sync_merge thread started", __func__);
        }
    }
}

void sync_merge_thread_stop(hwc_context_t *ctx) //destroyG2DMerge
{
    if (ctx->sync_merge_thread_created) {
        ctx->sync_merge_thread_enabled = 0;
        ctx->sync_merge_thread_running = 0;

        if (pthread_join(ctx->sync_merge_thread, NULL) < 0)
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s: pthread_join error", __func__);

        ctx->str1960_fieldc = -1; //retireFence?

        ctx->sync_merge_thread_created = 0;

        if (ctx->currentDisplay)
            free(ctx->currentDisplay);

        ctx->currentDisplay = NULL;
    }
}

static void *capture_thread(void *data)
{
	sigset_t mask = -1;

	hwc_context_t * ctx = (hwc_context_t *)(data);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    while (ctx->capture_thread_enabled) {
        //if (g2d_handle_oneshot_capture(ctx) < 0)
            SEC_HWC_Log(HWC_LOG_WARNING, "error in capture thread");
    }

    return NULL;
}

void capture_thread_start(hwc_context_t *ctx) //createG2DCapture
{
    if (!ctx->capture_thread_created) {
        ctx->capture_thread_enabled = 1;

        if ( pthread_create(&ctx->capture_thread, NULL, capture_thread, (void *)ctx) ) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s failed creating capture thread");
            ctx->capture_thread_created = 0;
        } else {
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s capture thread started");
            ctx->capture_thread_created = 1;
        }
    }
}

void capture_thread_stop(hwc_context_t *ctx) //destroyG2DCapture
{
    if (ctx->capture_thread_created) {
        ctx->capture_thread_enabled = 0;

        if (pthread_join(ctx->capture_thread, NULL) < 0)
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s: pthread_join error", __func__);

        ctx->str1960_fieldc = -1; //thread opeation ready???
        ctx->capture_thread_created = 0;
    }
}

void SyncSurfaceMerge(hwc_context_t *ctx, hwc_display_contents_1 *display)
{
    int size = 0;

    sync_merge_thread_start(ctx);

    if (ctx->currentNumHwLayers != display->numHwLayers) {
        if (ctx->currentDisplay) {
            free(ctx->currentDisplay);
            ctx->currentDisplay = NULL;
        }
    }

    ctx->currentNumHwLayers = display->numHwLayers;
    size = (display->numHwLayers * sizeof(hwc_display_contents_1)) + 20; //This 20 is the space needed to store the rest of hwc_display_contents_1

    if (!ctx->currentDisplay)
        ctx->currentDisplay = (hwc_display_contents_1*) malloc(size);

    memcpy(&ctx->currentDisplay, display, size);

    pthread_mutex_lock(&ctx->sync_merge_thread_mutex);
    pthread_cond_signal(&ctx->sync_merge_condition);
    pthread_mutex_unlock(&ctx->sync_merge_thread_mutex);
}

#ifdef SYSFS_VSYNC_NOTIFICATION
static void *hwc_vsync_sysfs_loop(void *data)
{
    static char buf[4096];
    int vsync_timestamp_fd;
    fd_set exceptfds;
    int res;
    int64_t timestamp = 0;
    hwc_context_t * ctx = (hwc_context_t *)(data);

    vsync_timestamp_fd = open("/sys/devices/platform/samsung-pd.2/s3cfb.0/vsync_time", O_RDONLY);
    char thread_name[64] = "hwcVsyncThread";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, -20);
    memset(buf, 0, sizeof(buf));

    SEC_HWC_Log(HWC_LOG_DEBUG,"Using sysfs mechanism for VSYNC notification");

    FD_ZERO(&exceptfds);
    FD_SET(vsync_timestamp_fd, &exceptfds);
    do {
        ssize_t len = read(vsync_timestamp_fd, buf, sizeof(buf));
        timestamp = strtoull(buf, NULL, 0);
        if(ctx->procs)
            ctx->procs->vsync(ctx->procs, 0, timestamp);
        select(vsync_timestamp_fd + 1, NULL, NULL, &exceptfds, NULL);
        lseek(vsync_timestamp_fd, 0, SEEK_SET);
    } while (1);

    return NULL;
}
#endif

void handle_vsync_uevent(hwc_context_t *ctx, const char *buff, int len)
{
    uint64_t timestamp = 0;
    const char *s = buff;

    if(!ctx->procs || !ctx->procs->vsync)
       return;

    s += strlen(s) + 1;

    while(*s) {
        if (!strncmp(s, "VSYNC=", strlen("VSYNC=")))
            timestamp = strtoull(s + strlen("VSYNC="), NULL, 0);

        s += strlen(s) + 1;
        if (s - buff >= len)
            break;
    }

    ctx->procs->vsync(ctx->procs, 0, timestamp);
}

static void *hwc_vsync_thread(void *data)
{
    hwc_context_t *ctx = (hwc_context_t *)(data);

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    while(ctx->vsync_thread_enabled) {
        while(ctx->vsync_thread_enabled) {
            if ((ctx->vsync_a7 != 0) && (ctx->str16a0_field8 == 0)){
                ++ctx->vsync_a8;
            }

            if (ctx->vsync_a8 > 6)
                break;

            usleep(16000);
        }
        ctx->vsync_a5 = 1;
        ctx->vsync_a6 = 1;
        ctx->vsync_a7 = 0;
        ctx->vsync_a8 = 0;

        ctx->procs->invalidate(ctx->procs);
    }

    ctx->vsync_a5 = 0;
    ctx->vsync_a6 = 0;
    ctx->vsync_a7 = 0;
    ctx->vsync_a8 = 0;

    return NULL;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int ret = 0;
    int i;
    if (ctx) {
        if (destroyFimc(&ctx->fimc) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyFimc fail", __func__);
            ret = -1;
        }

        if (window_close(&ctx->global_lcd_win) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_close() fail", __func__);
            ret = -1;
        }

        for (i = 0; i < NUM_HW_WINDOWS; i++) {
            if (window_close(&ctx->win[i]) < 0)
                SEC_HWC_Log(HWC_LOG_DEBUG, "%s::window_close() fail", __func__);
        }

        free(ctx);
    }
    return ret;
}

static int hwc_blank(struct hwc_composer_device_1 *dev, int dpy, int blank)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    // TODO: kill and restart vsync thread

    if (blank) {
        // release our resources, the screen is turning off
        // in our case, there is nothing to do.
        ctx->num_of_fb_layer_prev = 0;
        return 0;
    }
    else {
        // No need to unblank, will unblank on set()
        return 0;
    }
}

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = 0;
    int err    = 0;
    struct hwc_win_info_t   *win;
    int refreshRate = 0;
    int i = 0;
    struct fimg2d_blit  fimg_cmd;
    struct fimg2d_image fimg_tmp_img;

    if (strcmp(name, HWC_HARDWARE_COMPOSER))
        return  -EINVAL;

    struct hwc_context_t *dev;
    dev = (hwc_context_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* get gralloc module */
    if ( hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &dev->gralloc) != 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail on loading gralloc HAL");

        free(dev);
        return -EINVAL;
    }

    /* initialize the procs */
    dev->device.common.tag           = HARDWARE_DEVICE_TAG;
    dev->device.common.version       = HWC_DEVICE_API_VERSION_1_1;
    dev->device.common.module        = const_cast<hw_module_t*>(module);
    dev->device.common.close         = hwc_device_close;
    dev->device.prepare              = hwc_prepare;
    dev->device.set                  = hwc_set;
    dev->device.eventControl         = hwc_eventControl;
    dev->device.blank                = hwc_blank;
    dev->device.query                = hwc_query;
    dev->device.registerProcs        = hwc_registerProcs;
    //dev->device.dump
    dev->device.getDisplayConfigs    = hwc_getDisplayConfigs;
    dev->device.getDisplayAttributes = hwc_getDisplayAttributes;
    *device = &dev->device.common;

    dev->vsync_thread_enabled = 1;

    err = pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);
    if (err) {
        ALOGE("%s::pthread_create() failed : %s", __func__, strerror(err));
        status = -err;
        goto err;
    }

    //TODO create hdmi cable status thread

    // Initialize ION
    dev->ion_hdl.client = -1;
    dev->ion_hdl.param.client = -1;
    dev->ion_hdl.param.buffer = -1;
    dev->ion_hdl.param.size = 0;
    dev->ion_hdl.param.memory = NULL;
    dev->ion_hdl.param.physaddr = NULL;

    dev->ion_hdl.client = ion_client_create();

    dev->ion_hdl.client2 = dev->ion_hdl.client;
    dev->ion_hdl.param.client = dev->ion_hdl.client;

    dev->fimg_tmp_ion_hdl.client = -1;
    dev->fimg_tmp_ion_hdl.param.buffer = 0;
    dev->fimg_tmp_ion_hdl.param.memory = NULL;
    dev->fimg_tmp_ion_hdl.param.physaddr = NULL;

    dev->fimg_ion_hdl.param.client = dev->ion_hdl.client;
    dev->fimg_ion_hdl.param.buffer = -1;
    dev->fimg_ion_hdl.param.size = 0;
    dev->fimg_ion_hdl.param.memory = NULL;
    dev->fimg_ion_hdl.param.physaddr = NULL;

    /*
     * TODO:
     * subtitle_alloc_buffer(hwc_context_t *)
     * android::ExynosHWCService::getExynosHWCService(void)
     * android::ExynosHWCService::setExynosHWCCtx(hwc_context_t *)
     * android::ExynosIPService::getExynosIPService(void)
     */

    //TODO: memset(&str15c0 + 4,0,sizeof(?));

    dev->lcd_info.pixclock = 0;
    //dev->field_1e0 = 0;
    //dev->field_1e4 = 0;
    //dev->field_1e8 = 0;
    //dev->field_ac = 0;
    dev->str1680_field1c = 0;
    dev->str16a0_field8 = 0;
    dev->vsync_a7 = 0;
    dev->vsync_a8 = 0;

    /* open all windows */
    for (int i = 0; i < NUM_HW_WINDOWS; i++) {
        if (window_open(&(dev->win[i]), i)  < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR,
                    "%s:: Failed to open window %d device ", __func__, i);
            status = -EINVAL;
            goto err;
        }
    }

    /* query global LCD info */
    if (window_open(&dev->global_lcd_win, 2) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s:: Failed to open window %d device ", __func__, 2);
        status = -EINVAL;
        goto err;
    }

    if (window_get_global_lcd_info(dev->global_lcd_win.fd, &dev->lcd_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s::window_get_global_lcd_info is failed : %s",
                __func__, strerror(errno));
        status = -EINVAL;
        goto err;
    }

    memcpy(&dev->global_lcd_win.lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
    memcpy(&dev->global_lcd_win.var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

    refreshRate = 1000000000000LLU /
        (
            uint64_t( dev->lcd_info.upper_margin + dev->lcd_info.lower_margin + dev->lcd_info.yres)
            * ( dev->lcd_info.left_margin  + dev->lcd_info.right_margin + dev->lcd_info.xres)
            * dev->lcd_info.pixclock
        );

    if (refreshRate == 0) {
    	SEC_HWC_Log(HWC_LOG_WARNING, "%s:: Invalid refresh rate, using 60 Hz", __func__);
        refreshRate = 60;  /* 60 Hz */
    }

    dev->vsync_period = 1000000000UL / refreshRate;

    dev->width = dev->lcd_info.xres;
    dev->height = dev->lcd_info.yres;
    dev->xdpi = (dev->lcd_info.xres * 25.4f * 1000.0f) / dev->lcd_info.width;
    dev->ydpi = (dev->lcd_info.yres * 25.4f * 1000.0f) / dev->lcd_info.height;

    ALOGI("using\nxres         = %d px\nyres         = %d px\nwidth        = %d mm (%f dpi)\nheight       = %d mm (%f dpi)\nrefresh rate = %d Hz",
            dev->width, dev->height, dev->lcd_info.width, dev->xdpi, dev->lcd_info.height, dev->ydpi, refreshRate);

    dev->global_lcd_win.rect_info.x = 0;
    dev->global_lcd_win.rect_info.y = 0;
    dev->global_lcd_win.rect_info.w = dev->lcd_info.xres;
    dev->global_lcd_win.rect_info.h = dev->lcd_info.yres;

    if (window_get_info(&dev->global_lcd_win) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_info is failed : %s", __func__, strerror(errno));
        status = -EINVAL;
        goto err;
    }

    dev->global_lcd_win.size = dev->global_lcd_win.var_info.yres * dev->global_lcd_win.fix_info.line_length;
    dev->global_lcd_win.power_state = 1;

    dev->global_lcd_win.buf_index = 0;

    for(i = 0; i < NUM_OF_WIN_BUF; i++) {
        dev->global_lcd_win.addr[i] = dev->global_lcd_win.fix_info.smem_start + (dev->global_lcd_win.size * i);
        SEC_HWC_Log(HWC_LOG_ERROR, "%s:: win-%d addr[%d] = 0x%x\n", __func__, 2, i, dev->global_lcd_win.addr[i]);
    }

    dump_win(&dev->global_lcd_win);

#if defined(BOARD_USES_HDMI) //This is not in stock n7100 blob
    lcd_width   = dev->lcd_info.xres;
    lcd_height  = dev->lcd_info.yres;
#endif

    /* initialize the window context */
    for (int i = 0; i < NUM_HW_WINDOWS; i++) {
        win = &dev->win[i];
        memcpy(&win->lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
        memcpy(&win->var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

        win->rect_info.x = 0;
        win->rect_info.y = 0;
        win->rect_info.w = win->var_info.xres;
        win->rect_info.h = win->var_info.yres;

        for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
            win->secion_param[j].buffer = -1;
            win->secion_param[j].size = 0;
            win->secion_param[j].client = dev->ion_hdl.client;
            win->secion_param[j].memory = NULL;
            win->secion_param[j].physaddr = NULL;

            win->fence[j] = -1;
        }

       /* not in stock n7100 blob

        if (window_set_pos(win) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_set_pos is failed : %s",
                    __func__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        if (window_get_info(win) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_info is failed : %s",
                    __func__, strerror(errno));
            status = -EINVAL;
            goto err;
        } */
    }

    /* query fbdev1 info */
    if (window_open(&dev->fbdev1_win, 5) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s:: Failed to open window %d device ", __func__, 5);
    } else {
        if (window_get_global_lcd_info(dev->fbdev1_win.fd, &dev->fbdev1_info) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_global_lcd_info is failed : %s", __func__, strerror(errno));
        } else {
            memcpy(&dev->fbdev1_win.lcd_info, &dev->fbdev1_info, sizeof(struct fb_var_screeninfo));
            memcpy(&dev->fbdev1_win.var_info, &dev->fbdev1_info, sizeof(struct fb_var_screeninfo));

            dev->fbdev1_win.rect_info.x = 0;
            dev->fbdev1_win.rect_info.y = 0;
            dev->fbdev1_win.rect_info.w = dev->fbdev1_win.var_info.xres;
            dev->fbdev1_win.rect_info.h = dev->fbdev1_win.var_info.yres;

            //struct_1960.xres = 0;
            //struct_1960.yres = 0;
            //struct_1960.field_10 = 0;
            //struct_1960.field_1c = 0;

            dev->str1980_yres = 0;

            if (window_get_info(&dev->fbdev1_win) < 0) {
               SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_info is failed : %s", __func__, strerror(errno));
            } else {
            	if (dev->fbdev1_win.fix_info.smem_start != NULL) {
                    dev->fbdev1_win_needs_buffer = 0;
            		//struct_1980.field_19 = 0;

                    if (window_set_pos(&dev->fbdev1_win) < 0) {
                        SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_set_pos is failed : %s", __func__, strerror(errno));
                    } else {
                        dev->fbdev1_win.buf_index = 0;
                        dev->fbdev1_win.power_state = 0;

                        dev->fbdev1_win.size = dev->fbdev1_win.var_info.yres * dev->fbdev1_win.fix_info.line_length;
                        dev->fbdev1_win.addr[0] = dev->fbdev1_win.fix_info.smem_start;
                        dev->fbdev1_win.addr[1] = dev->fbdev1_win.fix_info.smem_start + dev->fbdev1_win.size;
                        dev->fbdev1_win.addr[2] = dev->fbdev1_win.fix_info.smem_start + (dev->fbdev1_win.size * 2);

                        for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
                            dev->fbdev1_win.fence[j] = -1;
                        }

                        dump_win(&dev->fbdev1_win);
                    }
            	} else {
            		dev->fbdev1_win_needs_buffer = 1;
                    //struct_1980.field_19 = 0;

                    dev->fbdev1_win.buf_index = 0;
                    dev->fbdev1_win.power_state = 0;
                    dev->fbdev1_win.addr[0] = NULL;
                    dev->fbdev1_win.addr[1] = NULL;
                    dev->fbdev1_win.addr[2] = NULL;

                    for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
                    	dev->fbdev1_win.secion_param[j].buffer = -1;
                    	dev->fbdev1_win.secion_param[j].size = 0;
                    	dev->fbdev1_win.secion_param[j].client = dev->ion_hdl.client;
                    	dev->fbdev1_win.secion_param[j].memory = NULL;
                    	dev->fbdev1_win.secion_param[j].physaddr = NULL;

                    	dev->fbdev1_win.fence[j] = -1;
                    }
            	}
            }
        }
    }

    //struct_18e0.field_0 = -1;
    //struct_18e0.field_4 = -1;
    //struct_18c0.field_1c = -1;  //this has to do with sechdmi

    //create PP
    if (createFimc(&dev->fimc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::createFimc() fail", __func__);
        status = -EINVAL;
        goto err;
    }

    dev->fimc_thread_enabled = 1;
    //TODO struct_19c0.field_c = 1;

    if ( pthread_create(&dev->fimc_thread, NULL, fimc_thread, (void *)dev) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s: Unable to create thread", __func__);
    } else {
        pthread_cond_init(&dev->fimc_thread_condition, NULL);
        pthread_mutex_init(&dev->fimc_thread_mutex, NULL);
    }

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: hwc_device_open: SUCCESS", __func__);

    struct_16c0.field_1c = 1;

    /*SecHdmiClient::getInstance();
      SecHdmiClient::getHdmiCableStatus();
      SecHdmiClient::getForceMirrorMode();
      SecHdmiClient::getS3DSupport()*/

    //Init fimg
    dev->fimg_tmp_ion_hdl.client = dev->ion_hdl.client;
    createIONMem(&dev->fimg_tmp_ion_hdl.param, 12*1024, ION_HEAP_EXYNOS_CONTIG_MASK); // Bit 23 is also active, but don't know the meaning

    memset(&dev->fimg_tmp_ion_hdl.param.memory, 0, 12*1024);

    fimg_tmp_img.addr.start = (unsigned long) dev->fimg_tmp_ion_hdl.param.physaddr + 8192;
    fimg_cmd.tmp = &fimg_tmp_img;
    fimg_cmd.seq_no = SEQ_NO_CMD_SET_DBUFFER;

    if (stretchFimgApi(&fimg_cmd) < 0) {
    	SEC_HWC_Log(HWC_LOG_ERROR, "%s: stretchFimgApi()", __func__);
    	return -1;
    }

    return 0;

err:
    if (destroyFimc(&dev->fimc) < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyFimc() fail", __func__);

    if (window_close(&dev->global_lcd_win) < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_close() fail", __func__);

    for (int i = 0; i < NUM_HW_WINDOWS; i++) {
        if (window_close(&dev->win[i]) < 0)
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s::window_close() fail", __func__);
    }

    return status;
}

static void exynos4_config_handle(private_handle_t *handle,
        hwc_rect_t &sourceCrop, hwc_rect_t &displayFrame,
        int32_t blending, int fence_fd, s3c_fb_win_config &cfg,
        struct hwc_context_t* ctx)
{
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = exynos4_format_to_bpp(handle->format);
    uint32_t offset = (sourceCrop.top * handle->stride + sourceCrop.left) * bpp / 8;

    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        ALOGV("layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
        offset += crop * bpp / 8;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > ctx->xres) {
        unsigned int crop = displayFrame.right - ctx->xres;
        ALOGV("layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    f (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        ALOGV("layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
        offset += handle->stride * crop * bpp / 8;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > ctx->yres) {
        int crop = displayFrame.bottom - ctx->yres;
        ALOGV("layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }

    cfg.state = cfg.S3C_FB_WIN_STATE_BUFFER;
    cfg.fd = handle->fd;
    cfg.x = x;
    cfg.y = y;
    cfg.w = w;
    cfg.h = h;
    cfg.format = exynos5_format_to_s3c_format(handle->format);
    cfg.offset = offset;
    cfg.stride = handle->stride * bpp / 8;
    cfg.blending = exynos4_blending_to_s3c_blending(blending);
    cfg.fence_fd = fence_fd;
}
