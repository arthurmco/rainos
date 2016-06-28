/*  Kernel debug shell control

    Copyright (C) 2016 Arthur M
*/

#ifndef _KSHELL_H
#define _KSHELL_H

typedef int (*KernShellCommand)(int argc, char* argv[]);

#define MAX_SHELL_CMD 128
struct kernel_shell_cmd {
    char name[16];
    char desc[96];
    KernShellCommand cmd;
};

void kshell_init();

void kshell_add(const char* name, const char* desc, KernShellCommand ksc);
void kshell_remove(KernShellCommand ksc);


#endif /* end of include guard: _KSHELL_H */
