#ifndef KLOG_H
#define KLOG_H
#include <stdint.h>
void klog(const char *msg);
void kklog(const char *msg);
void klog_hex(uint32_t val);
void klogf(const char *fmt, ...);
#endif
