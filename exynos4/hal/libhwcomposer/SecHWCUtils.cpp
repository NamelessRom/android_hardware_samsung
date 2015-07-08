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

/*
 *    Revision History:
 * - 2011/03/11 : Rama, Meka(v.meka@samsung.com)
 * Initial version
 *
 * - 2011/12/07 : Jeonghee, Kim(jhhhh.kim@samsung.com)
 * Add V4L2_PIX_FMT_YUV420M V4L2_PIX_FMT_NV12M
 *
 */

#include "SecHWCUtils.h"

#define V4L2_BUF_TYPE_CAPTURE V4L2_BUF_TYPE_VIDEO_CAPTURE

//#define CHECK_FPS
#ifdef CHECK_FPS
#include <sys/time.h>
#include <unistd.h>
#define CHK_FRAME_CNT 57

void check_fps()
{
    static struct timeval tick, tick_old;
    static int total = 0;
    static int cnt = 0;
    int FPS;
    cnt++;
    gettimeofday(&tick, NULL);
    if (cnt > 10) {
        if (tick.tv_sec > tick_old.tv_sec)
            total += ((tick.tv_usec/1000) + (tick.tv_sec - tick_old.tv_sec)*1000 - (tick_old.tv_usec/1000));
        else
            total += ((tick.tv_usec - tick_old.tv_usec)/1000);

        memcpy(&tick_old, &tick, sizeof(timeval));
        if (cnt == (10 + CHK_FRAME_CNT)) {
            FPS = 1000*CHK_FRAME_CNT/total;
            ALOGE("[FPS]:%d\n", FPS);
            total = 0;
            cnt = 10;
        }
    } else {
        memcpy(&tick_old, &tick, sizeof(timeval));
        total = 0;
    }
}
#endif

struct yuv_fmt_list yuv_list[] = {
    { "V4L2_PIX_FMT_NV12",      "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12,     12, 2 },
    { "V4L2_PIX_FMT_NV12T",     "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12T,    12, 2 },
    { "V4L2_PIX_FMT_NV21",      "YUV420/2P/LSB_CRCB",   V4L2_PIX_FMT_NV21,     12, 2 },
    { "V4L2_PIX_FMT_NV21X",     "YUV420/2P/MSB_CBCR",   V4L2_PIX_FMT_NV21X,    12, 2 },
    { "V4L2_PIX_FMT_NV12X",     "YUV420/2P/MSB_CRCB",   V4L2_PIX_FMT_NV12X,    12, 2 },
    { "V4L2_PIX_FMT_YUV420",    "YUV420/3P",            V4L2_PIX_FMT_YUV420,   12, 3 },
    { "V4L2_PIX_FMT_YUYV",      "YUV422/1P/YCBYCR",     V4L2_PIX_FMT_YUYV,     16, 1 },
    { "V4L2_PIX_FMT_YVYU",      "YUV422/1P/YCRYCB",     V4L2_PIX_FMT_YVYU,     16, 1 },
    { "V4L2_PIX_FMT_UYVY",      "YUV422/1P/CBYCRY",     V4L2_PIX_FMT_UYVY,     16, 1 },
    { "V4L2_PIX_FMT_VYUY",      "YUV422/1P/CRYCBY",     V4L2_PIX_FMT_VYUY,     16, 1 },
    { "V4L2_PIX_FMT_UV12",      "YUV422/2P/LSB_CBCR",   V4L2_PIX_FMT_NV16,     16, 2 },
    { "V4L2_PIX_FMT_UV21",      "YUV422/2P/LSB_CRCB",   V4L2_PIX_FMT_NV61,     16, 2 },
    { "V4L2_PIX_FMT_UV12X",     "YUV422/2P/MSB_CBCR",   V4L2_PIX_FMT_NV16X,    16, 2 },
    { "V4L2_PIX_FMT_UV21X",     "YUV422/2P/MSB_CRCB",   V4L2_PIX_FMT_NV61X,    16, 2 },
    { "V4L2_PIX_FMT_YUV422P",   "YUV422/3P",            V4L2_PIX_FMT_YUV422P,  16, 3 },
};

void dump_win(struct hwc_win_info_t *win)
{
    int i = 0;

    SEC_HWC_Log(HWC_LOG_DEBUG, "Dump Window Information");
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->fd = %d", win->fd);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->size = %d", win->size);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->rect_info.x = %d", win->rect_info.x);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->rect_info.y = %d", win->rect_info.y);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->rect_info.w = %d", win->rect_info.w);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->rect_info.h = %d", win->rect_info.h);

    for (i = 0; i < NUM_OF_WIN_BUF; i++) {
        SEC_HWC_Log(HWC_LOG_DEBUG, "win->addr[%d] = 0x%x", win->addr[i]);
    }

    SEC_HWC_Log(HWC_LOG_DEBUG, "win->buf_index = %d", win->buf_index);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->power_state = %d", win->power_state);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->blending = %d", win->blending);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->layer_index = %d", win->layer_index);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->status = %d", win->status);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->vsync = %d", win->vsync);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->fix_info.smem_start = 0x%x", win->fix_info.smem_start);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->fix_info.line_length = %d", win->fix_info.line_length);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.xres = %d", win->var_info.xres);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.yres = %d", win->var_info.yres);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.xres_virtual = %d", win->var_info.xres_virtual);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.yres_virtual = %d", win->var_info.yres_virtual);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.xoffset = %d", win->var_info.xoffset);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->var_info.yoffset = %d", win->var_info.yoffset);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->lcd_info.xres = %d", win->lcd_info.xres);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->lcd_info.yres = %d", win->lcd_info.yres);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->lcd_info.xoffset = %d", win->lcd_info.xoffset);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->lcd_info.yoffset = %d", win->lcd_info.yoffset);
    SEC_HWC_Log(HWC_LOG_DEBUG, "win->lcd_info.bits_per_pixel = %d", win->lcd_info.bits_per_pixel);
}

int window_open(struct hwc_win_info_t *win, int id)
{
    int fd = 0;
    char name[64];
    int vsync = 1;
    int real_id;

    char const * const device_template = "/dev/graphics/fb%u";
    // window & FB maping
    // fb0 -> win-id : 2
    // fb1 -> win-id : 3
    // fb2 -> win-id : 4
    // fb3 -> win-id : 0
    // fb4 -> win_id : 1
    // it is pre assumed that ...win0 or win1 is used here..

    if (id <= NUM_HW_WINDOWS - 1)
        real_id = (id + 3) % NUM_HW_WINDOWS;
    else if (id == NUM_HW_WINDOWS)
        real_id = id;
    else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::id(%d) is weird", __func__, id);
        goto error;
    }

    snprintf(name, 64, device_template, real_id);

    win->fd = open(name, O_RDWR);
    if (win->fd <= 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Failed to open window device (%s) : %s",
                __func__, strerror(errno), name);
        goto error;
    }

    return 0;

error:
    if (0 < win->fd)
        close(win->fd);
    win->fd = 0;

    return -1;
}

int window_clear(struct hwc_context_t *ctx)
{
    struct s3c_fb_win_config_data config;

    memset(&config, 0, sizeof(struct s3c_fb_win_config_data));
    if ( ioctl(ctx->global_lcd_win.fd, S3CFB_WIN_CONFIG, &config) < 0 )
    {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_WIN_CONFIG failed to clear screen : %s", __func__, strerror(errno));
    }

    return config.fence;
}

int window_close(struct hwc_win_info_t *win)
{
    int ret = 0;

    if (0 < win->fd) {
        ret = close(win->fd);
    }
    win->fd = 0;

    return ret;
}

