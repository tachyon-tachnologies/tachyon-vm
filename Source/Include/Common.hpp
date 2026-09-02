#pragma once

#define __hot __attribute((hot))
#define __unreachable __builtin_unreachable()
#define __unused __attribute((unused))
#define __used __attribute((used))
#define __packed __attribute((packed))
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
