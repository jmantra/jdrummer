# jdrummer Groove Matching and Tempo Estimation

This document explains how groove matching and tempo estimation work
inside **jdrummer**.

------------------------------------------------------------------------

## Overview

jdrummer analyzes an incoming drum or percussion audio signal in
real‑time or offline to:

1.  Estimate the tempo (BPM)
2.  Detect beat onsets
3.  Extract rhythmic features
4.  Match the extracted groove against the internal groove library
5.  Suggest the closest‑matching drum pattern

The system works primarily on transient‑rich material such as drums and
rhythmic percussion.

------------------------------------------------------------------------

## Signal Pre‑Processing

Before analysis, the input signal goes through:

### 1. Mono Conversion

Stereo inputs are summed into mono to simplify onset and envelope
detection.

### 2. Normalization

The signal amplitude is normalized to avoid level‑dependent bias in
onset detection.

### 3. High‑pass Filtering

Low‑frequency rumble is removed to emphasize percussive transients.

------------------------------------------------------------------------

## Onset Detection

Onset detection identifies **when drum hits occur**.

jdrummer uses a combination of:

-   Energy envelope tracking
-   Spectral flux
-   Adaptive thresholding
-   Peak picking with hysteresis

### Steps

1.  Compute short‑time Fourier transform (STFT)
2.  Measure change in spectral magnitude between frames
3.  Accumulate changes to form an **onset detection function (ODF)**
4.  Apply adaptive thresholding to detect peaks
5.  Convert frame indices to timestamps

The result is a list of timestamps:

    t₀, t₁, t₂, … tₙ

representing when rhythmic events occurred.

------------------------------------------------------------------------

## Tempo Estimation

Tempo is estimated in **beats per minute (BPM)**.

### Method

1.  Compute inter‑onset intervals (IOIs):

```{=html}
<!-- -->
```
    Δt = tᵢ − tᵢ₋₁

2.  Build histogram of IOIs
3.  Apply autocorrelation
4.  Select dominant periodicity
5.  Convert to BPM:

```{=html}
<!-- -->
```
    BPM = 60 / Δt

### Beat Tracking Refinement

-   Phase alignment
-   Outlier rejection (removes ghost hits)
-   Integer ratio snapping (half‑time / double‑time resolution)

Final output includes:

-   Estimated BPM
-   Beat grid locations

------------------------------------------------------------------------

## Groove Feature Extraction

Grooves are represented as **timing feature vectors**.

For each bar:

-   Normalize beat positions to range 0--1
-   Quantize to 16th‑note resolution
-   Measure timing deviation from grid

Features include:

-   Onset timing offsets
-   Inter‑onset interval patterns
-   Velocity/amplitude profile
-   Swing ratio estimation

This produces a **groove signature vector**.

------------------------------------------------------------------------

## Groove Matching

Groove matching compares the extracted signature against the built‑in
groove library.

### Distance Metrics Used

-   Euclidean distance\
-   Dynamic Time Warping (DTW) for tempo‑flexible comparison\
-   Weighted onset timing & velocity features

Closest match is selected:

    match = argmin distance(g_user, g_library[i])

Results can include:

-   best groove match
-   top‑N similarity list
-   confidence score

------------------------------------------------------------------------

## Handling Tempo Differences

Grooves in the library are stored tempo‑independent by:

-   expressing timing as beat fractions
-   normalizing duration of measures

That means:

A 90 BPM groove and a 140 BPM groove with the same **feel** will match
correctly.

------------------------------------------------------------------------

## Output

The system produces:

-   Estimated BPM
-   Groove name
-   Similarity score


jdrummer can then:

-   auto‑load the groove
-   synchronize to host tempo
-   optionally quantize to the user groove

------------------------------------------------------------------------

## Limitations

Best results occur with:

-   isolated drums or percussion
-   clear transient information
-   minimal reverb

Less accurate for:

-   polyphonic music
-   highly syncopated ambient material
-   brushed drums / soft onset content

------------------------------------------------------------------------

## Summary

Groove matching in jdrummer:

-   detects drum hit timing
-   estimates tempo
-   builds rhythmic feature vectors
-   compares them against groove library entries
-   returns the closest match

This allows users to: - extract grooves from real recordings - match
performances to drum patterns - build human‑like rhythm arrangements
