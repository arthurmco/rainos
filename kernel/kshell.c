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

#include "vfs/vfs.h"

/* Filesystem operations for the kernel shell */
static vfs_node_t* cwd = NULL;

static int kshell_ls(int argc, char** argv) {

    (void)argc;
    (void)argv;

    vfs_node_t* childs;
    int chcount = vfs_readdir(cwd, &childs);

    if (chcount < 0) {
        kprintf("An error happened while reading directory %s\n", cwd->name);
        return -1;
    } else {
        chcount = 0;
        while (childs) {
            kprintf("%s\t\t%s\t%d bytes\n", childs->name,
                childs->flags & VFS_FLAG_FOLDER ? "[DIR]" : "     ",
                (uint32_t)childs->size & 0xffffffff);
            childs = childs->next;
            chcount++;
        }

        kprintf("\n %d childs\n", chcount);
    }

    return 1;

}

static int kshell_cd(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: cd <dirname>\n\n");
        return 1;
    }

    if (!strcmp(argv[1], ".")) {
        /* Browse into itself */
        return 1;
    } else if (!strcmp(argv[1], "..")) {
        /* Browse into parent */
        if (cwd->parent) {
            cwd = cwd->parent;
        }
        return 1;
    }

    vfs_node_t* node = vfs_find_node_relative(cwd, argv[1]);
    if (!node) {
        kprintf("%s not found\n\n", argv[1]);
        return -1;
    } else if (node && !(node->flags & VFS_FLAG_FOLDER) ) {
        kprintf("%s isn't a directory\n\n", argv[1]);
        return -1;
    } else {
        cwd = node;
    }
    return 1;

}

static int kshell_mount(int argc, char* argv[]) {
    if (argc < 4) {
        kprintf("Usage: mkdir <dirname> <devname> <fs>\n");
        return 1;
    }

    char* dir = argv[1];
    char* dev = argv[2];
    char* fs = argv[3];

    vfs_node_t* node = vfs_find_node_relative(cwd, dir);
    if (!node) {
        kprintf("error: %s not found here\n", dir);
        return -1;
    } else if (node && !(node->flags & VFS_FLAG_FOLDER)) {
        kprintf("error: %s is a file!\n", dir);
        return -1;
    }

    device_t* odev = device_get_by_name(dev);

    if (!odev) {
        kprintf("Device %s not found!\n", dev);
        return -1;
    }

    vfs_filesystem_t* ofs = vfs_get_fs(fs);

    if (!ofs) {
        kprintf("Filesystem %s not found!\n", fs);
        return -1;
    }

    if (!vfs_mount(node, odev, ofs)) {
        kprintf("error: failed to mount %s on %s using fs %s",
            dev, dir, fs);
        return -1;
    }

    return 1;

}

void kshell_init()
{
    cwd = vfs_get_root();

    kshell_add("ls", "Read folder contents", &kshell_ls);
    kshell_add("cd", "Browse into a directory", &kshell_cd);
    kshell_add("mount", "Mount a filesystem", &kshell_mount);

    knotice("KSHELL: Starting kernel shell, %d commands", kcmd_count);
    char cmd[128];
    char* argv[16];
    while (1) {
        go_shell:
        terminal_setcolor(0x0f);
        terminal_puts("kernsh ");
        terminal_setcolor(0x0d);
        terminal_puts(cwd->name);
        terminal_setcolor(0x0f);
        terminal_puts(" >");
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
