#include "tapedelay.h"
#include "fastmath.h"

void tapedelay_tap(tapedelay_t *restrict tapedelay, tap_t *tap,
                   const enum tape_direction direction,
                   float y[restrict NCHANNELS]);
void tapedelay_output(tapedelay_t *restrict tapedelay,
                      float y[restrict NCHANNELS], float **restrict in,
                      float **restrict out, int n);

// return true if x > y
int comparegt(int64_t x, int64_t y)
{
    /* transform into unsigned to allow overflow */
    uint64_t tmp = -y;
    int64_t diff = x + tmp;
    return diff > 0;
}

void tapedelay_init(tapedelay_t *tapedelay, tapedelay_desc_t *desc,
                    float samplerate)
{
    memset(tapedelay, 0, sizeof(tapedelay_t));

    tapedelay->desc = *desc;

    tapedelay->direction = FORWARDS;

    tapedelay->samplerate                  = samplerate;
    tapedelay->ringbuffer[DELAYSIZE - 1].V = -DELAYUNIT;
    tapedelay->ringbuffer[0].V             = -DELAYUNIT;
    tapedelay->nwrite                      = 2;

    tapedelay->tap[0].xrevstop = -DELAYUNIT / 2;

    tapedelay->desc = *desc;
    tapedelay_set_delay(tapedelay, desc->delay);
}

void tapedelay_update(tapedelay_t *tapedelay, tapedelay_desc_t *desc)
{
#define param_update(name)                               \
    {                                                    \
        if (tapedelay->desc.name != desc->name)          \
            tapedelay_set_##name(tapedelay, desc->name); \
    }
    param_update(delay);
    param_update(reverse);
    param_update(cutoff);
#undef param_update
}

void tapedelay_set_delay(tapedelay_t *tapedelay, float delay)
{
    tapedelay->desc.delay = delay;
    tapedelay->speed = ((float)DELAYUNIT) / (delay * tapedelay->samplerate);
}

void tapedelay_set_reverse(tapedelay_t *tapedelay, float reverse)
{
    tapedelay->desc.reverse = reverse;

    if (reverse == 0.f) {
        tapedelay->direction = FORWARDS;
    } else {
        tapedelay->direction       = BACKWARDS;
        uint64_t xwrite            = tapedelay->ringbuffer[tapedelay->nwrite].V;
        tapedelay->tap[0].xread    = xwrite;
        tapedelay->tap[0].xrevstop = xwrite - DELAYUNIT / 2;
        tapedelay->tap[0].nread    = tapedelay->nwrite;
    }
}

void tapedelay_set_cutoff(tapedelay_t *tapedelay, float cutoff)
{
    tapedelay->desc.cutoff = cutoff;
    filter(_butterworth_lp)(&tapedelay->lowpassfilter, tapedelay->samplerate,
                            cutoff);
}

