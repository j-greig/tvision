/*---------------------------------------------------------*/
/*   doomgeneric_tvision.c - Turbo Vision Platform Layer  */
/*   Implements doomgeneric interface for WibWobDOS       */
/*---------------------------------------------------------*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "doomgeneric.h"
#include "doom.h"  // For D_DoomMain and tvision_tickable_mode
#include "m_argv.h"  // For myargc and myargv

// Global screen buffer and dimensions (externals from doomgeneric)
uint32_t* DG_ScreenBuffer = NULL;
unsigned DOOMGENERIC_RESX = 320;
unsigned DOOMGENERIC_RESY = 200;

// Internal state
static uint32_t* g_screenbuffer_copy = NULL;
static int g_initialized = 0;

// Static argv for DOOM (persists for program lifetime)
static char* doom_argv[3];
static char iwad_flag[] = "-iwad";
static char wad_path_buffer[512];

//
// DG_Init
//
// Initialize platform layer
// - Set up command-line arguments for WAD path
// - Allocate screen buffer
// - Enable tickable mode
// - Call D_DoomMain() to initialize game (which returns when tickable mode is on)
//
void DG_Init(const char* wadPath)
{
    // Set up command-line arguments for DOOM if WAD path is provided
    if (wadPath && wadPath[0] != '\0') {
        // argv[0] = program name (unused but required)
        // argv[1] = "-iwad"
        // argv[2] = path to WAD file
        doom_argv[0] = "doom";
        doom_argv[1] = iwad_flag;

        // Copy WAD path to persistent buffer
        strncpy(wad_path_buffer, wadPath, sizeof(wad_path_buffer) - 1);
        wad_path_buffer[sizeof(wad_path_buffer) - 1] = '\0';
        doom_argv[2] = wad_path_buffer;

        // Set global argc/argv for DOOM engine
        myargc = 3;
        myargv = doom_argv;

        fprintf(stderr, "[DOOM] WAD path set to: %s\n", wadPath);
    } else {
        // No WAD path specified - DOOM will search default locations
        doom_argv[0] = "doom";
        myargc = 1;
        myargv = doom_argv;
        fprintf(stderr, "[DOOM] No WAD path specified, using default search\n");
    }

    // Allocate screen buffer for DOOM to render into
    DG_ScreenBuffer = (uint32_t*)calloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY, sizeof(uint32_t));
    if (!DG_ScreenBuffer) {
        fprintf(stderr, "ERROR: Failed to allocate DOOM screen buffer\n");
        exit(1);
    }

    // Allocate copy buffer for frame persistence
    g_screenbuffer_copy = (uint32_t*)calloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY, sizeof(uint32_t));
    if (!g_screenbuffer_copy) {
        fprintf(stderr, "ERROR: Failed to allocate screen buffer copy\n");
        exit(1);
    }

    g_initialized = 1;

    // Enable tickable mode - prevents D_DoomMain from entering infinite loop
    tvision_tickable_mode = true;

    // TODO: WAD file validation could go here
    // For now, D_DoomMain() handles WAD loading and will call I_Error on failure

    // Initialize DOOM engine - this will return due to tickable mode
    fprintf(stderr, "[DOOM] Calling D_DoomMain()...\n");
    D_DoomMain();
    fprintf(stderr, "[DOOM] D_DoomMain() returned successfully\n");
}

//
// DG_DrawFrame
//
// Called by DOOM after rendering a frame
// - Copy DG_ScreenBuffer to internal storage
// - Signal TView to redraw (TODO: Phase 3)
//
void DG_DrawFrame(void)
{
    if (!g_initialized || !DG_ScreenBuffer || !g_screenbuffer_copy) {
        return;
    }

    // Copy frame buffer for persistence
    memcpy(g_screenbuffer_copy, DG_ScreenBuffer,
           DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(uint32_t));

    // TODO Phase 3: Signal TDoomAsciiView to convert and blit buffer
}

//
// DG_GetKey
//
// Poll for keyboard input
// Returns 1 if key pressed, 0 otherwise
// - Dequeue from TView's input queue
//
// External accessors from doom_ascii_view.cpp
extern int doomInputQueueEmpty(void);
extern unsigned char doomInputQueuePop(void);

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    // Check if input queue has keys
    if (doomInputQueueEmpty()) {
        *pressed = 0;
        return 0;
    }

    // Dequeue key
    *pressed = 1;
    *doomKey = doomInputQueuePop();
    return 1;
}

//
// DG_ReadInput
//
// Called each frame to read input
// - Currently no-op (input handled in DG_GetKey)
//
void DG_ReadInput(void)
{
    // No-op: input is queued by TView event handling
}

//
// DG_GetTicksMs
//
// Return monotonic time in milliseconds
// - Uses clock_gettime for accuracy
//
uint32_t DG_GetTicksMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

//
// DG_SleepMs
//
// Sleep for specified milliseconds
// - No-op: timer controls frame rate, not busy-wait
//
void DG_SleepMs(uint32_t ms)
{
    // No-op: TV timer controls frame rate (33ms intervals)
    // DOOM doesn't need to sleep; we call D_DoomTick() explicitly
}

//
// DG_SetWindowTitle
//
// Update window title
// - TODO Phase 6: Update TWindow title text
//
void DG_SetWindowTitle(const char* title)
{
    // TODO Phase 6: Update TDoomAsciiWindow title
    // Store title for later retrieval or update window directly
}

//
// Accessor for TView to get rendered frame buffer
// Returns pointer to screen buffer copy
//
uint32_t* getDoomScreenBuffer(void)
{
    return g_screenbuffer_copy;
}

//
// Accessor for TView to get screen dimensions
//
void getDoomScreenDimensions(unsigned* width, unsigned* height)
{
    *width = DOOMGENERIC_RESX;
    *height = DOOMGENERIC_RESY;
}
