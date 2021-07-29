#include "panic.h"
#include "util/ptr.h"
#include "drivers/gfx/gfx.h"
#include "drivers/io/serial.h"
#include "trace/strace.h"
#include <stdarg.h>
#include <libk/kprintf.h>

char panic_buff[512];
void panic(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(GENERIC_CAST(char *, &panic_buff), GENERIC_CAST(size_t, -1), fmt, ap);
	va_end(ap);

	printk("panic", "KERNEL PANIC: %s\n", panic_buff);
	debug(true, "KERNEL PANIC: %s\n", panic_buff);

	strace();
}