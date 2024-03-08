#ifndef _SPRINGREVERB_H
#define _SPRINGREVERB_H

#ifndef MAXSPRINGS
#define MAXSPRINGS 8

#endif
#ifndef NSPRINGS
#define NSPRINGS MAXSPRINGS
#endif

/* include filter functions */
#undef FILTER_VECSIZE
#include "filter.h"
#define FILTER_VECSIZE NSPRINGS

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

#define LOW_CASCADE_N          120
#define LOW_CASCADE_STATE_SIZE 32
#define LOW_CASCADE_STATE_MASK (LOW_CASCADE_STATE_SIZE - 1)

#define LOW_EQ_STATE_SIZE (LOW_CASCADE_STATE_SIZE * 2)
#define LOW_EQ_STATE_MASK (LOW_EQ_STATE_SIZE - 1)

#define LOWDELAY1SIZE      (2048 << 2)
#define LOWDELAY1MASK      (LOWDELAY1SIZE - 1)
#define LOWDELAYECHOSIZE   (512 << 2)
#define LOWDELAYECHOMASK   (LOWDELAYECHOSIZE - 1)
#define LOWDELAYRIPPLESIZE (128 << 2)
#define LOWDELAYRIPPLEMASK (LOWDELAYRIPPLESIZE - 1)

#define gmod 10.f
#define amod (1.f - 0.05f / gmod)

#define HIGH_CASCADE_N       70
#define HIGH_CASCADE_STRETCH 2

#define HIGHDELAYSIZE LOWDELAY1SIZE
#define HIGHDELAYMASK (HIGHDELAYSIZE - 1)

#define glow2high (0.0f)
#define ghigh2low (0.0f)

// filter
#define FILTERMEMSIZE 4
#define FILTERMEMMASK (FILTERMEMSIZE - 1)
#define NAASOS        5 // number of aa 2nd order filter
#define NLOWPASSSOS   5 // number of lowpass 2nd order filter

#define MAXBLOCKSIZE 512

#define RMS_BUFFER_SIZE  512
#define RMS_BUFFER_MASK  (RMS_BUFFER_SIZE - 1)
#define RMS_SIZE         2048
#define RMS_OVERLAP_SIZE (128)
#define RMS_OVERLAP_MASK (RMS_OVERLAP_SIZE - 1)
#define RMS_NOVERLAPS    (RMS_SIZE / RMS_OVERLAP_SIZE)

typedef float springsfloat[MAXSPRINGS]
    __attribute__((aligned(sizeof(float) * MAXSPRINGS)));
typedef int springsint[MAXSPRINGS]
    __attribute__((aligned(sizeof(int) * MAXSPRINGS)));
typedef unsigned int springsuint[MAXSPRINGS]
    __attribute__((aligned(sizeof(unsigned int) * MAXSPRINGS)));

struct springparam {
    springsfloat val;
    springsfloat inc;
};

enum inc {
    NO_INCREMENT = 0,
    INCREMENT    = 1,
};

/* spring parameters */
typedef struct {
    springsfloat ftr;
    springsuint stages;
    springsfloat a1;
    springsfloat ahigh;
    springsfloat length;
    springsfloat fcutoff;
    springsfloat gripple;
    springsfloat gecho;
    springsfloat t60;
    springsfloat chaos;
    springsfloat vol;
    springsint mute;
    springsint solo;
    springsfloat hilomix;
    springsint source;
    springsfloat pan;
    float drywet;
} springs_desc_t;

/* low dc filter */
struct low_dc {
    springsfloat b;
    springsfloat a;
    springsfloat state;
};

/* low allpass cascade */
struct low_cascade {
    springsuint stages;
    unsigned int max_stages;
    int min_stages;
    springsint iK;
    struct springparam a1;
    springsfloat a2;
    struct {
        springsfloat s2;
        springsfloat s1[LOW_CASCADE_STATE_SIZE];
    } state[LOW_CASCADE_N];
    int id;
};

/* low equalizer */
struct low_eq {
    springsint Keq;
    springsfloat b0;
    springsfloat ak;
    springsfloat a2k;
    springsfloat state[LOW_EQ_STATE_SIZE];
    int id;
};

/* high cascade */
struct high_cascade {
    struct springparam a;
    springsfloat state[HIGH_CASCADE_N][HIGH_CASCADE_STRETCH];
};

/* perlin noise */
struct noise {
    springsint seed;
    springsint state;
    springsfloat amount;
    springsfloat y[3];
    springsfloat c[4];
    float phase;
    float freq;
};
/* delay line */
struct delay_tap {
    springsint idelay;
    springsfloat fdelay;
    int id;
};

