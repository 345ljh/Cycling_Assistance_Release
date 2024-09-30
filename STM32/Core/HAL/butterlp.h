#ifndef _BUTTERLP_H
#define _BUTTERLP_H

#include "math.h"
#include "malloc.h"
#include "string.h"

#define BUTTER_LEVEL 1 // 滤波器阶数

typedef struct butterlp_t
{
    double* data;  // 进行滤波的数据

    /*
        y       b0 + b1z^-1 + ...
        - = --------------------------  ->   y0 = (b0u0 + b1u1 + ...) - (a1u1 + ...)
        u    1 + a1z^-1 + a2z^-2 + ...
    */
    double b[BUTTER_LEVEL + 1];
    double a[BUTTER_LEVEL + 1];

    double u[BUTTER_LEVEL + 1];
    double y[BUTTER_LEVEL + 1];
}ButterLP;

ButterLP* ButterLP_Init(double* data, double fs, double fc);
double ButterLP_Step(ButterLP* obj);

#endif