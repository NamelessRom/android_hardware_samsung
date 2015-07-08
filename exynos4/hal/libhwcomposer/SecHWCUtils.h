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
 *
 * @author Rama, Meka(v.meka@samsung.com)
	   Sangwoo, Park(sw5771.park@samsung.com)
	   Jamie, Oh (jung-min.oh@samsung.com)
 * @date   2011-03-11
 *
 */

#ifndef ANDROID_SEC_HWC_UTILS_H_
#define ANDROID_SEC_HWC_UTILS_H_

#include <stdlib.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <poll.h>

//#include <linux/videodev.h>
#include <linux/fb.h>
#include <sync/sync.h>

#include "videodev2.h"
#include "s5p_fimc.h"
#include "sec_utils.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include "gralloc_priv.h"

#include "s3c_lcd.h"
#include "sec_format.h"
#include "secion.h"

//#define HWC_DEBUG 1
#if defined(BOARD_USES_FIMGAPI)
#include "FimgApi.h"
#endif

#define SKIP_DUMMY_UI_LAY_DRAWING

#ifdef SKIP_DUMMY_UI_LAY_DRAWING
#define GL_WA_OVLY_ALL
#define THRES_FOR_SWAP  (3427)    /* 60sec in Frames. 57fps * 60 = 3427 */
#endif

#define NUM_OF_DUMMY_WIN    (4)

const size_t NUM_HW_WINDOWS = 5;
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;

#define NUM_OF_WIN_BUF      (3)
#define NUM_OF_MEM_OBJ      (1)

#if (NUM_OF_WIN_BUF < 2)
    #define ENABLE_FIMD_VSYNC
#endif

#define MAX_RESIZING_RATIO_LIMIT  (63)

#ifdef SAMSUNG_EXYNOS4x12
#define PP_DEVICE_DEV_NAME  "/dev/video2"
#endif

#ifdef SAMSUNG_EXYNOS4210
#define PP_DEVICE_DEV_NAME  "/dev/video1"
#endif

/* cacheable configuration */
#define V4L2_CID_CACHEABLE			(V4L2_CID_BASE+40)

#define EXYNOS4_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

struct sec_rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

struct sec_img {
    uint32_t f_w;
    uint32_t f_h;
    uint32_t w;
    uint32_t h;
    uint32_t format;
    uint32_t base;
    uint32_t offset;
    uint32_t paddr;
    uint32_t uoffset;
    uint32_t voffset;
    int      usage;
    int      mem_id;
    int      mem_type;
};

inline int SEC_MIN(int x, int y)
{
    return ((x < y) ? x : y);
}

inline int SEC_MAX(int x, int y)
{
    return ((x > y) ? x : y);
}

struct hwc_win_info_t {
    int        fd;
    int        size;
    sec_rect   rect_info;//0x8 in blob it has offset 0x18
    uint32_t   addr[NUM_OF_WIN_BUF]; //0x18, in blob it has offset 0x64
    int        buf_index; // 0x24, in blob it has offset

    int        power_state; // 0x28 -> in blob has offset 0x80
    int        blending; //0x2c
    int        layer_index; //0x30
    int        status; //0x34
    int        vsync; // 0x38
    int        field8c;
    int        field90;

    struct fb_fix_screeninfo fix_info; //0x3c -> in blob it has offset 0x9c
    struct fb_var_screeninfo var_info; // in blob, offset 0xe0
    struct fb_var_screeninfo lcd_info;

    struct secion_param secion_param[NUM_OF_WIN_BUF]; //in blob, it has offset 0x28
    int    fence[NUM_OF_WIN_BUF]; //in blob, it has offset 0x70
};

enum {
    HWC_WIN_FREE = 0,
    HWC_WIN_RESERVED,
};

enum {
    HWC_UNKNOWN_MEM_TYPE = 0,
    HWC_PHYS_MEM_TYPE,
    HWC_VIRT_MEM_TYPE,
};

#ifdef SKIP_DUMMY_UI_LAY_DRAWING
struct hwc_ui_lay_info{
    uint32_t   layer_prev_buf;
    int        layer_index;
    int        status;
    sec_rect   crop_info;
    sec_rect   display_frame;
};
#endif

struct ion_hdl{
    ion_client          client;
    struct secion_param param;
    ion_client          client2;
};

struct hwc_context_t {
    hwc_composer_device_1_t device;

