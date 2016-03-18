
#include "hwcomposer.h"

const char *V4L2_DEV = "/dev/video2";

int v4l2_open(struct hwc_context_t *ctx)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_control vc;

    // open V4L2 node
    if (ctx->v4l2_fd <= 0)
        ctx->v4l2_fd = open(V4L2_DEV, O_RDWR);

    if (ctx->v4l2_fd  <= 0) {
        ALOGE("%s: Unable to get v4l2 node for %s", __FUNCTION__, V4L2_DEV);
        goto err;
    }

    // Check capabilities
    if (ioctl(ctx->v4l2_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        ALOGE("%s: Unable to query capabilities");
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        ALOGE("%s: %s has no streaming support", __FUNCTION__, V4L2_DEV);
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        ALOGE("%s: %s has no video out support", ctx->v4l2_fd, V4L2_DEV);
        goto err;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(ctx->v4l2_fd, VIDIOC_G_FMT, &fmt) < 0) {
        ALOGE("%s: Unable to get format", __FUNCTION__);
        goto err;
    }

    vc.id = V4L2_CID_OVLY_MODE;
    vc.value = FIMC_OVLY_NONE_MULTI_BUF;

    if (ioctl(ctx->v4l2_fd, VIDIOC_S_CTRL, &vc) < 0) {
        ALOGE("%s: Unable to set overlay mode", __FUNCTION__);
        goto err;
    }

    return 0;

err:
    if (ctx->v4l2_fd > 0)
        close(ctx->v4l2_fd);

    ctx->v4l2_fd = 0;

    return -1;
}

void v4l2_close(struct hwc_context_t *ctx)
{
    if (ctx->v4l2_fd > 0)
        close(ctx->v4l2_fd);

    ctx->v4l2_fd = 0;
}