int window_set_addr(struct hwc_win_info_t *win)
{
    if ( ioctl(win->fd, S3CFB_EXTDSP_SET_WIN_ADDR, win->addr[win->buf_index]) < 0 ) {
    	SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_EXTDSP_SET_WIN_ADDR failed : %s", __func__, strerror(errno));
    	dump_win(win);
    	return -1;
    }

    if ( ioctl(win->fd, FBIOPAN_DISPLAY, win->var_info) < 0) {
    	SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPAN_DISPLAY(%d / %d / %d) failed : %s", __func__, win->var_info.yres,
            win->buf_index, win->var_info.yres_virtual, strerror(errno));
        dump_win(win);
        return -1;
    }

    return 0;
}

int window_set_pos(struct hwc_win_info_t *win)
{
    struct s3cfb_user_window window;

    window.x = win->rect_info.x;
    window.y = win->rect_info.y;

    if (ioctl(win->fd, S3CFB_WIN_POSITION, &window) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_WIN_POSITION(%d, %d) fail",
            __func__, window.x, window.y);
        dump_win(win);
        return -1;
    }

    /*SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: x(%d), y(%d)",
            __func__, win->rect_info.x, win->rect_info.y);*/

    win->var_info.xres_virtual = EXYNOS4_ALIGN(win->rect_info.w, 16);
    win->var_info.yres_virtual = win->rect_info.h * NUM_OF_WIN_BUF;
    win->var_info.xres = win->rect_info.w;
    win->var_info.yres = win->rect_info.h;

    win->var_info.activate &= ~FB_ACTIVATE_MASK;
    win->var_info.activate |= FB_ACTIVATE_FORCE;

    if (ioctl(win->fd, FBIOPUT_VSCREENINFO, &(win->var_info)) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPUT_VSCREENINFO(%d, %d) fail",
          __func__, win->rect_info.w, win->rect_info.h);
        dump_win(win);
        return -1;
    }

    return 0;
}

int window_get_info(struct hwc_win_info_t *win)
{
    if (ioctl(win->fd, FBIOGET_FSCREENINFO, &win->fix_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "FBIOGET_FSCREENINFO failed : %s",
            strerror(errno));

        dump_win(win);
        win->fix_info.smem_start = NULL;
        return -1;
    }

    return 0;
}

int window_pan_display(struct hwc_win_info_t *win)
{
    struct fb_var_screeninfo *lcd_info = &(win->lcd_info);

    lcd_info->yoffset = lcd_info->yres * win->buf_index;

    if (ioctl(win->fd, FBIOPAN_DISPLAY, lcd_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPAN_DISPLAY(%d / %d / %d) fail(%s)",
            __func__,
            lcd_info->yres,
            win->buf_index, lcd_info->yres_virtual,
            strerror(errno));
        return -1;
    }
    return 0;
}

int window_show(struct hwc_win_info_t *win)
{
    if (win->power_state == 0) {
        if (ioctl(win->fd, S3CFB_SET_WIN_ON, (unsigned int) 0) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_SET_WIN_ON failed : (%d:%s)",
                __func__, win->fd, strerror(errno));
            dump_win(win);
            return -1;
        }
        win->power_state = 1;
    }
    return 0;
}

int window_hide(struct hwc_win_info_t *win)
{
    if (win->power_state == 1) {
        if (ioctl(win->fd, S3CFB_SET_WIN_OFF, (unsigned int) 0) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_SET_WIN_OFF failed : (%d:%s)",
             __func__, win->fd, strerror(errno));
            dump_win(win);
            return -1;
        }
        win->power_state = 0;
    }
    return 0;
}

int window_power_on(struct hwc_win_info_t *win)
{
    if (win->power_state == 0) {
        if (ioctl(win->fd, FBIOBLANK, FB_BLANK_UNBLANK) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOBLANK failed : (%d:%s)",
                __func__, win->fd, strerror(errno));
            dump_win(win);
            return -1;
        }
        win->power_state = 1;
    }
    return 0;
}

int window_power_down(struct hwc_win_info_t *win)
{
    if (win->power_state == 1) {
    	if (ioctl(win->fd, FBIOBLANK, FB_BLANK_POWERDOWN) < 0) {
             SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOBLANK failed : (%d:%s)",
             __func__, win->fd, strerror(errno));
            dump_win(win);
            return -1;
        }
        win->power_state = 0;
    }
    return 0;
}

int window_get_global_lcd_info(int fd, struct fb_var_screeninfo *lcd_info)
{
    if (ioctl(fd, FBIOGET_VSCREENINFO, lcd_info) < 0) {
         SEC_HWC_Log(HWC_LOG_ERROR, "FBIOGET_VSCREENINFO failed : %s",
                 strerror(errno));
         return -1;
     }

     return 0;
 }

int window_buffer_allocate(struct hwc_context_t *ctx, struct hwc_win_info_t *win, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    int rc = 0;
    fb_var_screeninfo vinfo;
    int i, size;

    //SEC_HWC_Log(HWC_LOG_ERROR, "%s win->fd=%d ctx->ionclient=%d", __func__, win->fd, ctx->ionclient);

    rc = window_get_global_lcd_info(win->fd, (fb_var_screeninfo *) &vinfo);
    if ( rc < 0 )
    {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s window_get_global_lcd_info returned error: %s", __func__, strerror(errno));
        return -1;
    }

    memcpy(&win->var_info, &vinfo, sizeof(fb_var_screeninfo));

    win->rect_info.x = x;
    win->rect_info.y = y;
    win->rect_info.w = w;
    win->rect_info.h = h;

    win->var_info.xres = w;
    win->var_info.yres = h;
    win->var_info.xres_virtual = EXYNOS4_ALIGN(w,16);
    win->var_info.yres_virtual = h * NUM_OF_WIN_BUF;

    win->size = win->fix_info.line_length * h;

    size = EXYNOS4_ALIGN(h,2) * EXYNOS4_ALIGN(w,16) * win->lcd_info.bits_per_pixel * 8;

    for (i=0; i<NUM_OF_WIN_BUF; i++) {
        win->secion_param[i].client = ctx->ion_hdl.client;

        if (!win->secion_param[i].size)
           createIONMem(&win->secion_param[i], size, ION_HEAP_EXYNOS_CONTIG_MASK); // Bit 23 is also active, but don't know the meaning

        if (win->secion_param[i].size)
            win->addr[i] = win->secion_param[i].physaddr;
        else
            win->addr[i] = NULL;

        if (win->fence[i] >= 0)
            close(win->fence[i]);

        win->fence[i] = -1;
    }

    if (window_get_info(win) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_info is failed : %s", __func__, strerror(errno));
        return -1;
    }

    win->fix_info.line_length = win->var_info.xres_virtual * win->lcd_info.bits_per_pixel * 8;
    win->size = win->var_info.yres * win->lcd_info.bits_per_pixel * 8;

    return 0;
}

