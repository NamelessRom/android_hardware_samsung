
#ifndef ANDROID_HWCOMPOSER_H
#define ANDROID_HWCOMPOSER_H

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>

#include <hardware/hwcomposer.h>
#include <utils/Vector.h>

#include <EGL/egl.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sync/sync.h>
#include <system/graphics.h>

#include "gralloc_priv.h"
#include "s3c_lcd.h"
#include "secion.h"
#include "sec_format.h"
#include "s5p_fimc.h"

#include "videodev2.h"

#include "window.h"
#include "utils.h"
#include "v4l2.h"
#include "FimgApi.h"

#ifndef _VSYNC_PERIOD
#define _VSYNC_PERIOD 1000000000UL
#endif

const size_t NUM_HW_WINDOWS = 5;
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;
const size_t NUM_OF_WIN_BUF = 3;

#define DEBUG 1

#ifdef ALOGD
#undef ALOGD
#endif
#define ALOGD(...) if (DEBUG) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#define EXYNOS4_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }

struct sec_rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

struct ion_hdl{
    ion_client          client;
    struct secion_param param;
    ion_client          client2;
};

struct gsc_map_t {
    enum {
        NONE = 0,
        FIMG,
        FIMC,
        MAX,
    } mode;
};

/* I decided to use hwc_win_info_t because the windows don't often change its geometry

const size_t NUM_GSC_UNITS = gsc_map_t::mode.MAX - 1;

struct hwc_post_data_t {
    int         overlay_map[NUM_HW_WINDOWS];
    gsc_map_t   gsc_map[NUM_HW_WINDOWS];
    size_t      fb_window;
};

const size_t NUM_GSC_DST_BUFS = 3;
struct gsc_data_t {
    enum gsc_map_t::mode type;
    struct fimg2d_blit   fimg_cmd;
    buffer_handle_t      dst_buf[NUM_GSC_DST_BUFS];
    int                  dst_buf_fence[NUM_GSC_DST_BUFS];
    size_t               current_buf;
};*/

struct hwc_win_info_t {
    int                         fd;
    int                         size;
    sec_rect                    rect_info;
    buffer_handle_t             dst_buf[NUM_OF_WIN_BUF];
    int                         dst_buf_fence[NUM_OF_WIN_BUF];
    size_t                      current_buf;
    uint32_t                    addr[NUM_OF_WIN_BUF]; // needed for win_fb0 and win_devfb1

    struct gsc_map_t            gsc;
    struct fimg2d_blit          fimg_cmd; //if geometry, blending hasn't changed, only buffers have to be swapped

    int                         power_state;
    int                         blending;
    int                         layer_index;
    int                         status;
    int                         vsync;

    struct fb_fix_screeninfo    fix_info;
    struct fb_var_screeninfo    var_info;
    struct fb_var_screeninfo    lcd_info;

    struct secion_param         secion_param[NUM_OF_WIN_BUF];
    //int                       fence[NUM_OF_WIN_BUF]; --> we better use dst_buf_fence
};

struct hwc_context_t {
    hwc_composer_device_1_t   device;
    /* our private state goes below here */
    hwc_procs_t              *procs;

    struct private_module_t  *gralloc_module;
    alloc_device_t           *alloc_device;

    int                       force_gpu;

    struct hwc_win_info_t     win[NUM_HW_WINDOWS];
    struct hwc_win_info_t     win_fb0;
    struct hwc_win_info_t     win_fbdev1;
    struct fb_var_screeninfo  lcd_info;
    struct fb_var_screeninfo  fbdev1_info;
    bool                      win_fbdev1_needs_buffer;

    struct ion_hdl            ion_hdl;
    struct ion_hdl            fimg_tmp_ion_hdl;

    // V4L2 info of FIMC device
    int    v4l2_fd;

    int    xres;
    int    yres;
    float  xdpi;
    float  ydpi;
    int    vsync_period;

    // several layer counters
    unsigned int yuv_layer;
    unsigned int hwc_layer;
    unsigned int gsc_layer;

    unsigned int current_gsc_index;

    bool         fb_needed;
    size_t       first_fb;
    size_t       last_fb;
    size_t       fb_window;

    struct s3c_fb_win_config last_config[NUM_HW_WINDOWS];
    size_t                   last_fb_window;
    const void              *last_handles[NUM_HW_WINDOWS];

};

#endif //ANDROID_HWCOMPOSER_H
