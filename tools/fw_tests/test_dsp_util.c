#include "../firmware/dsp_util.h"
#include "math.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    int j[] = {65535, 32767, 32768, 0};

    for (int k = 0; k < 4; k++) {
        double theta = ((double)j[k] * 2. * 3.14159265358979) / (65536.);
        double d = sin(theta);
        int16_t fixed_point = sine_u16(j[k]);
        double test = (double)(fixed_point / (double)(1 << 15));

        printf("itheta = %i, theta = %f; %-7.5f %-7.5f, delta = %-7.5f\r\n", j[k], theta, d, test, d - test);
    }
}