int window_buffer_deallocate(struct hwc_context_t *ctx, struct hwc_win_info_t *win)
{
    unsigned int cur_win_addr = NULL;
    int i = 0, fbdev1_or_hdmi_connected = 0;

    if ((win != &ctx->fbdev1_win)
#ifdef BOARD_USES_HDMI
        || (ctx->hdmi_cable_status)
#endif
                                        ) {
        fbdev1_or_hdmi_connected = 0;
    } else {
        if ( ioctl(ctx->fbdev1_win.fd, S3CFB_GET_CUR_WIN_BUF_ADDR, &cur_win_addr) < 0 )
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_GET_CUR_WIN_BUF_ADDR failed");

        fbdev1_or_hdmi_connected = 1;
    }

    for (i=0; i<NUM_OF_WIN_BUF; i++) {
        if (win->fence[i]) {
            if (sync_wait(win->fence[i],1000) < 0)
                SEC_HWC_Log(HWC_LOG_WARNING, "%s::sync_wait error", __func__);

            close(win->fence[i]);
            win->fence[i] = -1;
        }

        if (fbdev1_or_hdmi_connected) {
            if ((win->secion_param[i].size > 0) && (win->buf_index != i) && (win->addr[i] != cur_win_addr))
                destroyIONMem(&win->secion_param[i]);
            else
                if ((win->secion_param[i].size > 0) && (win->buf_index != -1))
                    destroyIONMem(&win->secion_param[i]);
        } else {
            if (win->secion_param[i].size > 0)
                destroyIONMem(&win->secion_param[i]);
        }
    }

    return 0;
}

int get_buf_index(hwc_win_info_t *win)
{
    int index = 0;
    struct s3c_fb_win_config_data config;

    config.fence = 0;

    if (ioctl(win->fd, S3CFB_WIN_CONFIG, &config) < 0)
        index = (win->buf_index + 1) % NUM_OF_WIN_BUF;

    return index;
}

int get_user_ptr_buf_index(hwc_win_info_t *win)
{
    unsigned int locked = 0, addr = 0;
    int index = 0, i = 0;

    if ( ioctl(win->fd, S3CFB_EXTDSP_GET_LOCKED_BUFFER, &locked) >= 0 && ioctl(win->fd, S3CFB_GET_FB_PHY_ADDR, &addr) >= 0 ) {
        for( i=0; i < NUM_OF_WIN_BUF; i++ ) {
            if ( win->addr[i] != locked )
                if ( win->addr[i] != addr )
                    return i;
        }

        return 0;
    } else {
        index = (win->buf_index + 1) % NUM_OF_WIN_BUF;
    }

    return index;
}

int fimc_v4l2_set_src(int fd, unsigned int hw_ver, s5p_fimc_img_info *src)
{
    struct v4l2_format  fmt;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop    crop;
    struct v4l2_requestbuffers req;

    fmt.fmt.pix.width       = src->full_width;
    fmt.fmt.pix.height      = src->full_height;
    fmt.fmt.pix.pixelformat = src->color_space;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    fmt.type                = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::VIDIOC_S_FMT failed : errno=%d (%s)"
                " : fd=%d\n", __func__, errno, strerror(errno), fd);
        return -1;
    }

    /* crop input size */
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    crop.c.width  = src->width;
    crop.c.height = src->height;
    if (0x50 <= hw_ver) {
        crop.c.left   = src->start_x;
        crop.c.top    = src->start_y;
    } else {
        crop.c.left   = 0;
        crop.c.top    = 0;
    }

    if (ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_S_CROP :"
                "crop.c.left : (%d), crop.c.top : (%d), crop.c.width : (%d), crop.c.height : (%d)",
                __func__, crop.c.left, crop.c.top, crop.c.width, crop.c.height);
        return -1;
    }

    /* input buffer type */
    req.count       = 1;
    req.memory      = V4L2_MEMORY_USERPTR;
    req.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in VIDIOC_REQBUFS", __func__);
        return -1;
    }

    return 0;
}

int fimc_v4l2_set_dst(int fd, s5p_fimc_img_info *dst,
        int rotation, int hflip, int vflip, struct fimc_buf buffer)
{
    struct v4l2_format      sFormat;
    struct v4l2_control     vc;
    struct v4l2_framebuffer fbuf;
    int ret;

    /* set rotation configuration */
    vc.id = V4L2_CID_ROTATION;
    vc.value = rotation;

    ret = ioctl(fd, VIDIOC_S_CTRL, &vc);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s::Error in video VIDIOC_S_CTRL - rotation (%d)"
                "vc.id : (%d), vc.value : (%d)", __func__, ret, vc.id, vc.value);
        return -1;
    }

    vc.id = V4L2_CID_HFLIP;
    vc.value = hflip;

    ret = ioctl(fd, VIDIOC_S_CTRL, &vc);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s::Error in video VIDIOC_S_CTRL - hflip (%d)"
                "vc.id : (%d), vc.value : (%d)", __func__, ret, vc.id, vc.value);
        return -1;
    }

    vc.id = V4L2_CID_VFLIP;
    vc.value = vflip;

    ret = ioctl(fd, VIDIOC_S_CTRL, &vc);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s::Error in video VIDIOC_S_CTRL - vflip (%d)"
                "vc.id : (%d), vc.value : (%d)", __func__, ret, vc.id, vc.value);
        return -1;
    }

    /* set size, format & address for destination image (DMA-OUTPUT) */
    ret = ioctl(fd, VIDIOC_G_FBUF, &fbuf);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_G_FBUF (%d)", __func__, ret);
        return -1;
    }

    fbuf.base            = (void *)buffer.base[0];
    fbuf.fmt.width       = dst->full_width;
    fbuf.fmt.height      = dst->full_height;
    fbuf.fmt.pixelformat = dst->color_space;

    ret = ioctl(fd, VIDIOC_S_FBUF, &fbuf);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_S_FBUF (%d)", __func__, ret);
        return -1;
    }

    vc.id = V4L2_CID_DST_INFO;
    vc.value = (unsigned int) &buffer;

    ret = ioctl(fd, VIDIOC_S_CTRL, &vc);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_S_CTRL - dst info (%d)", __func__, ret);
        return -1;
    }

    /* set destination window */
    sFormat.type             = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    sFormat.fmt.win.w.left   = dst->start_x;
    sFormat.fmt.win.w.top    = dst->start_y;
    sFormat.fmt.win.w.width  = dst->width;
    sFormat.fmt.win.w.height = dst->height;

    ret = ioctl(fd, VIDIOC_S_FMT, &sFormat);
    if (ret < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_S_FMT (%d)", __func__, ret);
        return -1;
    }

    return 0;
}

int fimc_v4l2_queue(int fd, enum v4l2_buf_type type, struct fimc_buf *fimc_buf, int index)
{
    struct v4l2_buffer buf;
    int ret;

    buf.type   = type;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.m.userptr   = (unsigned long)fimc_buf;

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
        buf.index  = 0;
        buf.length = 0;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in VIDIOC_QBUF : (%d)", __func__, ret);
            return -1;
        }
    } else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        buf.index  = index;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in VIDIOC_QBUF : (%d)", __func__, ret);
            return -1;
        }
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Wrong type %d", __func__, type);
        return -1;
    }

    return 0;
}

int fimc_v4l2_dequeue(int fd, enum v4l2_buf_type type, struct fimc_buf *fimc_buf)
{
    struct v4l2_buffer buf;

    if ((type == V4L2_BUF_TYPE_VIDEO_OUTPUT) || (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.type   = type;

        if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            buf.m.userptr = (unsigned long) fimc_buf;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in VIDIOC_DQBUF", __func__);
            return -1;
        }
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Wrong type %d", __func__, type);
        return -1;
    }

    return buf.index;
}

int fimc_v4l2_stream_off(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_STREAMOFF\n");
        return -1;
    }

    return 0;
}

int fimc_v4l2_stream_on(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_STREAMON\n");
        return -1;
    }

    return 0;
}

