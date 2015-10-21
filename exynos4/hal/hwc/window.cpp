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

/*****************************************************************************/

void dump_win(struct hwc_win_info_t *win)
{
    int i = 0;

    ALOGD("Dump Window Information");
    ALOGD("win->fd = %d", win->fd);
    ALOGD("win->size = %d", win->size);
    ALOGD("win->rect_info.x = %d", win->rect_info.x);
    ALOGD("win->rect_info.y = %d", win->rect_info.y);
    ALOGD("win->rect_info.w = %d", win->rect_info.w);
    ALOGD("win->rect_info.h = %d", win->rect_info.h);

    for (i = 0; i < NUM_OF_WIN_BUF; i++) {
        ALOGD("win->addr[%d] = 0x%x", i, win->addr[i]);
    }

    ALOGD("win->buf_index = %d", win->buf_index);
    ALOGD("win->power_state = %d", win->power_state);
    ALOGD("win->blending = %d", win->blending);
    ALOGD("win->layer_index = %d", win->layer_index);
    ALOGD("win->status = %d", win->status);
    ALOGD("win->vsync = %d", win->vsync);
    ALOGD("win->fix_info.smem_start = 0x%x", win->fix_info.smem_start);
    ALOGD("win->fix_info.line_length = %d", win->fix_info.line_length);
    ALOGD("win->var_info.xres = %d", win->var_info.xres);
    ALOGD("win->var_info.yres = %d", win->var_info.yres);
    ALOGD("win->var_info.xres_virtual = %d", win->var_info.xres_virtual);
    ALOGD("win->var_info.yres_virtual = %d", win->var_info.yres_virtual);
    ALOGD("win->var_info.xoffset = %d", win->var_info.xoffset);
    ALOGD("win->var_info.yoffset = %d", win->var_info.yoffset);
    ALOGD("win->lcd_info.xres = %d", win->lcd_info.xres);
    ALOGD("win->lcd_info.yres = %d", win->lcd_info.yres);
    ALOGD("win->lcd_info.xoffset = %d", win->lcd_info.xoffset);
    ALOGD("win->lcd_info.yoffset = %d", win->lcd_info.yoffset);
    ALOGD("win->lcd_info.bits_per_pixel = %d", win->lcd_info.bits_per_pixel);
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
        ALOGE("%s::id(%d) is weird", __func__, id);
        goto error;
    }

    snprintf(name, 64, device_template, real_id);

    win->fd = open(name, O_RDWR);
    if (win->fd <= 0) {
        ALOGE("%s::Failed to open window device (%s) : %s",
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

void window_clear(struct hwc_context_t *ctx)
{
	struct s3c_fb_win_config_data config;

    memset(&config, 0, sizeof(struct s3c_fb_win_config_data));
    if ( ioctl(ctx->win_fb0.fd, S3CFB_WIN_CONFIG, &config) < 0 )
    {
        ALOGE("%s::S3CFB_WIN_CONFIG failed to clear screen : %s", __func__, strerror(errno));
    }
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
    	ALOGE("%s::S3CFB_EXTDSP_SET_WIN_ADDR failed : %s", __func__, strerror(errno));
    	dump_win(win);
    	return -1;
    }

    if ( ioctl(win->fd, FBIOPAN_DISPLAY, win->var_info) < 0) {
    	ALOGE("%s::FBIOPAN_DISPLAY(%d / %d / %d) failed : %s", __func__, win->var_info.yres,
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
        ALOGE("%s::S3CFB_WIN_POSITION(%d, %d) fail",
            __func__, window.x, window.y);
        dump_win(win);
        return -1;
    }

    /*ALOGD("%s:: x(%d), y(%d)",
            __func__, win->rect_info.x, win->rect_info.y);*/

    win->var_info.xres_virtual = EXYNOS4_ALIGN(win->rect_info.w, 16);
    win->var_info.yres_virtual = win->rect_info.h * NUM_OF_WIN_BUF;
    win->var_info.xres = win->rect_info.w;
    win->var_info.yres = win->rect_info.h;

    win->var_info.activate &= ~FB_ACTIVATE_MASK;
    win->var_info.activate |= FB_ACTIVATE_FORCE;

    if (ioctl(win->fd, FBIOPUT_VSCREENINFO, &(win->var_info)) < 0) {
        ALOGE("%s::FBIOPUT_VSCREENINFO(%d, %d) fail",
          __func__, win->rect_info.w, win->rect_info.h);
        dump_win(win);
        return -1;
    }

    return 0;
}

int window_get_info(struct hwc_win_info_t *win)
{
    if (ioctl(win->fd, FBIOGET_FSCREENINFO, &win->fix_info) < 0) {
        ALOGE("FBIOGET_FSCREENINFO failed : %s",
            strerror(errno));

        dump_win(win);
        win->fix_info.smem_start = NULL;
        return -1;
    }

    return 0;
}

int window_get_var_info(int fd, struct fb_var_screeninfo *var_info)
{
    if (ioctl(fd, FBIOGET_VSCREENINFO, var_info) < 0) {
         ALOGE("FBIOGET_VSCREENINFO failed : %s",
                 strerror(errno));
         return -1;
     }

     return 0;
}

int window_pan_display(struct hwc_win_info_t *win)
{
    struct fb_var_screeninfo *lcd_info = &(win->lcd_info);

    lcd_info->yoffset = lcd_info->yres * win->buf_index;

    if (ioctl(win->fd, FBIOPAN_DISPLAY, lcd_info) < 0) {
        ALOGE("%s::FBIOPAN_DISPLAY(%d / %d / %d) fail(%s)",
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
            ALOGE("%s::S3CFB_SET_WIN_ON failed : (%d:%s)",
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
            ALOGE("%s::S3CFB_SET_WIN_OFF failed : (%d:%s)",
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
            ALOGE("%s::FBIOBLANK failed : (%d:%s)",
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
             ALOGE("%s::FBIOBLANK failed : (%d:%s)",
             __func__, win->fd, strerror(errno));
            dump_win(win);
            return -1;
        }
        win->power_state = 0;
    }
    return 0;
}

int window_buffer_allocate(struct hwc_context_t *ctx, struct hwc_win_info_t *win, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    int rc = 0;
    fb_var_screeninfo vinfo;
    int i, size;

    //ALOGE("%s win->fd=%d ctx->ionclient=%d", __func__, win->fd, ctx->ionclient);

    rc = window_get_var_info(win->fd, (fb_var_screeninfo *) &vinfo);
    if ( rc < 0 )
    {
        ALOGE("%s window_get_var_info returned error: %s", __func__, strerror(errno));
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
        ALOGE("%s::window_get_info is failed : %s", __func__, strerror(errno));
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

    if ((win != &ctx->win_fbdev1)
#ifdef BOARD_USES_HDMI
        || (ctx->hdmi_cable_status)
#endif
                                        ) {
        fbdev1_or_hdmi_connected = 0;
    } else {
        if ( ioctl(ctx->win_fbdev1.fd, S3CFB_GET_CUR_WIN_BUF_ADDR, &cur_win_addr) < 0 )
            ALOGE("%s::S3CFB_GET_CUR_WIN_BUF_ADDR failed");

        fbdev1_or_hdmi_connected = 1;
    }

    for (i=0; i<NUM_OF_WIN_BUF; i++) {
        if (win->fence[i]) {
            if (sync_wait(win->fence[i],1000) < 0)
                ALOGE("%s::sync_wait error", __func__);

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
