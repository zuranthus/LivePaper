#pragma once

void MessageAndQuit(int line, const char *message, ...);

#define FAIL_WITH(M, ...) MessageAndQuit(__LINE__, M, ##__VA_ARGS__)
#define FAIL() MessageAndQuit(__LINE__, NULL)