int fimc_v4l2_clr_buf(int fd, enum v4l2_buf_type type)
{
    struct v4l2_requestbuffers req;

    if ((type == V4L2_BUF_TYPE_VIDEO_OUTPUT) || (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        req.count  = 0;
        req.memory = V4L2_MEMORY_USERPTR;
        req.type   = type;

        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_REQBUFS");
        }
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Wrong type %d", __func__, type);
        return -1;
    }

    return 0;
}

/* This function is called by a function not called at all, so there is no need to implement it
int fimc_poll(struct pollfd *fds)
{
    int rc = 0;

    rc = poll(fds, 1, 50000000);
    if ( rc >= 0 )
    {
        if ( !rc ) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::No data in 5 seconds", __func__);
            return rc;
        }
    } else if (errno != EINTR ) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Poll error", __func__);
        return rc;
    }

    return rc;
} */

int fimc_handle_oneshot(int fd, struct fimc_buf *fimc_src_buf, struct fimc_buf *fimc_dst_buf)
{
    if (fimc_v4l2_stream_on(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : SRC v4l2_stream_on()");
        return -1;
    }

    if (fimc_v4l2_queue(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT, fimc_src_buf, 0) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : SRC v4l2_queue()");
    }

    return 0;
}

static int memcpy_rect(void *dst, void *src, int fullW, int fullH, int realW, int realH, int format)
{
    unsigned char *srcCb, *srcCr;
    unsigned char *dstCb, *dstCr;
    unsigned char *srcY, *dstY;
    int srcCbOffset, srcCrOffset;
    int dstCbOffset, dstFrameOffset, dstCrOffset;
    int cbFullW, cbRealW, cbFullH, cbRealH;
    int ySrcFW, ySrcFH, ySrcRW, ySrcRH;
    int planes;
    int i;

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "++memcpy_rect()::"
            "dst(0x%x),src(0x%x),f.w(%d),f.h(%d),r.w(%d),r.h(%d),format(0x%x)",
            (unsigned int)dst, (unsigned int)src, fullW, fullH, realW, realH, format);

// Set dst Y, Cb, Cr address for FIMC
    {
        cbFullW = fullW >> 1;
        cbRealW = realW >> 1;
        cbFullH = fullH >> 1;
        cbRealH = realH >> 1;
        dstFrameOffset = fullW * fullH;
        dstCrOffset = cbFullW * cbFullH;
        dstY = (unsigned char *)dst;
        dstCb = (unsigned char *)dst + dstFrameOffset;
        dstCr = (unsigned char *)dstCb + dstCrOffset;
    }

// Get src Y, Cb, Cr address for source buffer.
// Each address is aligned by 16's multiple for GPU both width and height.
    {
        ySrcFW = fullW;
        ySrcFH = fullH;
        ySrcRW = realW;
        ySrcRH = realH;
        srcCbOffset = EXYNOS4_ALIGN(ySrcRW,16)* EXYNOS4_ALIGN(ySrcRH,16);
        srcCrOffset = EXYNOS4_ALIGN(cbRealW,16)* EXYNOS4_ALIGN(cbRealH,16);
        srcY =  (unsigned char *)src;
        srcCb = (unsigned char *)src + srcCbOffset;
        srcCr = (unsigned char *)srcCb + srcCrOffset;
    }
    SEC_HWC_Log(HWC_LOG_DEBUG,
            "--memcpy_rect()::\n"
            "dstY(0x%x),dstCb(0x%x),dstCr(0x%x) \n"
            "srcY(0x%x),srcCb(0x%x),srcCr(0x%x) \n"
            "cbRealW(%d),cbRealH(%d)",
            (unsigned int)dstY,(unsigned int)dstCb,(unsigned int)dstCr,
            (unsigned int)srcY,(unsigned int)srcCb,(unsigned int)srcCr,
            cbRealW, cbRealH);

    if (format == HAL_PIXEL_FORMAT_YV12) { //YV12(Y,Cr,Cv)
        planes = 3;
//This is code for VE, deleted temporory by SSONG 2011.09.22
// This will be enabled later.
/*
        //as defined in hardware.h, cb & cr full_width should be aligned to 16. ALIGN(y_stride/2, 16).
        ////Alignment is hard coded to 16.
        ////for example...check frameworks/media/libvideoeditor/lvpp/VideoEditorTools.cpp file for UV stride cal
        cbSrcFW = (cbSrcFW + 15) & (~15);
        srcCbOffset = ySrcFW * fullH;
        srcCrOffset = srcCbOffset + ((cbSrcFW * fullH) >> 1);
        srcY =  (unsigned char *)src;
        srcCb = (unsigned char *)src + srcCbOffset;
        srcCr = (unsigned char *)src + srcCrOffset;
*/
    } else if ((format == HAL_PIXEL_FORMAT_YCbCr_420_P)) {
        planes = 3;
    } else if (format == HAL_PIXEL_FORMAT_YCbCr_420_SP || format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
        planes = 2;
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "use default memcpy instead of memcpy_rect");
        return -1;
    }
//#define CHECK_PERF
#ifdef CHECK_PERF
    struct timeval start, end;
    gettimeofday(&start, NULL);
#endif
    for (i = 0; i < realH; i++)
        memcpy(dstY + fullW * i, srcY + ySrcFW * i, ySrcRW);
    if (planes == 2) {
        for (i = 0; i < cbRealH; i++)
            memcpy(dstCb + ySrcFW * i, srcCb + ySrcFW * i, ySrcRW);
    } else if (planes == 3) {
        for (i = 0; i < cbRealH; i++)
            memcpy(dstCb + cbFullW * i, srcCb + cbFullW * i, cbRealW);
        for (i = 0; i < cbRealH; i++)
            memcpy(dstCr + cbFullW * i, srcCr + cbFullW * i, cbRealW);
    }
#ifdef CHECK_PERF
    gettimeofday(&end, NULL);
    SEC_HWC_Log(HWC_LOG_ERROR, "[COPY]=%d,",(end.tv_sec - start.tv_sec)*1000+(end.tv_usec - start.tv_usec)/1000);
#endif

    return 0;
}

/*****************************************************************************/
static int get_dst_phys_addr(struct hwc_context_t *ctx,
        sec_img *dst_img, sec_rect *dst_rect)
{
    s5p_fimc_t *fimc = &ctx->fimc;

    fimc->params.dst.buf_addr_phy_rgb_y = dst_img->paddr;
    fimc->params.dst.buf_addr_phy_cb = dst_img->paddr + dst_img->uoffset;
    fimc->params.dst.buf_addr_phy_cr = dst_img->paddr + dst_img->uoffset + dst_img->voffset;

    return dst_img->base;
}

