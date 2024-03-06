#include <springreverb.h>
#include <stdlib.h>
#include <unity.h>

void setUp(void) {}

void tearDown(void) {}

void test_rms(void)
{
#define TESTSIZE RMS_SIZE*RMS_NOVERLAPS
    /* test input */
    springsfloat y[TESTSIZE];
    for (int n = 0; n < TESTSIZE; ++n) {
        for (int i = 0; i < MAXSPRINGS; ++i) {
            y[n][i] = (float)rand() / RAND_MAX;
        }
    }
    const int nrms = TESTSIZE / RMS_OVERLAP_SIZE;

    /* expected result */
    springsfloat expect[nrms];
    for (int i = 0; i < MAXSPRINGS; ++i) {
        for (int k = 0; k < nrms; ++k) {
            const int nend = (k + 1) * RMS_OVERLAP_SIZE;
            int nstart     = nend - RMS_SIZE;
            nstart         = nstart < 0 ? 0 : nstart;
            float sum      = 0.f;
            for (int n = nstart; n < nend; ++n) {
                sum += y[n][i] * y[n][i];
            }
            expect[k][i] = sqrtf(sum / RMS_SIZE);
        }
    }

    /* test one call */
    {
        struct rms rms = {0};
        rms_process(&rms, y, TESTSIZE);
        TEST_ASSERT_EQUAL_INT(nrms, rms.rms_id);
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(&expect[0][0], &rms.rms[0][0],
                                      nrms * MAXSPRINGS);
    }

    /* test smaller blocks calls */
    {
        const int blocksize = 8;
        struct rms rms      = {0};
        for (int j = 0; j < TESTSIZE / blocksize; ++j)
            rms_process(&rms, y + j * blocksize, blocksize);
        ;
        TEST_ASSERT_EQUAL_INT(nrms, rms.rms_id);
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(&expect[0][0], &rms.rms[0][0],
                                      nrms * MAXSPRINGS);
    }
#undef TESTSIZE
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rms);
    return UNITY_END();
}
