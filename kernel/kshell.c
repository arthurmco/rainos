#include "kshell.h"

#include <kstdio.h>
#include <kstring.h>
#include <kstdlib.h>
#include <kstdlog.h>
#include "terminal.h"
#include "time.h"

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
        size_t bytes = 0;
        char csize[16];
        while (childs) {
            sprintf(csize, "%d bytes", (uint32_t)childs->size & 0xffffffff);
            kprintf("%20s\t\t%10s", childs->name,
                childs->flags & VFS_FLAG_FOLDER ? "[DIR]" : csize);

            bytes += (size_t)childs->size;

            struct time_tm cdate = {0};
            time_from_unix(childs->date_creation, &cdate);
            kprintf("\t\t%d/%d/%d %d:%d:%d\n",
                    cdate.day, cdate.month, cdate.year,
                    cdate.hour, cdate.minute, cdate.second);

            childs = childs->next;
            chcount++;

            if (chcount % 20 == 0 && chcount > 0) {
                kgetc();
            }
        }

        kprintf("\n %d childs, %d %s total\n", chcount,
            (bytes > 1024) ? (bytes/1024) : bytes,
            (bytes > 8192) ? "kB" : "bytes");
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
        kprintf("Usage: %s <dirname> <devname> <fs>\n", argv[0]);
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

    if (!(odev->devtype & DEVTYPE_BLOCK)) {
        kprintf("Device %s isn't a block device!\n", dev);
        knotice("%s devtype: 0x%02x, required 0x%02x", odev->devname,
            odev->devtype, DEVTYPE_BLOCK);
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

static int kshell_cat(int argc, char* argv[])
{
    if (argc < 2) {
        kprintf("\n Usage: cat <filename> \n");
        return 2;
    }

    vfs_node_t* node = vfs_find_node_relative(cwd, argv[1]);

    if (!node) {
        kprintf("\n\t%s doesn't exist\n", argv[1]);
        return -1;
    }

    if (node->flags & VFS_FLAG_FOLDER) {
        kprintf("\n\t%s is a folder\n", node->name);
        return 2;
    }

    kprintf("\n %s - %d bytes\n", node->name, node->size);
    size_t pos = 0;
    char* file = kcalloc(node->size + 1,1);
    int rx = vfs_read(node, pos, -1, file);

    if (rx >= 0) {
        kputs(file);
    }

    kfree(file);
    return 0;
}

static int kshell_hexdump(int argc, char* argv[])
{
    if (argc < 2) {
        kprintf("\n Usage: hexdump <filename> \n");
        return 2;
    }

    vfs_node_t* node = vfs_find_node_relative(cwd, argv[1]);

    if (!node) {
        kprintf("\n\t%s doesn't exist\n", argv[1]);
        return -1;
    }

    if (node->flags & VFS_FLAG_FOLDER) {
        kprintf("\n\t%s is a folder\n", node->name);
        return 2;
    }

    kprintf("\n %s - %d bytes\n", node->name, node->size);
    size_t pos = 0;
    char* file = kcalloc(node->size,1);
    int rx = vfs_read(node, pos, -1, file);

    if (rx >= 0) {
        do {
            for (size_t c = 0; c <= 320/16; c++) {
                for (size_t r = 0; r < 16; r++) {
                    uint8_t s = file[pos+(c*16+r)];
                    kprintf("%02x ", s);
                }
                kprintf(" | ");
                for (size_t r = 0; r < 16; r++) {
                    unsigned char ch = (unsigned char)file[pos+(c*16+r)];
                    if (ch < ' ')
                        putc('.');
                    else
                        putc(ch);
                }
                kprintf("\n");
            }

            char x = kgetc();
            if (x == 'q') {
                break;
            }

            pos += 320;
        } while (pos < node->size);
    }

    kfree(file);
    return 0;
}

static int kshell_date(int argc, char** argv)
{
    struct time_tm time;
    time_gettime(&time);

    kprintf("\n%02d/%02d/%04d %02d:%02d:%02d\n",
        time.day, time.month, time.year,
        time.hour, time.minute, time.second);

}

void kshell_init()
{
    cwd = vfs_get_root();

    kshell_add("ls", "Read folder contents", &kshell_ls);
    kshell_add("cd", "Browse into a directory", &kshell_cd);
    kshell_add("mount", "Mount a filesystem", &kshell_mount);
    kshell_add("clear", "Clears the screen", &terminal_clear);
    kshell_add("cat", "Read the content", &kshell_cat);
    kshell_add("hexdump", "Dumps file binary data", &kshell_hexdump);
    kshell_add("date", "Show date and time", &kshell_date);

    knotice("KSHELL: Starting kernel shell, %d commands", kcmd_count);
    char cmd[128];
    char* argv[16];
    while (1) {
        go_shell:
        terminal_setcolor(0x0f);
        terminal_puts("kernsh ");
        terminal_setcolor(0x0a);
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