    /* our private state goes below here */
    struct hwc_win_info_t     win[NUM_HW_WINDOWS]; //offset 0x1f8, win[0] in 0x1f8, win[1] in 0x418, win[2] in 0x638, win[3] in 0x858, win[4] in 0xa78
    struct hwc_win_info_t     global_lcd_win; // in blob it has offset 0x1044, accessed via 0x1040 + 4
    struct hwc_win_info_t     fbdev1_win; // 0xe24

#ifdef SKIP_DUMMY_UI_LAY_DRAWING
    struct hwc_ui_lay_info    win_virt[NUM_OF_DUMMY_WIN];
    int                       fb_lay_skip_initialized;
    int                       num_of_fb_lay_skip;
#ifdef GL_WA_OVLY_ALL
    int                       ui_skip_frame_cnt;
#endif
#endif

    struct fb_var_screeninfo  lcd_info; //in blob it has offset 0x1480 + 4
    struct fb_var_screeninfo  fbdev1_info; //in blob it has offset 0x1520 + 4
    s5p_fimc_t                fimc;  //a similar structure has offset 0x15c0 + 4 in blob
    hwc_procs_t               *procs; // in blob, it has offset 0x98
    pthread_t                 uevent_thread;

    pthread_t                 vsync_thread;
    int                       vsync_thread_enabled;
    int                       vsync_a5;
    int                       vsync_a6;
    int                       vsync_a7;
    int                       vsync_a8;
    int                       field_ac;

    pthread_t                 sync_merge_thread;
    pthread_cond_t            sync_merge_condition;
    pthread_mutex_t           sync_merge_thread_mutex;
    int                       sync_merge_thread_running;
    int                       sync_merge_thread_enabled;
    int                       sync_merge_thread_created;
    int                       sync_merge_ready; //struct 19a0, offset 0x14

    pthread_t                 fimc_thread;
    pthread_cond_t            fimc_thread_condition; //struct 19c0, offset 4
    pthread_mutex_t           fimc_thread_mutex;
    int                       fimc_thread_enabled;  //struct 19a0, offset 1c

    pthread_t                 capture_thread;
    int                       capture_thread_enabled; //byte_1021C
    int                       capture_thread_created;

    struct private_module_t   *gralloc;
    int                       num_of_fb_layer;
    int                       num_of_hwc_layer; // 0xcb4
    int                       num_of_fb_layer_prev;
    int                       num_2d_blit_layer;
    uint32_t                  layer_prev_buf[NUM_HW_WINDOWS];

    int                       num_of_ext_disp_layer; // offset 0xc98
    int                       num_of_ext_disp_video_layer; //offset 0xc9c
    int                       num_of_hwoverlay_layer; // offset 0xcac
    int                       num_of_other_layer; //offset 0xca8
    int                       num_of_camera_layer; //offset 0xcb0

#ifdef BOARD_USES_HDMI
    int                       hdmiCableStatus; //struct 16c0, offset 0x18
    android::SecHdmiClient    hdmiClient; //str18c0 field 8
    int                       hdmiMirrorWithVideoMode; //str16e0 offset 0
    int                       hdmiVideoTransform;  //19a0 offset 8
    bool                      hdmi_cable_status_changed; //16c0 offset 1c
#endif

    int                       hdmi_xres; //0xcf4
    int                       hdmi_yres; //0xcf8

    int                       wfd_connected; //0x1e4
    int                       wfd_connected_last; //0x1e8

    struct ion_hdl            ion_hdl;  // 0x1640 + 4
    struct ion_hdl            fimg_tmp_ion_hdl; //0x1660
    struct ion_hdl            fimg_ion_hdl; // 0x1680

    int                       overlay_map[NUM_HW_WINDOWS]; //0x7c
    int                       fb_window; //0x90

    int                       xres; //0xb0
    int                       yres; //0xb4
    float                     xdpi; //0xb8
    float                     ydpi; //0xbc
    int                       vsync_period; //0x0c

    int                       max_private_layer; //str1680 field 18
    int                       num_of_private_layer; // 0xca4
    int                       str1680_field1c;
    int                       str16a0_field0;
    int                       str16a0_field8; //str16a0 field 8
    int                       str1960_fieldc;
    int                       str1980_yres; //field 0
    int                       fbdev1_win_needs_buffer; //str1980 field 18
    int                       str1980_field18;
    bool                      fimg_secure;
    int                       str1980_field1c;
    int                       currentNumHwLayers; //str19a0 field 0x10
    hwc_display_contents_1   *currentDisplay; //str19a0 field 0xc
    unsigned int              str16c0_field4;

    bool                      avoid_eglSwapBuffers; //str19a0 offset 18
    bool                      str19a0_field19;
    bool                      str19a0_field1a;
    bool                      str19a0_field1b;
};



typedef enum _LOG_LEVEL {
    HWC_LOG_DEBUG,
    HWC_LOG_WARNING,
    HWC_LOG_ERROR,
} HWC_LOG_LEVEL;

