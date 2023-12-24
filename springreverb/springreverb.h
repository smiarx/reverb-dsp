#ifndef _SPRINGREVERB_H
#define _SPRINGREVERB_H

#ifndef MAXSPRINGS
#define MAXSPRINGS 8
#endif
#ifndef NSPRINGS
#define NSPRINGS MAXSPRINGS
#endif

#ifndef STEREO
#ifndef MONO
#define STEREO
#endif
#endif
#ifdef STEREO
#define NCHANNELS 2
#else
#define NCHANNELS 1
#endif

#define BUFFERSIZE (1 << 12)
#define BUFFERMASK (BUFFERSIZE - 1)

#define MLOW        120
#define MLOWBUFSIZE 32
#define MLOWBUFMASK (MLOWBUFSIZE - 1)

#define LOWDELAY1SIZE      2048
#define LOWDELAY1MASK      (LOWDELAY1SIZE - 1)
#define LOWDELAYECHOSIZE   512
#define LOWDELAYECHOMASK   (LOWDELAYECHOSIZE - 1)
#define LOWDELAYRIPPLESIZE 128
#define LOWDELAYRIPPLEMASK (LOWDELAYRIPPLESIZE - 1)

#define gecho   0.01f
#define gripple 0.01f
#define glf     0.95f

#define amod 0.997f
#define gmod 8.3f

#define LOWPASSN2ND    5 // number of lowpass 2nd order filter
#define LOWPASSMEMSIZE 4
#define LOWPASSMEMMASK (LOWPASSMEMSIZE - 1)

#define springparam(typescal, name) typescal name[MAXSPRINGS];

typedef struct {
    /* set with ftr */
    springparam(float, K);
    springparam(int, iK);
    springparam(float, a2);
    /*--------*/

    springparam(float, a1);

    /* modulation */
    springparam(int, randseed);
    springparam(int, randstate);
    springparam(float, Lmodmem);

    struct {
        springparam(float, mem2);
        springparam(float, mem1[MLOWBUFSIZE]);
    } lowstate[MLOW];
    int lowbufid;

    springparam(float, L1);
    springparam(float, Lecho);
    springparam(float, Lripple);
    springparam(float, lowdelay1[LOWDELAY1SIZE]);
    springparam(float, lowdelayecho[LOWDELAYECHOSIZE]);
    springparam(float, lowdelayripple[LOWDELAYRIPPLESIZE]);
    int lowdelay1id;
    int lowdelayechoid;
    int lowdelayrippleid;

    springparam(float, lowpassmem[LOWPASSN2ND + 1][LOWPASSMEMSIZE]);
    int lowpassmemid;

    float samplerate;
} springs_t;

void springs_init(springs_t *springs, float samplerate);
void springs_set_ftr(springs_t *springs, float *ftr);
void springs_set_a1(springs_t *springs, float *a1);
void springs_set_Td(springs_t *springs, float *Td);
void springs_set_Nripple(springs_t *springs, float Nripple);

void springs_lowdelayline(springs_t *restrict springs, float *restrict y);
void springs_lowallpasschain(springs_t *restrict springs, float *restrict y);
void springs_lowlpf(springs_t *restrict springs, float *restrict y);
void springs_process(springs_t *restrict springs, float **restrict in,
                     float **restrict out, int count);

#endif
