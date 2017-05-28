#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void hexColor(unsigned int col, double *a, double *r, double *g,
              double *b)
{
    unsigned char rr, gg, bb, aa;

    bb = (col & 0xff) ;
    gg = ((col & 0x0000ff00) >> 8) ;
    rr = ((col & 0x00ff0000) >> 16) ;
    aa = ((col & 0xff000000) >> 24) ;

    *a = (1.0 / 256.0) * (double)aa;
    *r = (1.0 / 256.0) * (double)rr;
    *g = (1.0 / 256.0) * (double)gg;
    *b = (1.0 / 256.0) * (double)bb;


    return;
}


int main(int argc, char **argv)
{
    unsigned int hexcol;
    double a, r, g, b;


    if (argc != 2)
    {
        fprintf(stderr,
                "usage: hex2float <color>, where color is an argb hex number\n");

        exit(1);
    }

    if (sscanf(argv[1], "%x", &hexcol) != 1)
    {
        fprintf(stderr, "sscanf faild.\n");

        fprintf(stderr,
                "usage: hex2float <color>, where color is an argb hex number\n");

        exit(1);
    }

    hexColor(hexcol, &a, &r, &g, &b);

    printf("%0.3f %0.3f %0.3f %0.3f\n",
           a, r, b, g);

    exit(0);
}
