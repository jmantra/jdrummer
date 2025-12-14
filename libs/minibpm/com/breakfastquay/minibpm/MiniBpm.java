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

package com.breakfastquay.minibpm;

/**
 * A fixed-tempo BPM estimator. This Java wrapper currently provides
 * only the single-shot API from the C++ library.
 *
 * A single channel of audio only may be supplied (multi-channel is
 * not supported). To process multi-channel audio, average the
 * channels first.
 */
public class MiniBpm
{
    public MiniBpm(int sampleRate) {
	handle = 0;
	initialise(sampleRate);
    }

    /**
     * Call this to dispose of the tempo estimator after use. You must
     * do so after you have finished using this estimator object, as
     * the Java garbage collector cannot collect the native object.
     */
    public native void dispose();

    /**
     * Set the range of valid tempi. The default is 55-190bpm.
     */
    public native void setBPMRange(double min, double max);

    /**
     * Get the current minimum supported tempo.
     */
    public native double getBPMMin();

    /**
     * Get the current maximum supported tempo.
     */
    public native double getBPMMax();

    /**
     * Set the number of beats per bar, if known. If unknown, leave at
     * the default (which is 4).
     */
    public native void setBeatsPerBar(int bpb);

    /**
     * Get the current number of beats per bar. (This simply returns
     * the value set by setBeatsPerBar, which is a hint to the tempo
     * estimator: the estimator does not estimate meter.)
     */
    public native int getBeatsPerBar();

    /**
     * Return the estimated tempo in bpm of the music audio in the
     * given sequence of samples. nsamples contains the number of
     * samples, starting at the given offset in the given input
     * array. If the tempo cannot be estimated because the clip is too
     * short, return 0.
     *
     * The input samples are expected to be in the range [-1,1].
     *
     * If you wish to subsequently obtain a tempo estimate of a
     * different clip, you must call reset() before calling this
     * function again. (This is so that the data that is available to
     * be returned from getTempoCandidates can be cleared.)
     */
    public native double estimateTempoOfSamples(float[] input,
						int offset, int nsamples);

    /**
     * Return all candidate tempi for the last tempo estimation that
     * was carried out, in order of likelihood (best first). The value
     * returned from estimateTempoOfSamples will therefore be the
     * first in the returned sequence. Calling reset() will clear this
     * information.
     */
    public native double[] getTempoCandidates();

    /**
     * Prepare the object to carry out another tempo estimation on a
     * new audio clip. You can either call this between uses, or
     * simply destroy this object and construct a new one.
     */
    public native void reset();

    private native void initialise(int sampleRate);
    private long handle;

    static {
	System.loadLibrary("minibpm-jni");
    }
};


    