static int get_src_phys_addr(struct hwc_context_t *ctx,
        sec_img *src_img, sec_rect *src_rect)
{
    s5p_fimc_t *fimc = &ctx->fimc;

    unsigned int src_virt_addr  = 0;
    unsigned int src_phys_addr  = 0;
    unsigned int src_frame_size = 0;

    ADDRS * addr;

    // error check routine
    if (0 == src_img->base && !(src_img->usage & GRALLOC_USAGE_HW_ION)) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s invalid src image base\n", __func__);
        return 0;
    }

    switch (src_img->mem_type) {
    case HWC_PHYS_MEM_TYPE:
        src_phys_addr = src_img->base + src_img->offset;
        break;

    case HWC_VIRT_MEM_TYPE:
    case HWC_UNKNOWN_MEM_TYPE:
        switch (src_img->format) {
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP: //0x110
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP: //0x111
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED: //0x112
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP: //0x113
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP: //0x114
            addr = (ADDRS *)(src_img->base);
            fimc->params.src.buf_addr_phy_rgb_y = addr->addr_y;
            fimc->params.src.buf_addr_phy_cb    = addr->addr_cbcr;

            src_phys_addr = fimc->params.src.buf_addr_phy_rgb_y;
            if (0 == src_phys_addr) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s address error "
                        "(format=CUSTOM_YCbCr/YCrCb_420_SP Y-addr=0x%x "
                        "CbCr-Addr=0x%x)",
                        __func__, fimc->params.src.buf_addr_phy_rgb_y,
                        fimc->params.src.buf_addr_phy_cb);
                return 0;
            }
            break;
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I: //0x115
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I: //0x117
        case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I: //0x117
        case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I: //0x118
            addr = (ADDRS *)(src_img->base + src_img->offset);
            fimc->params.src.buf_addr_phy_rgb_y = addr->addr_y;
            src_phys_addr = fimc->params.src.buf_addr_phy_rgb_y;
            if (0 == src_phys_addr) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s address error "
                        "(format=CUSTOM_YCbCr/CbYCrY_422_I Y-addr=0x%x)",
                    __func__, fimc->params.src.buf_addr_phy_rgb_y);
                return 0;
            }
            break;
        default:
            if (src_img->usage & GRALLOC_USAGE_HW_ION) {
                fimc->params.src.buf_addr_phy_rgb_y = src_img->paddr;
                fimc->params.src.buf_addr_phy_cb = src_img->paddr + src_img->uoffset;
                fimc->params.src.buf_addr_phy_cr = src_img->paddr + src_img->uoffset + src_img->voffset;
                src_phys_addr = fimc->params.src.buf_addr_phy_rgb_y;
            } else {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::\nformat = 0x%x : Not "
                        "GRALLOC_USAGE_HW_FIMC1 can not supported\n",
                        __func__, src_img->format);
            }
            break;
        }
    }

    return src_phys_addr;
}

int formatValueHAL2G2D(unsigned int format, unsigned int *g2d_format,
        unsigned int *pixel_order, unsigned int *bpp)
{
    //TODO
    return 0;
}

int rotateValueHAL2G2D(unsigned char transform)
{
    int rotate_flag = transform & 0x7;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:  return ROT_90;
    case HAL_TRANSFORM_ROT_180: return ROT_180;
    case HAL_TRANSFORM_ROT_270: return ROT_270;
    case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90: return ORIGIN;
    case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90: return ORIGIN;
    default: return ORIGIN;
    }
}

static inline int rotateValueHAL2PP(unsigned char transform)
{
    int rotate_flag = transform & 0x7;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:  return 90;
    case HAL_TRANSFORM_ROT_180: return 180;
    case HAL_TRANSFORM_ROT_270: return 270;
    case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90: return 90;
    case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90: return 90;
    case HAL_TRANSFORM_FLIP_H: return 0;
    case HAL_TRANSFORM_FLIP_V: return 0;
    }
    return 0;
}

static inline int hflipValueHAL2PP(unsigned char transform)
{
    int flip_flag = transform & 0x7;
    switch (flip_flag) {
    case HAL_TRANSFORM_FLIP_H:
    case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90:
        return 1;
    case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90:
    case HAL_TRANSFORM_ROT_90:
    case HAL_TRANSFORM_ROT_180:
    case HAL_TRANSFORM_ROT_270:
    case HAL_TRANSFORM_FLIP_V:
        break;
    }
    return 0;
}

static inline int vflipValueHAL2PP(unsigned char transform)
{
    int flip_flag = transform & 0x7;
    switch (flip_flag) {
    case HAL_TRANSFORM_FLIP_V:
    case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90:
        return 1;
    case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90:
    case HAL_TRANSFORM_ROT_90:
    case HAL_TRANSFORM_ROT_180:
    case HAL_TRANSFORM_ROT_270:
    case HAL_TRANSFORM_FLIP_H:
        break;
    }
    return 0;
}

static inline int multipleOf2(int number)
{
    if (number % 2 == 1)
        return (number - 1);
    else
        return number;
}

static inline int multipleOf4(int number)
{
    int remain_number = number % 4;

    if (remain_number != 0)
        return (number - remain_number);
    else
        return number;
}

static inline int multipleOf8(int number)
{
    int remain_number = number % 8;

    if (remain_number != 0)
        return (number - remain_number);
    else
        return number;
}

static inline int multipleOf16(int number)
{
    int remain_number = number % 16;

    if (remain_number != 0)
        return (number - remain_number);
    else
        return number;
}

static inline int widthOfPP(unsigned int ver, int pp_color_format, int number)
{
    if (0x50 <= ver) {
        switch (pp_color_format) {
        /* 422 1/2/3 plane */
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUV422P:

        /* 420 2/3 plane */
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
        case V4L2_PIX_FMT_YUV420:
            return multipleOf2(number);

        default :
            return number;
        }
    } else {
        switch (pp_color_format) {
        case V4L2_PIX_FMT_RGB565:
            return multipleOf8(number);

        case V4L2_PIX_FMT_RGB32:
            return multipleOf4(number);

        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            return multipleOf4(number);

        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
            return multipleOf8(number);

        case V4L2_PIX_FMT_YUV422P:
            return multipleOf16(number);

        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
            return multipleOf8(number);

        case V4L2_PIX_FMT_YUV420:
            return multipleOf16(number);

        default :
            return number;
        }
    }
    return number;
}

static inline int heightOfPP(int pp_color_format, int number)
{
    switch (pp_color_format) {
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12T:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
        return multipleOf2(number);

    default :
        return number;
        break;
    }
    return number;
}

static unsigned int get_yuv_bpp(unsigned int fmt)
{
    int i, sel = -1;

    for (i = 0; i < (int)(sizeof(yuv_list) / sizeof(struct yuv_fmt_list)); i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].bpp;
}

static unsigned int get_yuv_planes(unsigned int fmt)
{
    int i, sel = -1;

    for (i = 0; i < (int)(sizeof(yuv_list) / sizeof(struct yuv_fmt_list)); i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].planes;
}

