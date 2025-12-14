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

#include "MiniBpmVamp.h"

#include "MiniBpm.h"

using std::string;

namespace breakfastquay {

MiniBpmVamp::MiniBpmVamp(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_mbpm(new MiniBPM(inputSampleRate)),
    m_beatsPerBar(4)
{
}

MiniBpmVamp::~MiniBpmVamp()
{
    delete m_mbpm;
}

string
MiniBpmVamp::getIdentifier() const
{
    return "minibpm";
}

string
MiniBpmVamp::getName() const
{
    return "MiniBPM Detector";
}

string
MiniBpmVamp::getDescription() const
{
    return "Return a single estimated tempo in BPM based on sum of autocorrelations of separate low- and high-frequency bands";
}

string
MiniBpmVamp::getMaker() const
{
    // Your name here
    return "Breakfast Quay";
}

int
MiniBpmVamp::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 2;
}

string
MiniBpmVamp::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "GPL";
}

MiniBpmVamp::InputDomain
MiniBpmVamp::getInputDomain() const
{
    return TimeDomain;
}

size_t
MiniBpmVamp::getPreferredBlockSize() const
{
    return 0;
}

size_t 
MiniBpmVamp::getPreferredStepSize() const
{
    return 0;
}

size_t
MiniBpmVamp::getMinChannelCount() const
{
    return 1;
}

size_t
MiniBpmVamp::getMaxChannelCount() const
{
    return 1;
}

MiniBpmVamp::ParameterList
MiniBpmVamp::getParameterDescriptors() const
{
    ParameterList list;

    ParameterDescriptor d;

    d.identifier = "bpb";
    d.name = "Beats per bar";
    d.description = "Number of beats per bar, if known. If not known, leave this set to 4.";
    d.unit = "beats";
    d.minValue = 1;
    d.maxValue = 12;
    d.defaultValue = 4;
    d.isQuantized = true;
    d.quantizeStep = 1;
    list.push_back(d);

    return list;
}

float
MiniBpmVamp::getParameter(string identifier) const
{
    if (identifier == "bpb") {
	return m_beatsPerBar;
    }
    return 0.f;
}

void
MiniBpmVamp::setParameter(string identifier, float value) 
{
    if (identifier == "bpb") {
	m_beatsPerBar = int(value + 0.5);
    }
}

MiniBpmVamp::ProgramList
MiniBpmVamp::getPrograms() const
{
    ProgramList list;
    return list;
}

string
MiniBpmVamp::getCurrentProgram() const
{
    return ""; // no programs
}

void
MiniBpmVamp::selectProgram(string name)
{
}

MiniBpmVamp::OutputList
MiniBpmVamp::getOutputDescriptors() const
{
    OutputList outputs;

    OutputDescriptor d;
    d.identifier = "bpm";
    d.name = "Tempo";
    d.description = "Single estimated tempo in beats per minute";
    d.unit = "bpm";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = m_inputSampleRate;
    d.hasDuration = true;
    outputs.push_back(d);

    d = OutputDescriptor();
    d.identifier = "candidates";
    d.name = "Tempo Candidates";
    d.description = "A multi-valued feature containing all tempo candidates starting with the most likely";
    d.hasFixedBinCount = false;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = m_inputSampleRate;
    d.hasDuration = true;
    outputs.push_back(d);

    return outputs;
}

bool
MiniBpmVamp::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    m_blockSize = blockSize;

    reset();

    return true;
}

void
MiniBpmVamp::reset()
{
    m_mbpm->reset();
    m_mbpm->setBeatsPerBar(m_beatsPerBar);
    m_lastTimestamp = Vamp::RealTime::zeroTime;
}

MiniBpmVamp::FeatureSet
MiniBpmVamp::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_mbpm->process(inputBuffers[0], m_blockSize);
    m_lastTimestamp = timestamp;
    return FeatureSet();
}

MiniBpmVamp::FeatureSet
MiniBpmVamp::getRemainingFeatures()
{
    FeatureSet fs;

    double bpm = m_mbpm->estimateTempo();

    Feature f;
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;
    f.hasDuration = true;
    f.duration = m_lastTimestamp +
        Vamp::RealTime::frame2RealTime(m_blockSize, m_inputSampleRate);
    f.values.push_back(bpm);
    fs[0].push_back(f);

    std::vector<double> candidates = m_mbpm->getTempoCandidates();

    f = Feature();
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;
    f.hasDuration = true;
    f.duration = m_lastTimestamp +
        Vamp::RealTime::frame2RealTime(m_blockSize, m_inputSampleRate);
    for (size_t i = 0; i < candidates.size(); ++i) {
        f.values.push_back(candidates[i]);
    }
    fs[1].push_back(f);
    
    return fs;
}

}

