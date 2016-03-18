#ifndef ANDROID_HWCOMPOSER_WINDOW_H
#define ANDROID_HWCOMPOSER_WINDOW_H

void dump_win(struct hwc_win_info_t *win);

int window_open(struct hwc_win_info_t *win, int id);
int window_close(struct hwc_win_info_t *win);

int window_get_info(struct hwc_win_info_t *win);
int window_get_var_info(int fd, struct fb_var_screeninfo *var_info);

int window_clear(struct hwc_context_t *ctx);
int window_set_pos(struct hwc_win_info_t *win);
int window_set_addr(struct hwc_win_info_t *win);

int window_buffer_allocate(struct hwc_context_t *ctx, struct hwc_win_info_t *win);
int window_buffer_deallocate(struct hwc_context_t *ctx, struct hwc_win_info_t *win);

int get_buf_index(hwc_win_info_t *win);
int get_user_ptr_buf_index(hwc_win_info_t *win);

int window_power_on(struct hwc_win_info_t *win);
int window_power_down(struct hwc_win_info_t *win);

void config_handle(struct hwc_context_t* ctx, private_handle_t *handle, hwc_rect_t &sourceCrop,
        hwc_rect_t &displayFrame, int32_t blending, int fence_fd, s3c_fb_win_config &cfg );

void dump_fb_win_cfg(struct s3c_fb_win_config_data &cfg);

#endif //ANDROID_HWCOMPOSER_WINDOW_H
