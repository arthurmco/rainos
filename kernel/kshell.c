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

static int cmd_args(int argc, char* argv[])
{
    kprintf("argc: %d\n", argc);
    kprintf("argv: ");

    for (int i = 0; i < argc; i++) {
        kprintf("'%s' ", argv[i]);
    }

    kprintf("\n");
    return 1;
}

void kshell_init()
{
    kshell_add("args", "Test kernel shell argument passing", cmd_args);
    knotice("KSHELL: Starting kernel shell, %d commands", kcmd_count);
    char cmd[128];
    char* argv[16];
    while (1) {
        go_shell:
        terminal_setcolor(0x0f);
        terminal_puts("kernsh> ");
        terminal_restorecolor();
        size_t l = kgets(cmd, 128);
        if (l > 1) {
            int argc = 0;
            cmd[l-1] = 0;
            kshell_parse_args(cmd, l, &argc, argv);

            for (size_t i = 0; i < kcmd_count; i++) {
                if (!strcmp(cmd, kcmds[i].name)) {
                    kcmds[i].cmd(argc, argv);
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
    memcpy((void*)name, kcmds[kcmd].name, min(16, strlen(name)+1));
    memcpy((void*)desc, kcmds[kcmd].desc, min(96, strlen(desc)+1));
    kcmds[kcmd].cmd = ksc;
    knotice("Adding command %s @ 0x%08x", kcmds[kcmd].name, kcmds[kcmd].cmd);
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

void kshell_parse_args(char* cmd, size_t len, int* argc, char* argv[])
{
    int args = 0;
    size_t last_arg_start = 0, last_arg_end = 0;
    int trim = 0;

    /* Parse kernel shell command into arguments */
    for (size_t i = 0; i < len; i++) {

        /* Is it a space? Mark end of an argument */
        if (cmd[i] == ' ') {
            cmd[i] = 0;
            last_arg_end = i;
            trim = 1;
            continue;
        }

        /* Unneeded zero space */
        if (cmd[i] == ' ' && i > last_arg_end && trim) {
            cmd[i] = 0;
        }

        /*  Non-space char after space char, mark beginning of new command
            and end of old command */
        if (cmd[i] != ' ' && trim) {
            argv[args++] = &cmd[last_arg_start];
            last_arg_start = i;
            trim = 0;
        }

        /* Newline or zero, end of string */
        if (cmd[i] == 0 || cmd[i] == '\n') {
            argv[args++] = &cmd[last_arg_start];
            break;
        }
    }

    *argc = args;
}
