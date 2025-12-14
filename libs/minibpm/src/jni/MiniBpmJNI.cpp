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

#include "MiniBpm.h"

#include <jni.h>

using namespace breakfastquay;

extern "C" {
/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    dispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breakfastquay_minibpm_MiniBpm_dispose
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    setBPMRange
 * Signature: (DD)V
 */
JNIEXPORT void JNICALL Java_com_breakfastquay_minibpm_MiniBpm_setBPMRange
  (JNIEnv *, jobject, jdouble, jdouble);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    getBPMMin
 * Signature: ()D
 */
JNIEXPORT jdouble JNICALL Java_com_breakfastquay_minibpm_MiniBpm_getBPMMin
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    getBPMMax
 * Signature: ()D
 */
JNIEXPORT jdouble JNICALL Java_com_breakfastquay_minibpm_MiniBpm_getBPMMax
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    setBeatsPerBar
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_breakfastquay_minibpm_MiniBpm_setBeatsPerBar
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    getBeatsPerBar
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_breakfastquay_minibpm_MiniBpm_getBeatsPerBar
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    estimateTempoOfSamples
 * Signature: ([FII)D
 */
JNIEXPORT jdouble JNICALL Java_com_breakfastquay_minibpm_MiniBpm_estimateTempoOfSamples
  (JNIEnv *, jobject, jfloatArray, jint, jint);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    getTempoCandidates
 * Signature: ()[D
 */
JNIEXPORT jdoubleArray JNICALL Java_com_breakfastquay_minibpm_MiniBpm_getTempoCandidates
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breakfastquay_minibpm_MiniBpm_reset
  (JNIEnv *, jobject);

/*
 * Class:     com_breakfastquay_minibpm_MiniBpm
 * Method:    initialise
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_breakfastquay_minibpm_MiniBpm_initialise
  (JNIEnv *, jobject, jint);


}

MiniBPM *
getMiniBPM(JNIEnv *env, jobject obj)
{
    jclass c = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(c, "handle", "J");
    jlong handle = env->GetLongField(obj, fid);
    return (MiniBPM *)handle;
}

void
setMiniBPM(JNIEnv *env, jobject obj, MiniBPM *minibpm)
{
    jclass c = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(c, "handle", "J");
    jlong handle = (jlong)minibpm;
    env->SetLongField(obj, fid, handle);
}

JNIEXPORT void JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_initialise(JNIEnv *env, jobject obj, jint sampleRate)
{
    setMiniBPM(env, obj, new MiniBPM(sampleRate));
}

JNIEXPORT void JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_dispose(JNIEnv *env, jobject obj)
{
    delete getMiniBPM(env, obj);
    setMiniBPM(env, obj, 0);
}

JNIEXPORT void JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_setBPMRange(JNIEnv *env, jobject obj, jdouble min, jdouble max)
{
    getMiniBPM(env, obj)->setBPMRange(min, max);
}

JNIEXPORT jdouble JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_getBPMMin(JNIEnv *env, jobject obj)
{
    double min, max;
    getMiniBPM(env, obj)->getBPMRange(min, max);
    return min;
}

JNIEXPORT jdouble JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_getBPMMax(JNIEnv *env, jobject obj)
{
    double min, max;
    getMiniBPM(env, obj)->getBPMRange(min, max);
    return max;
}

JNIEXPORT void JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_setBeatsPerBar(JNIEnv *env, jobject obj, jint bpb)
{
    getMiniBPM(env, obj)->setBeatsPerBar(bpb);
}

JNIEXPORT jint JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_getBeatsPerBar(JNIEnv *env, jobject obj)
{
    return getMiniBPM(env, obj)->getBeatsPerBar();
}

JNIEXPORT jdouble JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_estimateTempoOfSamples(JNIEnv *env, jobject obj, jfloatArray data, jint offset, jint n)
{
    float *arr = env->GetFloatArrayElements(data, 0);

    double estimate =
        getMiniBPM(env, obj)->estimateTempoOfSamples(arr + offset, n);

    env->ReleaseFloatArrayElements(data, arr, 0);
    
    return estimate;
}

JNIEXPORT jdoubleArray JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_getTempoCandidates(JNIEnv *env, jobject obj)
{
    std::vector<double> candidates = getMiniBPM(env, obj)->getTempoCandidates();
    jdoubleArray out = env->NewDoubleArray(candidates.size());
    env->SetDoubleArrayRegion(out, 0, candidates.size(), &candidates[0]);
    return out;
}

JNIEXPORT void JNICALL
Java_com_breakfastquay_minibpm_MiniBpm_reset(JNIEnv *env, jobject obj)
{
    getMiniBPM(env, obj)->reset();
}
