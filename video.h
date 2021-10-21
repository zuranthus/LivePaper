#pragma once

struct Video* VideoLoad(char *path, const struct Context *context);
void VideoUpdate(double delta_sec, struct Video *video, const struct Context *context);
void VideoClear(struct Video *video, const struct Context *context);