#pragma once

struct Context;

struct Video* VideoLoad(const struct Context *context);
void VideoUpdate(double delta_sec, struct Video *video, const struct Context *context);
void VideoClear(struct Video *video, const struct Context *context);