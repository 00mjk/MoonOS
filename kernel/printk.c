#include <libk/kstring.h>
#include <libk/kprintf.h>
#include "printk.h"
#include <devices/serial/serial.h>
#include <devices/term/early/early_term.h>
#include <devices/term/limine-port/term.h>
#include <mm/mm.h>

static bool is_verbose_boot = false;

void printk_init(bool verbose_boot, BootContext term_info)
{
    set_boot_term_available(false);
    is_verbose_boot = verbose_boot;
    term_init();
}

void printk(char *status, char *fmt, ...)
{
    char buffer[512];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf((char *)&buffer, (size_t)-1, fmt, arg);
    va_end(arg);

    if (!is_verbose_boot)
    {
        serial_set_color(BASH_GREEN);
        debug(false, "DEBUG: ");
        serial_set_color(BASH_DEFAULT);
        debug(false, "[%s] %s", status, (const char *)&buffer);
        return;
    }


    fmt_puts("%s: %s", status, buffer);
}

void fmt_puts(const char *fmt, ...)
{
    char buffer[512];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf((char *)&buffer, (size_t)-1, fmt, arg);
    va_end(arg);

    _term_write((const char *)&buffer, strlen(buffer));
}

// Note: This should only be called when information
// must be shown, a kernel panic for example
void override_quiet_boot() { is_verbose_boot = true; }