#define SEC_HWC_LOG_TAG     "SECHWC_LOG"

#ifdef HWC_DEBUG
#define SEC_HWC_Log(a, ...)    ((void)_SEC_HWC_Log(a, SEC_HWC_LOG_TAG, __VA_ARGS__))
#else
#define SEC_HWC_Log(a, ...)                                         \
    do {                                                            \
	if (a == HWC_LOG_ERROR)                                     \
	    ((void)_SEC_HWC_Log(a, SEC_HWC_LOG_TAG, __VA_ARGS__)); \
    } while (0)
#endif

extern void _SEC_HWC_Log(HWC_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

/* copied from gralloc module ..*/
typedef struct {
    native_handle_t base;

    /* These fields can be sent cross process. They are also valid
     * to duplicate within the same process.
     *
     * A table is stored within psPrivateData on gralloc_module_t (this
     * is obviously per-process) which maps stamps to a mapped
     * PVRSRV_CLIENT_MEM_INFO in that process. Each map entry has a lock
     * count associated with it, satisfying the requirements of the
     * Android API. This also prevents us from leaking maps/allocations.
     *
     * This table has entries inserted either by alloc()
     * (alloc_device_t) or map() (gralloc_module_t). Entries are removed
     * by free() (alloc_device_t) and unmap() (gralloc_module_t).
     *
     * As a special case for framebuffer_device_t, framebuffer_open()
     * will add and framebuffer_close() will remove from this table.
     */

#define IMG_NATIVE_HANDLE_NUMFDS 1
    /* The `fd' field is used to "export" a meminfo to another process.
     * Therefore, it is allocated by alloc_device_t, and consumed by
     * gralloc_module_t. The framebuffer_device_t does not need a handle,
     * and the special value IMG_FRAMEBUFFER_FD is used instead.
     */
    int fd;

#if 1
    int format;
    int magic;
    int flags;
    int size;
    int offset;
    int base_addr;
#define IMG_NATIVE_HANDLE_NUMINTS ((sizeof(uint64_t) / sizeof(int)) + 4 + 6)
#else
#define IMG_NATIVE_HANDLE_NUMINTS ((sizeof(IMG_UINT64) / sizeof(int)) + 4)
#endif
    /* A KERNEL unique identifier for any exported kernel meminfo. Each
     * exported kernel meminfo will have a unique stamp, but note that in
     * userspace, several meminfos across multiple processes could have
     * the same stamp. As the native_handle can be dup(2)'d, there could be
     * multiple handles with the same stamp but different file descriptors.
     */
    uint64_t ui64Stamp;

    /* We could live without this, but it lets us perform some additional
     * validation on the client side. Normally, we'd have no visibility
     * of the allocated usage, just the lock usage.
     */
    int usage;

    /* In order to do efficient cache flushes we need the buffer dimensions
     * and format. These are available on the android_native_buffer_t,
     * but the platform doesn't pass them down to the graphics HAL.
     *
     * TODO: Ideally the platform would be modified to not require this.
     */
    int width;
    int height;
    int bpp;
}
__attribute__((aligned(sizeof(int)),packed)) sec_native_handle_t;

int window_open       (struct hwc_win_info_t *win, int id);
int window_close      (struct hwc_win_info_t *win);
int window_set_pos    (struct hwc_win_info_t *win);
int window_get_info   (struct hwc_win_info_t *win);
int window_pan_display(struct hwc_win_info_t *win);
int window_show       (struct hwc_win_info_t *win);
int window_hide       (struct hwc_win_info_t *win);
int window_get_global_lcd_info(int fd, struct fb_var_screeninfo *lcd_info);

int createFimc (s5p_fimc_t *fimc);
int destroyFimc(s5p_fimc_t *fimc);
int runFimc(struct hwc_context_t *ctx,
	    struct sec_img *src_img, struct sec_rect *src_rect,
	    struct sec_img *dst_img, struct sec_rect *dst_rect,
	    uint32_t transform);
void clearBufferbyFimc(struct hwc_context_t *ctx, sec_img *dst_img);
int check_yuv_format(unsigned int color_format);
void dump_win(struct hwc_win_info_t *win);
int waiting_fimc_csc(int fd);

int formatValueHAL2G2D(unsigned int format, unsigned int *g2d_format,
        unsigned int *pixel_order, unsigned int *bpp);
int rotateValueHAL2G2D(unsigned char transform);

int get_buf_index(hwc_win_info_t *win);
int get_user_ptr_buf_index(hwc_win_info_t *win);

#endif /* ANDROID_SEC_HWC_UTILS_H_*/
