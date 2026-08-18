#ifdef CATCH_EXCEPTIONS
#include "exceptionman/exceptions.h"
#endif
#include <sifrpc-common.h>
#include <kernel.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <iopheap.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <rom0_info.h>
#include <fileio.h>
#include <fileXio_rpc.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sio.h>
#include <debug.h>
#include <sbv_patches.h>
#include <ps2sdkapi.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PROGVER
#define PROGVER "???"
#endif


#define EXTERN_MODULE(_irx) extern unsigned char _irx[]; extern unsigned int size_##_irx

EXTERN_MODULE(iomanX_irx);
EXTERN_MODULE(fileXio_irx);

#define LOADMODULE(_irx, ret) SifExecModuleBuffer(&_irx, size_##_irx, 0, NULL, ret)
#define LOADMODULEARG(_irx, argc, argv, ret) SifExecModuleBuffer(&_irx, size_##_irx, argc, argv, ret)
#define LOADMODULEFILE(path, ret) SifLoadStartModule(path, 0, NULL, ret)
#define LOADMODULEFILEARG(path, argc, argv, ret) SifLoadStartModule(path, argc, argv, ret)
#define IRX_REPORT(irx) scr_printf("\t[IRX] %-16s: id %2d ret %d\n", irx, id, ret);

bool file_exists(const char* F) {
    int fd;
    if ((fd = open(F, O_RDONLY)) >= 0) {
        close(fd);
        return true;
    }
    return false;
}

void scr_hexdump(const void* data, uint32_t size, int hdr) {
    char ascii[17];
    uint32_t i, j;
    ascii[16] = '\0';
    scr_printf("\t");
    if (hdr) {
        for (i = 0; i < 16; i++) {
            if (i == 8)
                scr_printf(" ");
            scr_printf("%02X ", i);
        }
        scr_printf("\n\t");
        for (i = 0; i < 23; i++)
            scr_printf("---");
        scr_printf("\n\t");
    }

    for (i = 0; i < size; ++i) {
        scr_printf("%02X ", ((unsigned char*) data)[i]);
        if (((unsigned char*) data)[i] >= ' ' && ((unsigned char*) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            scr_printf(" ");
            if ((i + 1) % 16 == 0) {
                scr_printf("|  %s \n\t", ascii);
            } else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    scr_printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    scr_printf("   ");
                }
                scr_printf("|  %s \n", ascii);
            }
        }
    }
}

#define LOADMODULE_WALT(path, alias, _irx, ret) loadIRXfromfilewithbuffalt(path, alias, &_irx, size_##_irx, 0, NULL, ret)
#define LOADMODULE_WALT_ARG(path, alias, _irx, argc, argv, ret) loadIRXfromfilewithbuffalt(path, alias, &_irx, size_##_irx, argc, argv, ret)
// try to execute an IRX from file location, if not exist try with embedded buffer
int loadIRXfromfilewithbuffalt(char* path, char* bufalias, void* buffer, unsigned int buffersize, int argc, char* argv, int* ret) {
    int id;
    if (file_exists(path)) {
        id = SifLoadStartModule(path, argc, argv, ret);
        scr_printf("\t[IRX] %-16s: id %2d ret %d\n", path, id, *ret);
    } else {
        id = SifExecModuleBuffer(buffer, buffersize, argc, argv, ret);
        scr_printf("\t[IRX] %-16s: id %2d ret %d\n", bufalias, id, *ret);
    }
    return id;
}


int main(int argc, char** argv) {
    scr_printf("\thello world\n");
    error:
    scr_printf("Program finished...");
    SleepThread();
}


void _ps2sdk_memory_init() {
#ifdef CATCH_EXCEPTIONS
    installExceptionHandlers();
#endif
    sio_puts("BuilDate: "__DATE__ " " __TIME__ "\n");
    while (!SifIopReset("", 0));
    while (!SifIopSync());
    sio_puts("loading fileXio");
    LOADMODULE(iomanX_irx, NULL);
    LOADMODULE(fileXio_irx, NULL); // use fileXio instead of FILEIO to dodge API differences due to SDK 3.0 on arcade bios
    fileXioInit();
    sio_puts("SCR INIT");
    init_scr();
    scr_setCursor(0);
    scr_clear();
    scr_printf("\n\n");
    sio_puts("loading daemon and cdvdfsv");
    int id, ret;
    scr_printf("\t[IOP]: enable load module buffer:%d\n", sbv_patch_enable_lmb());
    scr_printf("\t[IOP]: disable device security:%d\n", sbv_patch_disable_prefix_check());
    id = LOADMODULEFILE("rom0:SIO2MAN", &ret);
    IRX_REPORT("SIO2MAN");
    id = LOADMODULEFILE("rom0:MCMAN", &ret);
    IRX_REPORT("MCMAN");
    id = LOADMODULEFILE("rom0:DAEMON", &ret);
    IRX_REPORT("DAEMON");
    id = LOADMODULEFILE("rom0:CDVDFSV", &ret); // bring back CDVDMAN RPC to avoid hangs on libcglue library
    IRX_REPORT("CDVDFSV");

}