void sub_29B4(struct hwc_win_info_t *win0, struct hwc_win_info_t *win1, sec_img *src_img, sec_img *dst_img,
                sec_rect *src_rect, sec_rect *dst_rect, int rotation)
{
	unsigned int yresrel = 0;

    v7 = a4;
    v8 = a1;
    v9 = a2;

    src_img->f_w = win0->lcd_info.xres;
    src_img->f_h = win0->lcd_info.yres;
    src_img->w = win0->var_info.xres;
    src_img->h = win0->var_info.yres;

    src_img->base = 0;
    src_img->offset = 0;
    src_img->mem_id = 0;

    src_img->usage = 0;
    src_img->uoffset = 0;
    src_img->paddr = win0->addr[win0->buf_index];
    src_img->voffset = 0;
    src_img->mem_type = HWC_PHYS_MEM_TYPE;

    src_rect->x = 0;
    src_rect->y = 0;
    src_rect->w = win0->var_info.xres;
    src_rect->h = win0->var_info.yres;

    yresrel = win1->lcd_info->yres / win0->lcd_info->yres;

    //TODO: find constants for these rotation literals
    switch(rotation) {
    case 0:
        /*v14 = win0->lcd_info->xres * win1->lcd_info->yres / win0->lcd_info->yres;

        dst_rect->x = (win0->rect_info.x * v14 / win0->lcd_info->xres)
                       + ((win0->lcd_info.xres - v14) / 2);
    	v15 = win0->rect_info.y * win1->lcd_info->yres / win0->lcd_info->yres;
        v14 = win0->lcd_info->xres * yresrel;
        dst_rect->w = win0->var_info.xres * v14 / win0->lcd_info.xres;*/

    	dst_rect->x = (win0->rect_info.x * yresrel)
    	               + ((win0->lcd_info.xres - (win0->lcd_info->xres * yresrel)) / 2);
    	dst_rect->y = win0->rect_info.y * yresrel;
    	dst_rect->w = win0->var_info.xres * yresrel;
        dst_rect->h = win0->var_info.yres * yresrel;

        dst_img->f_w = win1->var_info.xres;
        dst_img->f_h = win1->var_info.yres;
        dst_img->w = dst_rect->w;
        dst_img->h = dst_rect->h;
        break;

    case 3u: //HAL_TRANSFORM_ROT_90 or ROT_270 (from sec_g2d_4x.h) ????
        /*v18 = win0->lcd_info.xres * win1->lcd_info.yres / win0->lcd_info.yres;
        dst_rect->x = v18 + ((win1->lcd_info.xres - v18) / 2)
                      - (v18 * (win0->rect_info.x + win0->var_info.xres) / win0->lcd_info.xres);
        dst_rect->y = win1->lcd_info.yres
                      - ( (win1->lcd_info.yres * (win0->var_info.yres + win0->rect_info.y))
                          / win0->lcd_info.yres);
        dst_rect->w = win0->var_info.xres * v18 / win0->lcd_info.xres;
        dst_rect->h = win0->var_info.yres * win1->lcd_info.yres / win0->lcd_info.yres;*/

        v18 = win0->lcd_info.xres * yresrel;
        dst_rect->x = v18 + ((win1->lcd_info.xres - v18) / 2)
    	                      - (v18 * (win0->rect_info.x + win0->var_info.xres) / win0->lcd_info.xres);
        dst_rect->y = win1->lcd_info.yres
                      - ( (win1->lcd_info.yres * (win0->var_info.yres + win0->rect_info.y))
                          / win0->lcd_info.yres);
        dst_rect->w = win0->var_info.xres * v18 / win0->lcd_info.xres;
        dst_rect->h = win0->var_info.yres * win1->lcd_info.yres / win0->lcd_info.yres;

        dst_img->f_w = win1->var_info.xres;
        dst_img->f_h = win1->var_info.yres;
        dst_img->w = dst_rect->w;
        dst_img->h = dst_rect->h;
        break;

    case 4u:
        if (win0->lcd_info.yres * win1->lcd_info.yres >= win0->lcd_info.xres * win1->lcd_info.xres) {
            v18 = win1->lcd_info.xres;
            v17 = win0->lcd_info.xres * win1->lcd_info.xres / win0->lcd_info.yres; //r9
            v25 = (win1->lcd_info.yres - v17) / 2; //r3
        } else {
            v25 = 0; //r3
            v17 = win0->lcd_info.xres; //r9
            v18 = win0->lcd_info.yres * win1->lcd_info.yres / win0->lcd_info.xres;//r8
        }

        dst_rect->x = v18 + ((win1->lcd_info.xres - v18) / 2)
                      - ( (win0->rect_info.x + win0->var_info.xres) * v18 / win0->lcd_info.xres);

        dst_rect->y = (win0->rect_info.y * v17 / win0->lcd_info.yres) + v25;
        dst_rect->w = win0->var_info.xres * v18 / win0->lcd_info.xres;
        dst_rect->h = win0->var_info.yres * v17 / win0->lcd_info.yres

        dst_img->f_w = win1->var_info.xres;
        dst_img->f_h = win1->var_info.yres;
        dst_img->w = dst_rect->w;
        dst_img->h = dst_rect->h;
        break;

    case 7u:
        if (win0->lcd_info->yres * win1->lcd_info->yres >= win0->lcd_info->xres * win1->lcd_info->xres) {
            v14 = win1->lcd_info->xres; //r9
            v34 = 0; //r2
            v13 = win0->lcd_info->xres * win1->lcd_info->xres / win0->lcd_info->yres; //r8
            v33 = (win1->lcd_info->yres - v13) / 2; //r11
        } else {
            v13 = win1->lcd_info->xres;
            v33 = 0;
            v14 = win0->lcd_info->yres * win1->lcd_info->yres / win0->lcd_info->xres;
            v34 = (win1->lcd_info->xres - (v14)) / 2;
        }

        dst_rect->x = (win0->rect_info.x * v14 / win0->lcd_info->xres) + v34;
                v15 = *(_DWORD *)(v8 + 0x1C) * v13 / *(_DWORD *)(v8 + 0x184) + v33;
LABEL_14:
                dst_rect->y = v15;
                v7 = win1->var_info.xres;
                dst_img->f_h = win1->var_info.yres;

                dst_rect->w =
                dst_img->w = win0->var_info.xres * v14 / win0->lcd_info.xres;

                v27 = win0->var_info.yres * v13;
LABEL_15:

                dst_rect->h =
                dst_img->h = v27 / win0->lcd_info.yres;
                break;
            default:
                break;
        }

    dst_img->format = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP;
    dst_img->base = 0;
    dst_img->offset = 0;
    dst_img->mem_id = 0;
    dst_img->uoffset = EXYNOS4_ALIGN( EXYNOS4_ALIGN(dst_img->f_w, 16) * EXYNOS4_ALIGN(dst_img->f_h, 16), 8192);
    dst_img->mem_type = HWC_PHYS_MEM_TYPE;
    dst_img->paddr = win1->addr[win1->buf_index];
}

void rect_from_layer_get(struct hwc_context_t *ctx, hwc_layer_1_t *cur, int *left, int *top, int *right, int *bottom)
{
    *left = cur->displayFrame.left;
    *top = cur->displayFrame.top;
    *right = cur->displayFrame.right - cur->displayFrame.left;
    *bottom = cur->displayFrame.bottom - cur->displayFrame.top;

    if (cur->displayFrame.left < 0) {
        *left = 0;
        *right += cur->displayFrame.left;
    }

    if (cur->displayFrame.right > ctx->hdmi_xres)
        *right -= cur->displayFrame.right - ctx->hdmi_xres;

    if (cur->displayFrame.top < 0) {
        *top = 0;
        *bottom += cur->displayFrame.top;
    }

    if (cur->displayFrame.bottom > ctx->hdmi_yres)
        *bottom -= cur->displayFrame.bottom - ctx->hdmi_yres;
}

int waiting_fimc_csc(int fd)
{
    int result;

    if ( fimc_v4l2_dequeue(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT, NULL) < 0 )
        SEC_HWC_Log(HWC_LOG_WARNING, "%s fimc_v4l2_dequeue() error", __func__);

    if ( fimc_v4l2_stream_off(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT) >= 0 )
        result = 0;
    else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s fimc_v4l2_stream_off()", __func__);
        result = -1;
    }
    result = rotateValueHAL2G2D(result);
    return result;
}

