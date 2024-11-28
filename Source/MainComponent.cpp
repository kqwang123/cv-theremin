#include "MainComponent.h"
#include <JuceHeader.h>
#include "opencv2/opencv.hpp"

MainComponent::MainComponent()
{
    setSize(800, 600);

    // Frequency Slider
    frequencySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    frequencySlider.setRange(50.0, 500.0);
    frequencySlider.setValue(440.0);
    frequencySlider.addListener(this);
    addAndMakeVisible(frequencySlider);

    // Amplitude Slider
    amplitudeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    amplitudeSlider.setRange(0.0, 1.0);
    amplitudeSlider.setValue(0.25);
    amplitudeSlider.addListener(this);
    addAndMakeVisible(amplitudeSlider);

    // Audio channels setup
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) &&
        !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [this](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels(2, 2);
    }

    // OpenCV VideoCapture initialization
    if (!cap.open(0)) // Open the default camera
    {
        juce::Logger::writeToLog("Error: Could not open the webcam.");
    }
    else
    {
        startTimer(33); // Start a timer to capture frames (30 FPS)
    }
}

MainComponent::~MainComponent()
{
    shutdownAudio();
    stopTimer(); // Stop the timer when the component is destroyed
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &frequencySlider)
    {
        frequency = frequencySlider.getValue();
    }
    else if (slider == &amplitudeSlider)
    {
        amplitude = amplitudeSlider.getValue();
    }
}

void MainComponent::updateFrequency()
{
    increment = frequency * wtSize / currentSampleRate;
    phase = fmod((phase + increment), wtSize);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    frequency = frequencySlider.getValue();
    phase = 0.0;
    wtSize = 1024;
    currentSampleRate = sampleRate;

    increment = frequency * wtSize / currentSampleRate;
    amplitude = 0.25;
    for (int i = 0; i < wtSize; i++)
    {
        sineWave.insert(i, sin(2.0 * juce::double_Pi * double(i) / wtSize));
    }
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    float* const leftSpeaker = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    float* const rightSpeaker = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    for (int i = 0; i < bufferToFill.numSamples; i++)
    {
        leftSpeaker[i] = sineWave[int(phase)] * amplitude;
        rightSpeaker[i] = sineWave[int(phase)] * amplitude;
        updateFrequency();
    }
}

void MainComponent::releaseResources()
{
    // Release any audio resources
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);

    if (!frame.empty())
    {
        juce::Image image(juce::Image::PixelFormat::RGB, frame.cols, frame.rows, false);
        for (int y = 0; y < frame.rows; ++y)
        {
            for (int x = 0; x < frame.cols; ++x)
            {
                cv::Vec3b color = frame.at<cv::Vec3b>(y, x);
                image.setPixelAt(x, y, juce::Colour(color[2], color[1], color[0]));
            }
        }
        g.drawImage(image, getLocalBounds().toFloat());
    }
}

void MainComponent::resized()
{
    const int labelSpace = 100;
    frequencySlider.setBounds(labelSpace, 20, getWidth() - 100, 20);
    amplitudeSlider.setBounds(labelSpace, 60, getWidth() - 100, 20);
}

void MainComponent::timerCallback()
{
    if (cap.isOpened())
    {
        cap >> frame; 
        if (!frame.empty())
        {
            int centerX = frame.cols / 2;
            int centerY = frame.rows / 2;
            cv::Vec3b color = frame.at<cv::Vec3b>(centerY, centerX);
            int redValue = color[2]; 

            frequencySlider.setValue(redValue + 230.0);
        }
        repaint(); 
    }
}
