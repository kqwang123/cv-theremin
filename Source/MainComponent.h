#pragma once

#include <JuceHeader.h>
#include "opencv2/opencv.hpp"

class MainComponent : public juce::AudioAppComponent, juce::Slider::Listener, juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void updateFrequency();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override; // For periodic webcam frame updates

private:
    juce::Slider frequencySlider;
    juce::Slider amplitudeSlider;

    juce::Array<float> sineWave;
    double wtSize;
    double frequency;
    double phase;
    double increment;
    double amplitude;
    double currentSampleRate;

    cv::VideoCapture cap; 
    cv::Mat frame;        // Current webcam frame

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