static int runFimcCore(struct hwc_context_t *ctx,
        unsigned int src_phys_addr, sec_img *src_img, sec_rect *src_rect,
        uint32_t src_color_space,
        unsigned int dst_phys_addr, sec_img *dst_img, sec_rect *dst_rect,
        uint32_t dst_color_space, int transform)
{
    s5p_fimc_t        * fimc = &ctx->fimc;
    s5p_fimc_params_t * params = &(fimc->params);
    struct fimc_buf fimc_dst_buf;

    struct fimc_buf fimc_src_buf;
    int src_bpp, src_planes;

    unsigned int    frame_size = 0;

    bool src_cbcr_order = true;
    int rotate_value = rotateValueHAL2PP(transform);
    int hflip = 0;
    int vflip = 0;

    /* 1. param(fimc config)->src information
     *    - src_img,src_rect => s_fw,s_fh,s_w,s_h,s_x,s_y
     */
    params->src.full_width  = src_img->f_w;
    params->src.full_height = src_img->f_h;
    params->src.width       = src_rect->w;
    params->src.height      = src_rect->h;
    params->src.start_x     = src_rect->x;
    params->src.start_y     = src_rect->y;
    params->src.color_space = src_color_space;
    params->src.buf_addr_phy_rgb_y = src_phys_addr;

    /* check src minimum */
    if (multipleOf2(src_rect->w) || src_rect->w < 16 || src_rect->h < 8) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s src size is not supported by fimc : f_w=%d f_h=%d "
                "x=%d y=%d w=%d h=%d (ow=%d oh=%d) format=0x%x", __func__,
                params->src.full_width, params->src.full_height,
                params->src.start_x, params->src.start_y,
                params->src.width, params->src.height,
                src_rect->w, src_rect->h,
                params->src.color_space);
        return -1;
    }

    /* 2. param(fimc config)->dst information
     *    - dst_img,dst_rect,rot => d_fw,d_fh,d_w,d_h,d_x,d_y
     */
    switch (rotate_value) {
    case 0:
        hflip = hflipValueHAL2PP(transform);
        vflip = vflipValueHAL2PP(transform);
        params->dst.full_width  = dst_img->f_w;
        params->dst.full_height = dst_img->f_h;

        params->dst.start_x     = dst_rect->x;
        params->dst.start_y     = dst_rect->y;

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);
        params->dst.height      = heightOfPP(dst_color_space, dst_rect->h);
        break;
    case 90:
        hflip = vflipValueHAL2PP(transform);
        vflip = hflipValueHAL2PP(transform);
        params->dst.full_width  = dst_img->f_h;
        params->dst.full_height = dst_img->f_w;

        params->dst.start_x     = dst_rect->y;
        params->dst.start_y     = dst_img->f_w - (dst_rect->x + dst_rect->w);

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->h);
        params->dst.height      =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);

        if (0x50 > fimc->hw_ver)
            params->dst.start_y     += (dst_rect->w - params->dst.height);
        break;
    case 180:
        params->dst.full_width  = dst_img->f_w;
        params->dst.full_height = dst_img->f_h;

        params->dst.start_x     = dst_img->f_w - (dst_rect->x + dst_rect->w);
        params->dst.start_y     = dst_img->f_h - (dst_rect->y + dst_rect->h);

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);
        params->dst.height      = heightOfPP(dst_color_space, dst_rect->h);
        break;
    case 270:
        params->dst.full_width  = dst_img->f_h;
        params->dst.full_height = dst_img->f_w;

        params->dst.start_x     = dst_img->f_h - (dst_rect->y + dst_rect->h);
        params->dst.start_y     = dst_rect->x;

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->h);
        params->dst.height      =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);

        if (0x50 > fimc->hw_ver)
            params->dst.start_y += (dst_rect->w - params->dst.height);
        break;
    }
    params->dst.color_space = dst_color_space;

    SEC_HWC_Log(HWC_LOG_DEBUG,
            "runFimcCore()::"
            "SRC f.w(%d),f.h(%d),x(%d),y(%d),w(%d),h(%d)=>"
            "DST f.w(%d),f.h(%d),x(%d),y(%d),w(%d),h(%d)",
            params->src.full_width, params->src.full_height,
            params->src.start_x, params->src.start_y,
            params->src.width, params->src.height,
            params->dst.full_width, params->dst.full_height,
            params->dst.start_x, params->dst.start_y,
            params->dst.width, params->dst.height);

    /* check dst minimum */
    if (dst_rect->w  < 8 || dst_rect->h < 4) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s dst size is not supported by fimc : f_w=%d f_h=%d "
                "x=%d y=%d w=%d h=%d (ow=%d oh=%d) format=0x%x", __func__,
                params->dst.full_width, params->dst.full_height,
                params->dst.start_x, params->dst.start_y,
                params->dst.width, params->dst.height,
                dst_rect->w, dst_rect->h, params->dst.color_space);
        return -1;
    }
    /* check scaling limit
     * the scaling limie must not be more than MAX_RESIZING_RATIO_LIMIT
     */
    if (((src_rect->w > dst_rect->w) &&
                ((src_rect->w / dst_rect->w) > MAX_RESIZING_RATIO_LIMIT)) ||
        ((dst_rect->w > src_rect->w) &&
                ((dst_rect->w / src_rect->w) > MAX_RESIZING_RATIO_LIMIT))) {
        SEC_HWC_Log(HWC_LOG_ERROR,
                "%s over scaling limit : src.w=%d dst.w=%d (limit=%d)",
                __func__, src_rect->w, dst_rect->w, MAX_RESIZING_RATIO_LIMIT);
        return -1;
    }

   /* 3. Set configuration related to destination (DMA-OUT)
     *   - set input format & size
     *   - crop input size
     *   - set input buffer
     *   - set buffer type (V4L2_MEMORY_USERPTR)
     */

    /* TODO: in blob, they use a strange structure, so we'll use struct fimc_buf instead.
     * Later on we have to guess where are the addresses coming from. This is just to fix
     * the building process. */
    memset(&fimc_dst_buf, 0, sizeof(struct fimc_buf));

    fimc_dst_buf.base[0] = dst_phys_addr;
    //TODO fimc_dst_buf.base[1] =
    //TODO fimc_dst_buf.base[2] =

    if (fimc_v4l2_set_dst(fimc->dev_fd, &params->dst, rotate_value, hflip, vflip, fimc_dst_buf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "fimc_v4l2_set_dst is failed\n");
        return -1;
    }

   /* 4. Set configuration related to source (DMA-INPUT)
     *   - set input format & size
     *   - crop input size
     *   - set input buffer
     *   - set buffer type (V4L2_MEMORY_USERPTR)
     */
    if (fimc_v4l2_set_src(fimc->dev_fd, fimc->hw_ver, &params->src) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "fimc_v4l2_set_src is failed\n");
        return -1;
    }

    /* 5. Set input dma address (Y/RGB, Cb, Cr)
     *    - zero copy : mfc, camera
     */
    switch (src_img->format) {
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
        /* for video contents zero copy case */
        fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
        fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
        break;

    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_YV12:
    default:
        if (src_img->format == HAL_PIXEL_FORMAT_YV12){
            src_cbcr_order = false;
        }

        if (src_img->usage & GRALLOC_USAGE_HW_ION) {
            fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
            if (src_cbcr_order == true) {
                fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
                fimc_src_buf.base[2] = params->src.buf_addr_phy_cr;
            }
            else {
                fimc_src_buf.base[2] = params->src.buf_addr_phy_cb;
                fimc_src_buf.base[1] = params->src.buf_addr_phy_cr;
            }
            SEC_HWC_Log(HWC_LOG_DEBUG,
                    "runFimcCore - Y=0x%X, U=0x%X, V=0x%X\n",
                    fimc_src_buf.base[0], fimc_src_buf.base[1],fimc_src_buf.base[2]);
            break;
        }
    }

    /* 6. Run FIMC
     *    - stream on => queue => dequeue => stream off => clear buf
     */
    if (fimc_handle_oneshot(fimc->dev_fd, &fimc_src_buf, NULL) < 0) {
        ALOGE("fimcrun fail");
        fimc_v4l2_clr_buf(fimc->dev_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
        return -1;
    }

    return 0;
}

