#include "butterlp.h"

ButterLP* ButterLP_Init(double* data, double fs, double fc){
    ButterLP* obj = (ButterLP*)malloc(sizeof(ButterLP));
    memset(obj, 0, sizeof(ButterLP));
    obj->data = data;

    double a = tan(3.141592653589 * fc / fs);

    // 采样双线性变换求解滤波器参数
    if(BUTTER_LEVEL == 1){
        obj->a[0] = 1;
        obj->a[1] = (a - 1) / (a + 1);
        obj->b[0] = a / (a + 1);
        obj->b[1] = obj->b[0];
    } else if(BUTTER_LEVEL == 2){
        obj->a[0] = a * a + sqrt(2) * a + 1;
        obj->a[1] = (2 * a * a - 2) / obj->a[0];
        obj->a[2] = (a * a - sqrt(2) * a + 1) / obj->a[0];
        obj->b[0] = a * a / obj->a[0];
        obj->b[1] = 2 * obj->b[1];
        obj->b[2] = obj->b[0];
        obj->a[0] = 1;
    }

    return obj;
}

// 将当前的数据作为u0，计算返回y0
double ButterLP_Step(ButterLP* obj){
    for(int i = 0; i < BUTTER_LEVEL; i++){
        obj->u[i + 1] = obj->u[i];
        obj->y[i + 1] = obj->y[i];
    }
    obj->u[0] = *obj->data;
    obj->y[0] = obj->u[0] * obj->b[0];
    for(int i = 1; i <= BUTTER_LEVEL; i++){
        obj->y[0]  += obj->u[i] * obj->b[i];
        obj->y[0]  -= obj->y[i] * obj->a[i];
    }
    return obj->y[0];
}