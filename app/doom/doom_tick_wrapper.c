/*---------------------------------------------------------*/
/*   doom_tick_wrapper.c - Game Loop Adapter              */
/*   Wraps D_DoomLoop for frame-by-frame execution        */
/*---------------------------------------------------------*/

#include "doomgeneric.h"
#include "doom.h"  // For D_DoomTick

//
// This file is kept for future extensions if needed.
// The actual D_DoomTick() implementation is now in d_main.c
// as part of the vendored doom-ascii sources.
//
// D_DoomTick() is called by TDoomAsciiView::advance() every 33ms
// and executes one frame iteration of the game loop.
//