struct low_delayline {
    struct noise noise;

    struct springparam L1;
    struct delay_tap tap1;
    springsfloat buffer1[LOWDELAY1SIZE];

    struct springparam Lecho;
    struct delay_tap tap_echo;
    springsfloat buffer_echo[LOWDELAYECHOSIZE];
    springsfloat gecho;

    struct springparam Lripple;
    struct delay_tap tap_ripple;
    springsfloat buffer_ripple[LOWDELAYRIPPLESIZE];
    springsfloat gripple;

    struct springparam glf;
};

struct high_delayline {
    struct noise noise;

    struct springparam L;
    struct delay_tap tap;
    springsfloat buffer[HIGHDELAYSIZE];

    struct springparam ghf;
};

struct rms {
    springsfloat yoverlap[RMS_NOVERLAPS];
    springsfloat rms[RMS_BUFFER_SIZE];
    int overlap_n, rms_id;
};

typedef struct {
    springs_desc_t desc;

    /* block compute */
    int blocksize;
    springsfloat ylow[MAXBLOCKSIZE];
    springsfloat yhigh[MAXBLOCKSIZE];

    /* down sampling */
    int downsampleM;
    int downsampleid;
    filter(_t) aafilter[NAASOS];

    /* set with ftr */
    springsfloat K;
    /*--------*/

    struct low_dc low_dc;
    struct low_cascade low_cascade;
    enum inc increment_a1;

    struct low_delayline low_delayline;
    enum inc increment_delaytime, increment_t60;
    struct high_delayline high_delayline;

    struct low_eq low_eq;

    filter(_t) lowpassfilter[NLOWPASSSOS];

    struct high_cascade high_cascade;
    enum inc increment_ahigh;

    /* low and high gain */
    struct springparam glow;
    struct springparam ghigh;
    enum inc increment_glowhigh;

    struct springparam gchannel[NCHANNELS];
    enum inc increment_gchannel;

    struct rms rms;

    float drywet;
    float drywet_inc;

    float samplerate;
} springs_t;

void springs_init(springs_t *springs, springs_desc_t *desc, float samplerate,
                  int count);
void springs_update(springs_t *springs, springs_desc_t *desc);
void springs_set_dccutoff(springs_t *springs, float fcutoff[MAXSPRINGS]);
void springs_set_ftr(springs_t *springs, float ftr[MAXSPRINGS], int count);
void springs_set_stages(springs_t *restrict springs,
                        unsigned int stages[MAXSPRINGS]);
void springs_set_a1(springs_t *springs, float a1[MAXSPRINGS], int count);
void springs_set_length(springs_t *springs, float length[MAXSPRINGS],
                        int count);
void springs_set_ahigh(springs_t *springs, float a1[MAXSPRINGS], int count);
void springs_set_gripple(springs_t *springs, float gripple[MAXSPRINGS]);
void springs_set_gecho(springs_t *springs, float gecho[MAXSPRINGS]);
void springs_set_t60(springs_t *springs, float t60[MAXSPRINGS], int count);
void springs_set_chaos(springs_t *springs, float chaos[MAXSPRINGS], int count);
void springs_set_vol(springs_t *springs, float vol[MAXSPRINGS], int count);
void springs_set_solo(springs_t *restrict springs, int solo[MAXSPRINGS],
                      int count);
void springs_set_mute(springs_t *restrict springs, int mute[MAXSPRINGS],
                      int count);
void springs_set_pan(springs_t *springs, float pan[MAXSPRINGS], int count);
void springs_set_drywet(springs_t *springs, float drywet, int count);
void springs_set_hilomix(springs_t *springs, float hilomix[MAXSPRINGS],
                         int count);

void low_delayline_process(struct low_delayline *dl, float y[MAXSPRINGS],
                           enum inc inc_delaytime, enum inc inc_t60);
void low_dc_process(struct low_dc *dc, float y[MAXSPRINGS]);
void low_cascade_process(struct low_cascade *lc, float y[MAXSPRINGS],
                         enum inc inc_a1);
void springs_lowlpf(springs_t *springs, float y[MAXSPRINGS]);
void low_eq_process(struct low_eq *le, float y[MAXSPRINGS]);
void high_cascade_process(struct high_cascade *hc, float y[MAXSPRINGS],
                          enum inc inc_ahigh);
void high_delayline_process(struct high_delayline *dl, float y[MAXSPRINGS],
                            enum inc inc_delaytime, enum inc inc_t60);
void rms_process(struct rms *rms, const float y[][MAXSPRINGS], int count);
void springs_process(springs_t *springs, const float *const *in,
                     float *const *out, int count);

#endif
