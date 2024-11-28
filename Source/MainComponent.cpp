#include "MainComponent.h"
#include <JuceHeader.h>

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(800, 600);

    frequencySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    frequencySlider.setRange(50.0, 500.0);
    frequencySlider.setValue(440.0);
    frequencySlider.addListener(this);
    addAndMakeVisible(frequencySlider);

    amplitudeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    amplitudeSlider.setRange(0.0, 1.0);
    amplitudeSlider.setValue(0.25);
    amplitudeSlider.addListener(this);
    addAndMakeVisible(amplitudeSlider);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                          [&](bool granted)
                                          { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::sliderValueChanged(juce::Slider *slider) 
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

//==============================================================================
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

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{

    float *const leftSpeaker = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    float *const rightSpeaker = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    for (int i = 0; i < bufferToFill.numSamples; i++)
    {
        leftSpeaker[i] = sineWave[int(phase)] * amplitude;
        rightSpeaker[i] = sineWave[int(phase)] * amplitude;
        updateFrequency();
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    const int labelSpace = 100;
    frequencySlider.setBounds(labelSpace, 20, getWidth() - 100, 20);
}
