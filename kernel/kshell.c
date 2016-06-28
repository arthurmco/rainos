#include "kshell.h"

#include <kstdio.h>
#include <kstring.h>
#include <kstdlib.h>
#include <kstdlog.h>
#include "terminal.h"

static struct kernel_shell_cmd kcmds[MAX_SHELL_CMD];
static size_t kcmd_count = 0;

#define max(a,b) (a > b) ? a : b
#define min(a,b) (a < b) ? a : b

void kshell_init()
{
    knotice("KSHELL: Starting kernel shell, %d commands", kcmd_count);
    char cmd[128];
    while (1) {
        go_shell:
        terminal_setcolor(0x0f);
        terminal_puts("kernsh> ");
        terminal_restorecolor();
        size_t l = kgets(cmd, 128);
        if (l > 2) {
            cmd[l-1] = 0;
            for (size_t i = 0; i < kcmd_count; i++) {
                if (!strcmp(cmd, kcmds[i].name)) {
                    kcmds[i].cmd(0, &kcmds[i].name);
                    goto go_shell;
                }
            }

            if (!strcmp(cmd, "help")) {
                kputs("\nAvaliable commands: \n");
                for (size_t i = 0; i < kcmd_count; i++) {
                    kprintf("%s - %s\n", kcmds[i].name, kcmds[i].desc);
                }
                kputs("\n");
            } else {
                kprintf("Command '%s' not found\n", cmd);
            }
        }
    }
}

void kshell_add(const char* name, const char* desc, KernShellCommand ksc)
{
    size_t kcmd = kcmd_count++;
    memcpy(name, kcmds[kcmd].name, min(16, strlen(name)+1));
    memcpy(desc, kcmds[kcmd].desc, min(96, strlen(desc)+1));
    knotice("Adding command %s @ 0x%08x", kcmds[kcmd].name, kcmds[kcmd].cmd);
    kcmds[kcmd].cmd = ksc;
}

void kshell_remove(KernShellCommand ksc)
{
    for (size_t i = 0; i < kcmd_count; i++) {
        if (kcmds[i].cmd == ksc) {
            knotice("Removing command %s @ 0x%08x", kcmds[i].name, kcmds[i].cmd);
            memset(&kcmds[i], 0, sizeof(struct kernel_shell_cmd));
        }
    }
}
