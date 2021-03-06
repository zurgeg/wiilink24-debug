#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#include "boot.h"
#include "channel_load.h"
#include "helpers.h"
#include "patches.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {
    // We need to patch before loading anything else.
    bool successfully_applied = patch_ahbprot_reset();

    // Now we're good.
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    printf("\n\n\n\n\n\n");

    if (!successfully_applied) {
        printf("Failed to apply the first stage IOS patch!\n");
        goto finished;
    }

    successfully_applied = apply_patches();
    if (!successfully_applied) {
        printf("Failed to apply second stage IOS patches!\n");
        goto finished;
    }

    hexDump((void *)0x939f9370, 300);

    // TOOD: Allow configuration of loaded channel
    bool canLoadChannel = load_channel_metadata(0x000100014843494a);
    if (!canLoadChannel) {
        printf("Error loading channel metadata\n");
        goto finished;
    }

    // void *entrypoint = load_channel_dol();
    // if (entrypoint == NULL) {
    //     printf("Error loading channel to run\n");
    //     goto finished;
    // }
    // printf("Hey, it worked?\n");

    // Thanks for coming, folks!
    // jump_to_entrypoint(entrypoint);

finished:
    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME)
            exit(0);
        VIDEO_WaitVSync();
    }

    return 0;
}
