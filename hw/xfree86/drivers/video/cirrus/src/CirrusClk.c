#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Programming of the built-in Cirrus clock generator.
 * Harm Hanemaayer <hhanemaa@cs.ruu.nl>
 *
 * VCO stability criterion code added by Koen Gadeyne (koen.gadeyne@barco.com)
 * Max clock specification added by Harm Hanemaayer (H.Hanemaayer@inter.nl.net)
 *
 * Minor changes and cleanup Itai Nahshon.
 *
 * Made this completely chipset independent,  and moved chipset dependent parts
 * into the specific sub-drivers.  Derek Fawcus <derek@spider.com>
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"

#include "cir.h"

/* CLOCK_FACTOR is double the osc freq in kHz (osc = 14.31818 MHz) */
#define CLOCK_FACTOR 28636

/* Stability constraints for internal VCO -- MAX_VCO also determines
 * the maximum Video pixel clock
 */
#define MIN_VCO CLOCK_FACTOR
#define MAX_VCO 111000

/* clock in kHz is (numer * CLOCK_FACTOR / (denom & 0x3E)) >> (denom & 1) */
#define VCOVAL(n, d) \
	((((n) & 0x7F) * CLOCK_FACTOR / ((d) & 0x3E)) )

#define CLOCKVAL(n, d) \
	(VCOVAL(n, d) >> ((d) & 1))

typedef struct {
	unsigned char numer;
	unsigned char denom;
} cirrusClockRec;

static cirrusClockRec cirrusClockTab[] = {
	{ 0x2C, 0x33 },		/* 12.599 */
	{ 0x4A, 0x2B },		/* 25.227 */
	{ 0x5B, 0x2F },		/* 28.325 */
	{ 0x45, 0x30 },		/* 41.164 */
	{ 0x7E, 0x33 },		/* 36.082 */
	{ 0x42, 0x1F },		/* 31.500 */
	{ 0x51, 0x3A },		/* 39.992 */
	{ 0x55, 0x36 },		/* 45.076 */
	{ 0x65, 0x3A },		/* 49.867 */
	{ 0x76, 0x34 },		/* 64.983 */
	{ 0x7E, 0x32 },		/* 72.163 */
	{ 0x6E, 0x2A },		/* 75.000 */
	{ 0x5F, 0x22 },		/* 80.013 */
	{ 0x7D, 0x2A },		/* 85.226 */
	{ 0x58, 0x1C },		/* 89.998 */
	{ 0x49, 0x16 },		/* 95.019 */
	{ 0x46, 0x14 },		/* 100.226 */
	{ 0x53, 0x16 },		/* 108.035 */
	{ 0x5C, 0x18 },		/* 110.248 */

	{ 0x6D, 0x1A },		/* 120.050 */
	{ 0x58, 0x14 },		/* 125.998 */
	{ 0x6D, 0x18 },		/* 130.055 */
	{ 0x42, 0x0E },		/* 134.998 */

	{ 0x69, 0x14 },		/* 150.341 */
	{ 0x5E, 0x10 },		/* 168.239 */
	{ 0x5C, 0x0E },		/* 188.182 */
	{ 0x67, 0x0E },		/* 210.682 */
	{ 0x60, 0x0C },		/* 229.091 */
};

#define NU_FIXED_CLOCKS (sizeof(cirrusClockTab)/sizeof(cirrusClockTab[0]))


/*
 * This function returns the 7-bit numerator and 6-bit denominator/post-scalar
 * value that corresponds to the closest clock found.
 * If a frequency close to one of the tested clock values is found,
 * use the tested clock since others can be unstable.
 */

_X_EXPORT Bool
CirrusFindClock(int *rfreq, int max_clock, int *num_out, int *den_out)
{
	int n, i;
	int num = 0, den = 0;
	int freq, ffreq = 0, mindiff = 0;

	freq = *rfreq;
	/* Prefer a tested value if it matches within 0.1%. */
	for (i = 0; i < NU_FIXED_CLOCKS; i++) {
		int diff;
		diff = abs(CLOCKVAL(cirrusClockTab[i].numer,
					cirrusClockTab[i].denom) - freq);
		if (diff < freq / 1000) {
			num = cirrusClockTab[i].numer;
			den = cirrusClockTab[i].denom;
			ffreq = CLOCKVAL(num, den);
			goto foundclock;
		}
	}

	/*
	 * If max_clock is greater than the MAX_VCO default, ignore
	 * MAX_VCO. On the other hand, if MAX_VCO is higher than max_clock,
	 * make use of the higher MAX_VCO value.
	 */
	if (MAX_VCO > max_clock)
		max_clock = MAX_VCO;

	mindiff = freq;
	for (n = 0x10; n < 0x7f; n++) {
		int d;
		for (d = 0x14; d < 0x3f; d++) {
			int c, diff;

			/* Avoid combinations that can be unstable. */
			if ((VCOVAL(n, d) < MIN_VCO) || (VCOVAL(n, d) > max_clock))
				continue;
			c = CLOCKVAL(n, d);
			diff = abs(c - freq);
			if (diff < mindiff) {
				mindiff = diff;
				num = n;
				den = d;
				ffreq = c;
			}
		}
	}

	if (0 == num || 0 == den)
		return FALSE;

foundclock:
	*num_out = num;
	*den_out = den;
	*rfreq = ffreq;

	return TRUE;
}
