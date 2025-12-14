/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    MiniBPM
    A fixed-tempo BPM estimator for music audio
    Copyright 2012-2025 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for MiniBPM
    obtained by agreement with the copyright holders, you may
    redistribute and/or modify it under the terms described in that
    licence.

    If you wish to distribute code using MiniBPM under terms other
    than those of the GNU General Public License, you must obtain a
    valid commercial licence before doing so.
*/

#include "../src/MiniBpm.cpp"

using breakfastquay::FourierFilterbank;
using breakfastquay::Autocorrelation;
using breakfastquay::ACFCombFilter;
using breakfastquay::MiniBPM;

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TestFourierFilterbank)

#define FFT(n) FourierFilterbank(n, 2, 0, 1, false)
#define COMPARE_ZERO(x) BOOST_CHECK_SMALL(x, 1e-10)
#define COMPARE_ALL_ZERO(a) \
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        COMPARE_ZERO(a[cmp_i]); \
    }

BOOST_AUTO_TEST_CASE(outputSize)
{
    int size = FFT(4).getOutputSize();
    BOOST_CHECK_EQUAL(size, 3);
    size = FourierFilterbank(4, 4, 1, 1, false).getOutputSize();
    BOOST_CHECK_EQUAL(size, 1);
    size = FourierFilterbank(4, 4, 1, 2, false).getOutputSize();
    BOOST_CHECK_EQUAL(size, 2);
    size = FourierFilterbank(512, 44100, 0, 440, true).getOutputSize();
    BOOST_CHECK_EQUAL(size, 6);
}

