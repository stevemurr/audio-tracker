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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class RestRequest
{
public:
    
    RestRequest (String urlString) : url (urlString) {}
    RestRequest (URL url)          : url (url) {}
    RestRequest () {}
    
    struct Response
    {
        Result result;
        StringPairArray headers;
        var body;
        String bodyAsString;
        int status;
        
        Response() : result (Result::ok()), status (0) {} // not sure about using Result if we have to initialise it to ok...
    } response;
    
    
    RestRequest::Response execute ()
    {
        auto urlRequest = url.getChildURL (endpoint);
        bool hasFields = (fields.getProperties().size() > 0);
        if (hasFields)
        {
            MemoryOutputStream output;
            
            fields.writeAsJSON(output, 0, false);
            urlRequest = urlRequest.withPOSTData (output.toString());
        }
        
        ScopedPointer<InputStream> in (urlRequest.createInputStream (hasFields, nullptr, nullptr, stringPairArrayToHeaderString(headers), 0, &response.headers, &response.status, 5, verb));
        
        response.result = checkInputStream (in);
        if (response.result.failed()) return response;
        
        response.bodyAsString = in->readEntireStreamAsString();
        response.result = JSON::parse(response.bodyAsString, response.body);
        
        return response;
    }
    
    
    RestRequest get (const String& endpoint)
    {
        RestRequest req (*this);
        req.verb = "GET";
        req.endpoint = endpoint;
        
        return req;
    }
    
    RestRequest post (const String& endpoint)
    {
        RestRequest req (*this);
        req.verb = "POST";
        req.endpoint = endpoint;
        
        return req;
    }
    
    RestRequest put (const String& endpoint)
    {
        RestRequest req (*this);
        req.verb = "PUT";
        req.endpoint = endpoint;
        
        return req;
    }
    
    RestRequest del (const String& endpoint)
    {
        RestRequest req (*this);
        req.verb = "DELETE";
        req.endpoint = endpoint;
        
        return req;
    }
    
    RestRequest field (const String& name, const var& value)
    {
        fields.setProperty(name, value);
        return *this;
    }
    
    RestRequest header (const String& name, const String& value)
    {
        RestRequest req (*this);
        headers.set (name, value);
        return req;
    }
    
    const URL& getURL() const
    {
        return url;
    }
    
    const String& getBodyAsString() const
    {
        return bodyAsString;
    }
    
private:
    URL url;
    StringPairArray headers;
    String verb;
    String endpoint;
    DynamicObject fields;
    String bodyAsString;
    
    Result checkInputStream (InputStream* in)
    {
        if (! in) return Result::fail ("HTTP request failed, check your internet connection");
        return Result::ok();
    }
    
    static String stringPairArrayToHeaderString(StringPairArray stringPairArray)
    {
        String result;
        for (auto key : stringPairArray.getAllKeys())
        {
            result += key + ": " + stringPairArray.getValue(key, "") + "\n";
        }
        return result;
    }
};

struct gistResults {
    float f0;
    float rms;
    float rmsDB;
    float peakEnergy;
    float zeroCrossings;
    float spectralCentroid;
    float spectralCrest;
    float spectralFlatness;
    float spectralRolloff;
    float spectralKurtosis;
    float energyDifference;
    float spectralDifference;
    float spectralDifferenceHalfWaveRectified;
    float complexSpectralDifference;
    float highFrequencyContent;
};

class JuceDemoPluginAudioProcessor  : public AudioProcessor
{
public:
    JuceDemoPluginAudioProcessor();
    ~JuceDemoPluginAudioProcessor();
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    bool hasEditor() const override                                             { return true; }
    AudioProcessorEditor* createEditor() override;

    const String getName() const override                                       { return JucePlugin_Name; }

    bool acceptsMidi() const override                                           { return true; }
    bool producesMidi() const override                                          { return true; }

    double getTailLengthSeconds() const override                                { return 0.0; }

    int getNumPrograms() override                                               { return 0; }
    int getCurrentProgram() override                                            { return 0; }
    void setCurrentProgram (int /*index*/) override                             {}
    const String getProgramName (int /*index*/) override                        { return String(); }
    void changeProgramName (int /*index*/, const String& /*name*/) override     {}

    void getStateInformation (MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void post(juce::String url, juce::String endpoint);
    void analyze(const float * buffer, const int numSamples, gistResults *g);
    double getTimeInSeconds();
    
    AudioPlayHead::CurrentPositionInfo lastPosInfo;
    
    String startedAt, endedAt;
    
    int lastUIWidth, lastUIHeight;
    float result_f0, result_rms, result_peakEnergy, result_zcr, result_spectralCentroid, result_spectralCrest, result_spectralFlatness, result_spectralRolloff, result_spectralKurtosis, result_energyDifference, result_spectralDifference, result_sd_hwr, result_csd, result_hfc;
    
    int silenceThreshold, silenceHeard, blocksHeard;
    bool started;
    float accum_f0, accum_rms, accum_centroid, accum_kurtosis, accum_peakEnergy, accum_zcr, accum_specCrest, accum_specFlat, accum_specRolloff;
    float rmsThreshold;
    double startedTime, endedTime;
    
private:
    void updateCurrentTimeInfoFromHost();
    static BusesProperties getBusesProperties();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceDemoPluginAudioProcessor)
};