void clearBufferbyFimc(struct hwc_context_t *ctx, sec_img *dst_img)
{
    sec_img  src_img;
    sec_rect src_rect, dst_rect;
    int32_t      src_color_space;
    unsigned int dst_phys_addr  = 0;
    int32_t      dst_color_space;
    int          rc = 0;

    memset(&src_img, 0, sizeof(sec_img));

    src_img.f_w =
    src_img.f_h =
    src_img.w =
    src_img.h = 64;
    src_img.mem_type = HWC_PHYS_MEM_TYPE;
    src_img.format = HAL_PIXEL_FORMAT_RGB_565;
    //src_img.paddr = str1680.phys_addr;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.w = 64;
    src_rect.h = 64;

    src_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(src_img.format);

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.w = dst_img->f_w;
    dst_rect.h = dst_img->f_h;

    dst_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(dst_img->format);

    dst_phys_addr = get_dst_phys_addr(ctx, dst_img, &dst_rect);

    /* 4. FIMC: src_rect of src_img => dst_rect of dst_img */
    rc = runFimcCore(ctx, src_img.paddr, &src_img, &src_rect,
                (uint32_t)src_color_space, dst_phys_addr, dst_img, &dst_rect,
                (uint32_t)dst_color_space, 0);
    if (rc < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s rc=%d", __func__, rc);

    waiting_fimc_csc(ctx->fimc.dev_fd);
}

int createFimc(s5p_fimc_t *fimc)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_control vc;

    // open device file
    if (fimc->dev_fd <= 0)
        fimc->dev_fd = open(PP_DEVICE_DEV_NAME, O_RDWR);

    if (fimc->dev_fd <= 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Post processor open error (%d)",
                __func__, errno);
        goto err;
    }

    // check capability
    if (ioctl(fimc->dev_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "VIDIOC_QUERYCAP failed");
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%d has no streaming support", fimc->dev_fd);
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%d is no video output", fimc->dev_fd);
        goto err;
    }

    /*
     * malloc fimc_outinfo structure
     */
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(fimc->dev_fd, VIDIOC_G_FMT, &fmt) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_G_FMT", __func__);
        goto err;
    }

    vc.id = V4L2_CID_FIMC_VERSION;
    vc.value = 0;

    if (ioctl(fimc->dev_fd, VIDIOC_G_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_G_CTRL", __func__);
        goto err;
    }
    fimc->hw_ver = vc.value;

    vc.id = V4L2_CID_OVLY_MODE;
    vc.value = FIMC_OVLY_NONE_MULTI_BUF;

    if (ioctl(fimc->dev_fd, VIDIOC_S_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_S_CTRL", __func__);
        goto err;
    }

    return 0;

err:
    if (0 < fimc->dev_fd)
        close(fimc->dev_fd);
    fimc->dev_fd =0;

    return -1;
}

int destroyFimc(s5p_fimc_t *fimc)
{
    if (fimc->out_buf.virt_addr != NULL) {
        fimc->out_buf.virt_addr = NULL;
        fimc->out_buf.length = 0;
    }

    // close
    if (0 < fimc->dev_fd)
        close(fimc->dev_fd);
    fimc->dev_fd = 0;

    return 0;
}

int runFimc(struct hwc_context_t *ctx,
            struct sec_img *src_img, struct sec_rect *src_rect,
            struct sec_img *dst_img, struct sec_rect *dst_rect,
            uint32_t transform)
{
    s5p_fimc_t *  fimc = &ctx->fimc;

    unsigned int src_phys_addr  = 0;
    unsigned int dst_phys_addr  = 0;
    int          rotate_value   = 0;
    int32_t      src_color_space;
    int32_t      dst_color_space;

    /* 1. source address and size */
    src_phys_addr = get_src_phys_addr(ctx, src_img, src_rect);
    if (0 == src_phys_addr)
        return -1;

    /* 2. destination address and size */
    dst_phys_addr = get_dst_phys_addr(ctx, dst_img, dst_rect);;
    if (0 == dst_phys_addr)
        return -2;

    /* 3. check whether fimc supports the src format */
    src_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(src_img->format);
    if (0 > src_color_space)
        return -3;
    dst_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(dst_img->format);
    if (0 > dst_color_space)
        return -4;

    /* 4. FIMC: src_rect of src_img => dst_rect of dst_img */
    if (runFimcCore(ctx, src_phys_addr, src_img, src_rect,
                (uint32_t)src_color_space, dst_phys_addr, dst_img, dst_rect,
                (uint32_t)dst_color_space, transform) < 0)
        return -5;

    return 0;
}

int check_yuv_format(unsigned int color_format) {
    switch (color_format) {
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_YCbCr_422_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_I:
    case HAL_PIXEL_FORMAT_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CbYCrY_420_I:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
        return 1;
    default:
        return 0;
    }
}

int is_s3d_color_format(int format, int *hal_format, unsigned int *bpp)
{
    int result = 0;

    switch(format) {
      case 0x200:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        *bpp = 1;
        result = 1;
        break;

      case 0x201:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        *bpp = 2;
        result = 1;
        break;

      case 0x202:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        *bpp = 3;
        result = 1;
        break;

      case 0x203:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        *bpp = 4;
        result = 1;
        break;

      case 0x204:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        *bpp = 1;
        result = 1;
        break;

      case 0x205:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        *bpp = 2;
        result = 1;
        break;

      case 0x206:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        *bpp = 3;
        result = 1;
        break;

      case 0x207:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        *bpp = 4;
        result = 1;
        break;

      case 0x208:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        *bpp = 1;
        result = 1;
        break;

      case 0x209:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        *bpp = 2;
        result = 1;
        break;

      case 0x210:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        *bpp = 3;
        result = 1;
        break;

      case 0x211:
        *hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        *bpp = 4;
        result = 1;
        break;

      case 0x212:
        *hal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        *bpp = 1;
        result = 1;
        break;

      case 0x213:
        *hal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        *bpp = 2;
        result = 1;
        break;

      case 0x214:
        *hal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        *bpp = 3;
        result = 1;
        break;

      case 0x215:
        *hal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        *bpp = 4;
        result = 1;
        break;

      default:
          *hal_format = format;
          *bpp = 0;
          result = 0;
    }

    return result;
}

uint8_t exynos4_format_to_bpp(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return 32;

    case HAL_PIXEL_FORMAT_RGB_888:
        return 24;

    case HAL_PIXEL_FORMAT_RGB_565:
        return 16;

    default:
        ALOGW("unrecognized pixel format %u", format);
        return 0;
    }
}

enum s3c_fb_pixel_format exynos4_format_to_s3c_format(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return S3C_FB_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return S3C_FB_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_888:
        return S3C_FB_PIXEL_FORMAT_RGB_888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return S3C_FB_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return S3C_FB_PIXEL_FORMAT_BGRA_8888;
    default:
        return S3C_FB_PIXEL_FORMAT_MAX;
    }
}

enum s3c_fb_blending exynos4_blending_to_s3c_blending(int32_t blending)
{
    switch (blending) {
    case HWC_BLENDING_NONE:
        return S3C_FB_BLENDING_NONE;
    case HWC_BLENDING_PREMULT:
        return S3C_FB_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return S3C_FB_BLENDING_COVERAGE;
    default:
        return S3C_FB_BLENDING_MAX;
    }
}