inline void tapedelay_tap(tapedelay_t *restrict tapedelay, tap_t *tap,
                          const enum tape_direction direction,
                          float y[restrict NCHANNELS])
{
    uint64_t xread = tap->xread;

    size_t nread   = tap->nread;
    int searchsize = 2 * direction;

    /* find minimum binary search size */
    for (uint64_t xsearch;
         xsearch = tapedelay->ringbuffer[(nread + searchsize) & DELAYMASK].V,
         direction == FORWARDS ? comparegt(xread, xsearch)
                               : comparegt(xsearch, xread);
         nread += searchsize, searchsize *= 2) {
    }

    /* binary search */
    while (direction * searchsize > 1) {
        size_t searchsize2 = searchsize / 2;
        uint64_t xsearch =
            tapedelay->ringbuffer[(nread + searchsize2) & DELAYMASK].V;
        if (direction == FORWARDS ? comparegt(xread, xsearch)
                                  : comparegt(xsearch, xread))
            nread += searchsize2;
        searchsize = searchsize2;
    }
    nread &= DELAYMASK;

    uint64_t xfound  = tapedelay->ringbuffer[nread].V;
    uint64_t xfound1 = tapedelay->ringbuffer[(nread + direction) & DELAYMASK].V;
    float fnread     = direction == FORWARDS
                           ? ((float)(xread - xfound)) / (xfound1 - xfound)
                           : ((float)(xread - xfound1)) / (xfound - xfound1);

    float ym1[NCHANNELS], y0[NCHANNELS], y1[NCHANNELS], y2[NCHANNELS];

#if AA == 1
    float playbackspeed =
        direction == FORWARDS
            ? ((nread - tap->nread) & DELAYMASK) + fnread - tap->prev_fnread
            : ((tap->nread - nread) & DELAYMASK) - (fnread - tap->prev_fnread);
    tap->prev_fnread = fnread;

    if (direction == BACKWARDS) fnread = 1.f - fnread;

    if (playbackspeed <= 1.01) {
        if (tap->nread != nread) {
            tap->nread = nread;
            for (int c = 0; c < NCHANNELS; ++c) {
                tap->ym1[c] = tap->y0[c], tap->y0[c] = tap->y1[c],
                tap->y1[c] = tap->y2[c];
                tap->y2[c] =
                    tapedelay
                        ->ringbuffer[(tap->nread + 2 * direction) & DELAYMASK]
                        .y[c];
            }
        }
    } else {
        /* lowpass factor */
        float a = 1. / playbackspeed;
        do {
            tap->nread = (tap->nread + direction) & DELAYMASK;
            for (int c = 0; c < NCHANNELS; ++c) {
                tap->ym1[c] = tap->y0[c], tap->y0[c] = tap->y1[c],
                tap->y1[c] = tap->y2[c];
                /* filter */
                tap->y2[c] =
                    (1 - a) *
                        (tap->y2[c] + tap->y1[c] + tap->y0[c] + tap->ym1[c]) /
                        4. +
                    a * tapedelay
                            ->ringbuffer[(tap->nread + 2 * direction) &
                                         DELAYMASK]
                            .y[c];
            }
        } while (direction == FORWARDS ? comparegt(nread, tap->nread)
                                       : comparegt(tap->nread, nread));
    }
    for (int c = 0; c < NCHANNELS; ++c) {
        ym1[c] = tap->ym1[c];
        y0[c]  = tap->y0[c];
        y1[c]  = tap->y1[c];
        y2[c]  = tap->y2[c];
    }

#else
    tap->nread = nread;
    for (int c = 0; c < NCHANNELS; ++c) {
        ym1[c] = tapedelay->ringbuffer[(nread - direction) & DELAYMASK].y[c];
        y0[c]  = tapedelay->ringbuffer[nread].y[c];
        y1[c]  = tapedelay->ringbuffer[(nread + direction) & DELAYMASK].y[c];
        y2[c] = tapedelay->ringbuffer[(nread + 2 * direction) & DELAYMASK].y[c];
    }
#endif

    for (int c = 0; c < NCHANNELS; ++c)
        y[c] = hermite(ym1[c], y0[c], y1[c], y2[c], fnread);
}

inline void tapedelay_output(tapedelay_t *restrict tapedelay,
                             float y[restrict NCHANNELS], float **restrict in,
                             float **restrict out, int n)
{
    /* apply lowpass to tapeoutput */
    filter(_process)(&tapedelay->lowpassfilter, y, 1);

    for (int c = 0; c < NCHANNELS; ++c) {
        int c0 = c;
#if NCHANNELS == 2
        if (tapedelay->desc.pingpong) c0 ^= 1;
#endif
        tapedelay->ringbuffer[tapedelay->nwrite].y[c0] =
            in[c][n] + (tapedelay->desc.feedback * y[c]);

        float drywet = tapedelay->desc.drywet;
        out[c][n]    = drywet * y[c] + (1.f - drywet) * in[c][n];
    }
}

void tapedelay_process(tapedelay_t *restrict tapedelay, float **restrict in,
                       float **restrict out, int count)
{
    float y[NCHANNELS];
    tap_t *tap = &tapedelay->tap[tapedelay->tap_id];

    if (tapedelay->direction == FORWARDS) {
        for (int n = 0; n < count; ++n) {
            tap_t *tap    = tapedelay->tap;
            size_t nwrite = tapedelay->nwrite;
            uint64_t xwrite =
                tapedelay->ringbuffer[nwrite].V + tapedelay->speed;

            uint64_t xread = xwrite - DELAYUNIT;
            tap->xread     = xread;
            tapedelay_tap(tapedelay, tap, FORWARDS, y);
            tapedelay_output(tapedelay, y, in, out, n);

            tapedelay->nwrite = (tapedelay->nwrite + 1) & DELAYMASK;
            tapedelay->ringbuffer[tapedelay->nwrite].V = xwrite;
        }
    } else if (tapedelay->direction == BACKWARDS) {
        for (int n = 0; n < count; ++n) {
            size_t nwrite  = tapedelay->nwrite;
            int64_t xwrite = tapedelay->ringbuffer[nwrite].V + tapedelay->speed;

            uint64_t xread = tap->xread -= tapedelay->speed;
            if (comparegt(tap->xrevstop, xread)) {
                xread = tap->xread += DELAYUNIT;
                tap->xrevstop += DELAYUNIT / 2;
                tap->nread       = nwrite;
                tap->prev_fnread = 0.f;
            }
            tapedelay_tap(tapedelay, tap, BACKWARDS, y);
            tapedelay_output(tapedelay, y, in, out, n);

            tapedelay->nwrite = (tapedelay->nwrite + 1) & DELAYMASK;
            tapedelay->ringbuffer[tapedelay->nwrite].V = xwrite;
        }
    }
}
