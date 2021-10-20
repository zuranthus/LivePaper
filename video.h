#pragma once

struct Context;

struct Video* VideoLoad(char *path, const struct Context *context);
bool VideoUpdate(double delta_sec, struct Video *video, const struct Context *context);
void VideoClear(struct Video *video, const struct Context *context);