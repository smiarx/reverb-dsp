#include "springreverb.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 512
#define R 1000

int main(int argc, char **argv)
{
    int r = R, n = N;

    --argc;
    ++argv;

    while (argc > 0) {
        if (strcmp(*argv, "-n") == 0) {
            n = atoi(*++argv);
            --argc;
        } else if (strcmp(*argv, "-r") == 0) {
            r = atoi(*++argv);
            --argc;
        }
        ++argv;
        --argc;
    }

    float samplerate    = 48000;
    springs_desc_t desc = {
        .ftr    = {4210, 4106, 4200, 4300, 4330, 4118, 4190, 4310},
        .stages = {LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N,
                   LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N},
        .a1     = {0.18, 0.21, 0.312, 0.32, 0.32, 0.23, 0.21, 0.2},
        .ahigh  = {-0.63, -0.56, -0.83, -0.37, -0.67, -0.48, -0.76, -0.32},
        //.Td = {0.0552,0.04366,0.04340,0.04370,.0552,0.04423,0.04367,0.0432},
        .length  = {0.052, 0.054, 0.046, 0.048, 0.050, 0.05612, 0.04983,
                    0.051291},
        .fcutoff = {40, 40, 38, 20, 49, 31, 32, 33},
        .gripple = {0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01},
        .gecho   = {0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01},
        .t60     = {2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f},
    };

    float ins[NCHANNELS][8192];
    float outs[NCHANNELS][8192];
    float *in[2], *out[2];
    float y[MAXSPRINGS];

    for (int c = 0; c < NCHANNELS; ++c) {
        in[c]  = &ins[c][0];
        out[c] = &outs[c][0];
    }

    memset(ins, 0, sizeof(ins));

    springs_t springs;
    springs_init(&springs, &desc, samplerate, n);

    printf("%d tests of %d samples\n", r, n);

    double time = 0, dtime = 0;
    struct low_cascade *lc = &springs.low_cascade;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        low_cascade_process(lc, y, NO_INCREMENT);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("low allpasschain: %.5f\n", time);

    time = 0, dtime = 0;
    struct low_delayline *ldl = &springs.low_delayline;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        low_delayline_process(ldl, y, 0, 0);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("low delay line: %.5f\n", time);

    time = 0, dtime = 0;
    struct low_dc *dc = &springs.low_dc;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        low_dc_process(dc, y);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("low dc filter: %.5f\n", time);

    time = 0, dtime = 0;
    struct low_eq *le = &springs.low_eq;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        low_eq_process(le, y);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("low eq: %.5f\n", time);

    time = 0, dtime = 0;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        springs_lowlpf(&springs, y);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("low lpf: %.5f\n", time);

    time                    = 0;
    dtime                   = 0;
    struct high_cascade *hc = &springs.high_cascade;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        high_cascade_process(hc, y, NO_INCREMENT);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("high allpass: %.5f\n", time);

    time                       = 0;
    dtime                      = 0;
    struct high_delayline *hdl = &springs.high_delayline;
    for (int i = 0; i < n * r; i++) {
        dtime = -omp_get_wtime();
        high_delayline_process(hdl, y, 0, 0);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("high delayline: %.5f\n", time);

    time  = 0;
    dtime = 0;
    for (int i = 0; i < r; i++) {
        dtime = -omp_get_wtime();
        springs_process(&springs, in, out, n);
        dtime += omp_get_wtime();
        time += dtime;
    }
    printf("springs process: %.5f\n", time);
}
