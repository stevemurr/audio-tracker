/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "pitch_detector/pitch_detector.h"
#include <string>     // std::string, std::to_string

AudioProcessor* JUCE_CALLTYPE createPluginFilter();

//==============================================================================
JuceDemoPluginAudioProcessor::JuceDemoPluginAudioProcessor()
    : AudioProcessor (getBusesProperties()),
      lastUIWidth (400),
      lastUIHeight (200)
{
    lastPosInfo.resetToDefault();
    
    rmsThreshold = -50.0;
    silenceThreshold = 22050;
}

JuceDemoPluginAudioProcessor::~JuceDemoPluginAudioProcessor()
{
}

//==============================================================================
bool JuceDemoPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only mono/stereo and input/output must have same layout
    const AudioChannelSet& mainOutput = layouts.getMainOutputChannelSet();
    
    // input and output layout must be the same
    if (layouts.getMainInputChannelSet() != mainOutput)
        return false;

    // do not allow disabling the main buses
    if (mainOutput.isDisabled()) return false;

    // only allow stereo and mono
    if (mainOutput.size() > 2) return false;

    return true;
}

AudioProcessor::BusesProperties JuceDemoPluginAudioProcessor::getBusesProperties()
{
    return BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), true)
                            .withOutput ("Output", AudioChannelSet::stereo(), true);
}

//==============================================================================
void JuceDemoPluginAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void JuceDemoPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void JuceDemoPluginAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
}

void JuceDemoPluginAudioProcessor::analyze(const float * buffer, const int numSamples, gistResults *g) {
    Gist<float> gist(numSamples, 44100);
    gist.processAudioFrame(buffer, numSamples);
    
    g->f0 = gist.pitch();
    g->rms = gist.rootMeanSquare();
    g->rmsDB = Decibels::gainToDecibels(g->rms);
    g->peakEnergy = gist.peakEnergy();
    g->zeroCrossings = gist.zeroCrossingRate();
    g->spectralCentroid = gist.spectralCentroid();
    g->spectralCrest = gist.spectralCrest();
    g->spectralFlatness = gist.spectralFlatness();
    g->spectralRolloff = gist.spectralRolloff();
    g->spectralKurtosis = gist.spectralKurtosis();
    g->energyDifference = gist.energyDifference();
    g->spectralDifference = gist.spectralDifference();
    g->spectralDifferenceHalfWaveRectified = gist.spectralDifferenceHWR();
    g->complexSpectralDifference = gist.complexSpectralDifference();
    g->highFrequencyContent = gist.highFrequencyContent();
}

static String timeToTimecodeString (double seconds)
{
    const int millisecs = roundToInt (seconds * 1000.0);
    const int absMillisecs = std::abs (millisecs);
    
    return String::formatted ("%02d:%02d:%02d.%03d",
                              millisecs / 360000,
                              (absMillisecs / 60000) % 60,
                              (absMillisecs / 1000) % 60,
                              absMillisecs % 1000);
}

void JuceDemoPluginAudioProcessor::post(juce::String url, juce::String endpoint) {
    RestRequest::RestRequest request(url);
    RestRequest::RestRequest::Response response = request.post (endpoint)
    .field ("f0", result_f0)
    .field("centroid", result_spectralCentroid)
    .field("rms", result_rms)
    .field("startedAt", timeToTimecodeString(startedTime))
    .field("endedAt", endedAt)
    .field("localTime", Time::currentTimeMillis())
    .execute();
}

void JuceDemoPluginAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);
    
    // Retrive this blocks audio features and place them in g.
    gistResults g;
    analyze((const float*)buffer.getReadPointer(0), numSamples, &g);
    
    if (!started && g.rmsDB >= rmsThreshold) {
        // If we haven't detected an audio file previously,
        // but rms exceeds threshold for silence, start accumulating feature values.
        started = true;
        blocksHeard = 1;
        silenceHeard = 0;
        
        accum_f0 = 0;
        accum_rms = 0;
        accum_centroid = 0;
        accum_kurtosis = 0;
        accum_zcr = 0;
        accum_specCrest = 0;
        accum_specFlat = 0;
        accum_specRolloff = 0;
        
        startedTime = lastPosInfo.timeInSeconds;
        
    
    } else if (started && silenceHeard < silenceThreshold) {
        // If we have started but we haven't reached a silence limit
        
        if (g.rmsDB >= rmsThreshold)
            silenceHeard = 0;

        silenceHeard += numSamples;
        blocksHeard += 1;
        
        if (g.f0 >= 60.0 && g.f0 <= 600.0 )
            accum_f0 += g.f0;
        
        accum_rms += g.rmsDB;
        accum_centroid += g.spectralCentroid;
        accum_kurtosis += g.spectralKurtosis;
        accum_zcr += g.zeroCrossings;
        accum_specCrest += g.spectralCrest;
        accum_specRolloff += g.spectralRolloff;
        accum_specFlat += g.spectralFlatness;
        accum_peakEnergy += g.peakEnergy;
        
    } else if (started && silenceHeard >= silenceThreshold) {
        // If we have started and exceeded silence limit.
        
        blocksHeard += 1;
        
        result_f0 = accum_f0 / blocksHeard;
        result_spectralCentroid = accum_centroid / blocksHeard;
        result_rms = accum_rms / blocksHeard;
        result_zcr = accum_zcr / blocksHeard;
        result_spectralKurtosis = accum_kurtosis / blocksHeard;
        result_spectralCrest = accum_specCrest / blocksHeard;
        result_spectralRolloff = accum_specRolloff / blocksHeard;
        result_peakEnergy = accum_peakEnergy / blocksHeard;
        result_spectralFlatness = accum_specFlat / blocksHeard;
        
        endedAt = timeToTimecodeString(lastPosInfo.timeInSeconds);
        
        post("http://localhost:9091", "api/audio");
        
        started = false;
    }
    // Update time.
    updateCurrentTimeInfoFromHost();
}

double JuceDemoPluginAudioProcessor::getTimeInSeconds() {
    if (AudioPlayHead* ph = getPlayHead())
    {
        AudioPlayHead::CurrentPositionInfo newTime;
        
        if (ph->getCurrentPosition (newTime))
        {
            return newTime.timeInSeconds;
        }
    }
    return 0.0;
}

void JuceDemoPluginAudioProcessor::updateCurrentTimeInfoFromHost()
{
    if (AudioPlayHead* ph = getPlayHead())
    {
        AudioPlayHead::CurrentPositionInfo newTime;

        if (ph->getCurrentPosition (newTime))
        {
            lastPosInfo = newTime;  // Successfully got the current time from the host..
            return;
        }
    }

    // If the host fails to provide the current time, we'll just reset our copy to a default..
    lastPosInfo.resetToDefault();
}

//==============================================================================
AudioProcessorEditor* JuceDemoPluginAudioProcessor::createEditor()
{
    return new JuceDemoPluginAudioProcessorEditor (*this);
}

//==============================================================================
void JuceDemoPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:

    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);

    // Store the values of all our parameters, using their param ID as the XML attribute
    for (int i = 0; i < getNumParameters(); ++i)
        if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
            xml.setAttribute (p->paramID, p->getValue());

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}

void JuceDemoPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our last window size..
            lastUIWidth  = jmax (xmlState->getIntAttribute ("uiWidth", lastUIWidth), 400);
            lastUIHeight = jmax (xmlState->getIntAttribute ("uiHeight", lastUIHeight), 200);

            // Now reload our parameters..
            for (int i = 0; i < getNumParameters(); ++i)
                if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
                    p->setValue ((float) xmlState->getDoubleAttribute (p->paramID, p->getValue()));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JuceDemoPluginAudioProcessor();
}
