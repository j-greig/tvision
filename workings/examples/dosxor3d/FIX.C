#include <alloc.h>
#include <math.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include "vga.h"

#define FIX_PREC 9
#define TO_FIX(x) ((long)((x)*(1<<FIX_PREC)))
#define TO_DBL(x) (((double)x)/(double)(1<<FIX_PREC))
#define TO_LONG(x) ((x)/(1<<FIX_PREC))

#define SIN_SIZE 512
#define COS_OFF 128

long SIN[ SIN_SIZE + COS_OFF ];
long *COS = SIN + COS_OFF;

void init_sin()
{
    int i;
    double v;
    for(i = 0; i < SIN_SIZE + COS_OFF; ++i) {
        v = sin(2.0 * M_PI * i / (double)SIN_SIZE);
        SIN[i] = TO_FIX( v );
    }
}

long fix_mul(long a, long b)
{
    return (a * b) >> FIX_PREC;
}

long fix_sqr(long a)
{
    return (a * a) >> FIX_PREC;
}

long fix_div(long a, long b)
{
    return (a << FIX_PREC) / b;
}

#include "pyramid.h"
#include "cube.h"
#define MAX_VTX 64

void
draw_cube(unsigned int t, int type)
{
    const long *vtxX, *vtxY, *vtxZ;
    const int *edges;
    size_t numVtx, numEdge;
    long cubeRotX[MAX_VTX], cubeRotY[MAX_VTX], cubeRotZ[MAX_VTX];
    long tempY, tempZ;
    long cubeProjX[MAX_VTX], cubeProjY[MAX_VTX];
    int i, e1, e2, x1, y1, x2, y2;
    long a = TO_FIX(2), scale = TO_FIX(40);

    switch( type)
    {
    case 0:
        vtxX = cubeX;
        vtxY = cubeY;
        vtxZ = cubeZ;
        edges = cubeEdges;
        numVtx = cubeVtx;
        numEdge = cubeEdge;
        break;
    default:
        vtxX = pyramidX;
        vtxY = pyramidY;
        vtxZ = pyramidZ;
        edges = pyramidEdges;
        numVtx = pyramidVtx;
        numEdge = pyramidEdge;
        break;
    }


    for(i = 0; i < numVtx; ++i)
    {
        /* Rotation around Y */
        cubeRotX[i] =   fix_mul(vtxX[i], COS[t%SIN_SIZE])
            + fix_mul(vtxZ[i], SIN[t%SIN_SIZE]);
        cubeRotY[i] = vtxY[i];
        cubeRotZ[i] = - fix_mul(vtxX[i], SIN[t%SIN_SIZE])
            + fix_mul(vtxZ[i], COS[t%SIN_SIZE]);
        /* Rotation around X */
        tempY = fix_mul(cubeRotY[i], COS[t%SIN_SIZE]) - fix_mul(cubeRotZ[i], SIN[t%SIN_SIZE]);
        tempZ = fix_mul(cubeRotY[i], SIN[t%SIN_SIZE]) + fix_mul(cubeRotZ[i], COS[t%SIN_SIZE]);
        cubeRotY[i] = tempY;
        cubeRotZ[i] = tempZ;
        /* Translate further away */
	cubeRotZ[i] += TO_FIX(4) + COS[t%SIN_SIZE];
        /* projection to screen space*/
        cubeProjX[i] = fix_div( fix_mul(a, cubeRotX[i]), cubeRotZ[i] );
        cubeProjY[i] = fix_div( fix_mul(a, cubeRotY[i]), cubeRotZ[i] );
    }
    for(i = 0; i < numEdge; i+=2)
    {
        e1 = edges[i]; e2 = edges[i+1];
        x1 = TO_LONG(fix_mul(cubeProjX[e1], scale)) + (vga_width>>1);
        y1 = TO_LONG(fix_mul(cubeProjY[e1], scale)) + (vga_height>>1);
        x2 = TO_LONG(fix_mul(cubeProjX[e2], scale)) + (vga_width>>1);
        y2 = TO_LONG(fix_mul(cubeProjY[e2], scale)) + (vga_height>>1);

        draw_line_xor(x1, y1, x2, y2, 15);
    }
}

int main()
{
    unsigned int t = 0;
    dword i = 0;
    char kc = 0, dt = 2;
    int type = 0;

    init_sin();
    set_graphics_mode();
    randomize();
    for(i=0; i < (dword)SCREEN_HEIGHT * SCREEN_WIDTH; ++i)
	BUF[i] = (rand() % 2) * 15;
    while(kc != 0x1b) {
	if(kbhit()) kc=getch();
	switch( kc ) {
	case '1':
            type = 0;
            break;
	case '2':
            type = 1;
            break;
	case 'q':
	    dt = 0;
	    break;
	case 'w':
	    dt = 2;
	    break;
	default:
	    break;
	}
        wait_for_retrace();
	if( dt != 0 ) {
	    draw_cube(t, type);
	    t += dt;
	}
    }
    set_text_mode();

    return 0;
}