BOOST_AUTO_TEST_CASE(dc)
{
    double in[] = { 1, 1, 1, 1 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    BOOST_CHECK_EQUAL(mag[0], 4.0);
    COMPARE_ZERO(mag[1]);
    COMPARE_ZERO(mag[2]);
}

BOOST_AUTO_TEST_CASE(sine)
{
    double in[] = { 0, 1, 0, -1 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    COMPARE_ZERO(mag[0]);
    BOOST_CHECK_EQUAL(mag[1], 2.0);
    COMPARE_ZERO(mag[2]);
}

BOOST_AUTO_TEST_CASE(sineSubset)
{
    double in[] = { 0, 1, 0, -1 };
    double mag[1];
    FourierFilterbank(4, 4, 1, 1, false).forwardMagnitude(in, mag);
    BOOST_CHECK_EQUAL(mag[0], 2.0);
}

BOOST_AUTO_TEST_CASE(cosine)
{
    double in[] = { 1, 0, -1, 0 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    COMPARE_ZERO(mag[0]);
    BOOST_CHECK_EQUAL(mag[1], 2.0);
    COMPARE_ZERO(mag[2]);
}
	
BOOST_AUTO_TEST_CASE(sineCosine)
{
    double in[] = { 0.5, 1, -0.5, -1 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    COMPARE_ZERO(mag[0]);
    BOOST_CHECK_CLOSE(mag[1], sqrt(5.0), 1e-10);
    COMPARE_ZERO(mag[2]);
}

BOOST_AUTO_TEST_CASE(nyquist)
{
    double in[] = { 1, -1, 1, -1 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    COMPARE_ZERO(mag[0]);
    COMPARE_ZERO(mag[1]);
    BOOST_CHECK_EQUAL(mag[2], 4.0);
}

BOOST_AUTO_TEST_CASE(dirac)
{
    double in[] = { 1, 0, 0, 0 };
    double mag[3];
    FFT(4).forwardMagnitude(in, mag);
    BOOST_CHECK_EQUAL(mag[0], 1.0);
    BOOST_CHECK_EQUAL(mag[1], 1.0);
    BOOST_CHECK_EQUAL(mag[2], 1.0);
}

BOOST_AUTO_TEST_CASE(forwardArrayBounds)
{
    double in[] = { 1, 1, -1, -1 };
    double mag[] = { 999, 999, 999, 999, 999 };
    FFT(4).forwardMagnitude(in, mag + 1);
    BOOST_CHECK_EQUAL(mag[0], 999.0);
    BOOST_CHECK_EQUAL(mag[4], 999.0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestAutocorrelation)

BOOST_AUTO_TEST_CASE(unnormalised)
{
    double in[] = { 1,0,0, 1,0,0, 1,0,0, 1,0,0 };
    double out[] = { 0,0,0, 0,0,0, 0,0,0, 0,0,0 };
    Autocorrelation(12, 12).acf(in, out);
    BOOST_CHECK_EQUAL(out[0], 4.0);
    BOOST_CHECK_EQUAL(out[1], 0.0);
    BOOST_CHECK_EQUAL(out[2], 0.0);
    BOOST_CHECK_EQUAL(out[3], 3.0);
    BOOST_CHECK_EQUAL(out[4], 0.0);
    BOOST_CHECK_EQUAL(out[5], 0.0);
    BOOST_CHECK_EQUAL(out[6], 2.0);
    BOOST_CHECK_EQUAL(out[7], 0.0);
    BOOST_CHECK_EQUAL(out[8], 0.0);
    BOOST_CHECK_EQUAL(out[9], 1.0);
    BOOST_CHECK_EQUAL(out[10], 0.0);
    BOOST_CHECK_EQUAL(out[11], 0.0);
}

BOOST_AUTO_TEST_CASE(normalisedUnity)
{
    double in[] = { 1,0,0, 1,0,0, 1,0,0, 1,0,0 };
    double out[] = { 0,0,0, 0,0,0, 0,0,0, };
    Autocorrelation(12, 9).acfUnityNormalised(in, out);
    BOOST_CHECK_EQUAL(out[0], 1.0);
    BOOST_CHECK_EQUAL(out[1], 0.0);
    BOOST_CHECK_EQUAL(out[2], 0.0);
    BOOST_CHECK_EQUAL(out[3], 1.0);
    BOOST_CHECK_EQUAL(out[4], 0.0);
    BOOST_CHECK_EQUAL(out[5], 0.0);
    BOOST_CHECK_EQUAL(out[6], 1.0);
    BOOST_CHECK_EQUAL(out[7], 0.0);
    BOOST_CHECK_EQUAL(out[8], 0.0);
}

BOOST_AUTO_TEST_CASE(bpmLagConversion)
{
    int lag = Autocorrelation::bpmToLag(120.0, 2.0);
    BOOST_CHECK_EQUAL(lag, 1);
    lag = Autocorrelation::bpmToLag(130.0, 2.0);
    BOOST_CHECK_EQUAL(lag, 1);
    lag = Autocorrelation::bpmToLag(130.0, 100.0);
    BOOST_CHECK_EQUAL(lag, 46);

    double bpm = Autocorrelation::lagToBpm(1, 2.0);
    BOOST_CHECK_EQUAL(bpm, 120.0);
    bpm = Autocorrelation::lagToBpm(50, 100.0);
    BOOST_CHECK_EQUAL(bpm, 120.0);
    bpm = Autocorrelation::lagToBpm(46, 100.0);
    BOOST_CHECK_CLOSE(bpm, 130.0, 0.5);
    bpm = Autocorrelation::lagToBpm(0.5, 1.0);
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.1);

    bpm = Autocorrelation::lagToBpm(1, 1.0);
    BOOST_CHECK_EQUAL(bpm, 60.0);
    bpm = Autocorrelation::lagToBpm(2, 1.0);
    BOOST_CHECK_EQUAL(bpm, 30.0);
    bpm = Autocorrelation::lagToBpm(3, 1.0);
    BOOST_CHECK_EQUAL(bpm, 20.0);
    bpm = Autocorrelation::lagToBpm(4, 1.0);
    BOOST_CHECK_EQUAL(bpm, 15.0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestACFCombFilter)

BOOST_AUTO_TEST_CASE(contributingRange)
{
    int base, count;
    ACFCombFilter::getContributingRange(1, 1, base, count);
    BOOST_CHECK_EQUAL(base, 1);
    BOOST_CHECK_EQUAL(count, 1);
    ACFCombFilter::getContributingRange(1, 2, base, count);
    BOOST_CHECK_EQUAL(base, 2);
    BOOST_CHECK_EQUAL(count, 1);
    ACFCombFilter::getContributingRange(1, 4, base, count);
    BOOST_CHECK_EQUAL(base, 3);
    BOOST_CHECK_EQUAL(count, 3);
    ACFCombFilter::getContributingRange(1, 8, base, count);
    BOOST_CHECK_EQUAL(base, 6);
    BOOST_CHECK_EQUAL(count, 6);
}

BOOST_AUTO_TEST_CASE(contributingRangeCoverage)
{
    const int n = 64;
    const int maxMultiple = 16;
    int *target = new int[n];

    // Need to ensure that each lag contributes to no more than one
    // target lag, at each multiple
    for (int multiple = 1; multiple <= maxMultiple; multiple = multiple * 2) {
	for (int i = 1; i < n; ++i) {
	    target[i] = -1;
	}
	for (int i = 1; i < n; ++i) {
	    int base, count;
	    ACFCombFilter::getContributingRange(i, multiple, base, count);
	    for (int j = base; j < base + count; ++j) {
		BOOST_CHECK_GE(j, i);
		if (j >= 0 && j < n) {
		    if (target[j] != -1) {
			BOOST_CHECK_EQUAL(target[j], i);
		    }
		    target[j] = i;
		}
	    }
	}
    }

    // and that each lag is a source for only one multiple, for a
    // given target
    for (int i = 1; i < n; ++i) {
	for (int j = 1; j < n; ++j) {
	    target[j] = -1;
	}
	for (int multiple = 1; multiple <= maxMultiple; multiple = multiple * 2) {
	    int base, count;
	    ACFCombFilter::getContributingRange(i, multiple, base, count);
	    for (int j = base; j < base + count; ++j) {
		BOOST_CHECK_GE(j, i);
		if (j >= 0 && j < n) {
		    if (target[j] != -1) {
			BOOST_CHECK_EQUAL(target[j], multiple);
		    }
		    target[j] = multiple;
		}
	    }
	}
    }		

    delete[] target;
}

#define REFINER(n) ACFCombFilter(4, 10, 20, n)

BOOST_AUTO_TEST_CASE(refineSingleElement)
{
    double acf[] = { 0.0, 1.0 };
    double bpm = REFINER(1).refine(1, acf, 2);
    BOOST_CHECK_EQUAL(bpm, 60);
}

BOOST_AUTO_TEST_CASE(refineEmpty)
{
    double acf[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    double bpm = REFINER(1).refine(1, acf, 8);
    BOOST_CHECK_EQUAL(bpm, 60);
}
    
BOOST_AUTO_TEST_CASE(refineTrivial)
{
    double acf[] = { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    double bpm = REFINER(1).refine(1, acf, 8);
    BOOST_CHECK_EQUAL(bpm, 60);
}
    
BOOST_AUTO_TEST_CASE(refineConsistent)
{
    double acf[] = { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 };
    double bpm = REFINER(1).refine(1, acf, 8);
    BOOST_CHECK_EQUAL(bpm, 60);
}

BOOST_AUTO_TEST_CASE(refineAdjusting)
{
    double acf[] = { 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
    double bpm = REFINER(1).refine(1, acf, 8);
    BOOST_CHECK_EQUAL(bpm, 48);
}

BOOST_AUTO_TEST_SUITE_END()

static float *tap(double duration, double rate, double bpm, int &len)
{
    double bps = bpm / 60.0;
    len = int(duration * rate + 0.5);
    float *data = new float[len];
    for (int i = 0; i < len; ++i) {
	data[i] = 0.f;
    }
    int beats = int(bps * duration + 0.5);
    for (int i = 0; i < beats; ++i) {
	int ix = (i / bps) * rate;
	if (ix < len) data[ix] = 1.f;
    }
    return data;
}

BOOST_AUTO_TEST_SUITE(TestMiniBpm)

BOOST_AUTO_TEST_CASE(tapper)
{
    // Testing our test harness
    int len = 0;
    float *data = tap(2.0, 20.0, 240.0, len);
    BOOST_CHECK_EQUAL(len, 40);
    for (int i = 0; i < 8; ++i) {
	BOOST_CHECK_EQUAL(data[i * 5], 1.f);
	for (int j = 1; j < 5; ++j) {
	    BOOST_CHECK_EQUAL(data[i * 5 + j], 0.f);
	}
    }
    delete[] data;
}

BOOST_AUTO_TEST_CASE(complete_120bpm_8000)
{
    float rate = 8000.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    double bpm = MiniBPM(rate).estimateTempoOfSamples(data, len);
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
}

BOOST_AUTO_TEST_CASE(complete_120bpm_44100)
{
    float rate = 44100.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    double bpm = MiniBPM(rate).estimateTempoOfSamples(data, len);
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
}

BOOST_AUTO_TEST_CASE(complete_120bpm_48000)
{
    float rate = 48000.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    double bpm = MiniBPM(rate).estimateTempoOfSamples(data, len);
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
}

BOOST_AUTO_TEST_CASE(buffered_120bpm_44100)
{
    float rate = 44100.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    MiniBPM mbpm(rate);
    int blockSize = 1000;
    for (int i = 0; i < len/blockSize; ++i) {
	int thisBlock = blockSize;
	if (i * blockSize + thisBlock > len) {
	    thisBlock = len - i * blockSize;
	}
	mbpm.process(data + i * blockSize, thisBlock);
    }
    delete[] data;
    double bpm = mbpm.estimateTempo();
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
}

BOOST_AUTO_TEST_CASE(minmax)
{
    MiniBPM mbpm(44100);
    double min = 0.0, max = 0.0;
    mbpm.getBPMRange(min, max);
    BOOST_CHECK_NE(min, 0.0);
    BOOST_CHECK_NE(max, 0.0);
    BOOST_CHECK_GT(max, min);
    BOOST_CHECK_GT(min, 30.0);
    BOOST_CHECK_GT(max, 30.0);
    BOOST_CHECK_LT(min, 240.0);
    BOOST_CHECK_LT(max, 240.0);
    mbpm.setBPMRange(70, 140);
    mbpm.getBPMRange(min, max);
    BOOST_CHECK_EQUAL(min, 70);
    BOOST_CHECK_EQUAL(max, 140);
}

BOOST_AUTO_TEST_CASE(complete_240bpm_outOfRange)
{
    // 240bpm is too fast for default range; should return 120
    float rate = 44100.f;
    int len;
    float *data = tap(6.0, rate, 240.0, len);
    double bpm = MiniBPM(rate).estimateTempoOfSamples(data, len);
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
}

BOOST_AUTO_TEST_CASE(complete_120bpm_outOfRange)
{
    // adjust range to make 120bpm also too fast; should return 60
    float rate = 44100.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    MiniBPM mbpm(rate);
    mbpm.setBPMRange(30, 90);
    double bpm = mbpm.estimateTempoOfSamples(data, len);
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 60.0, 0.25);
}

BOOST_AUTO_TEST_CASE(candidates)
{
    float rate = 44100.f;
    int len;
    float *data = tap(6.0, rate, 120.0, len);
    MiniBPM mbpm(rate);
    double bpm = mbpm.estimateTempoOfSamples(data, len);
    double min, max;
    mbpm.getBPMRange(min, max);
    std::vector<double> candidates = mbpm.getTempoCandidates();
    delete[] data;
    BOOST_CHECK_CLOSE(bpm, 120.0, 0.25);
    BOOST_CHECK(candidates.size() > 1);
    BOOST_CHECK_CLOSE(candidates[0], 120.0, 0.25);
    BOOST_CHECK_CLOSE(candidates[1], 60.0, 0.25);
    for (int i = 0; i < candidates.size(); ++i) {
	BOOST_CHECK_GE(candidates[i], min);
	BOOST_CHECK_LE(candidates[i], max);
    }
}

BOOST_AUTO_TEST_SUITE_END()
