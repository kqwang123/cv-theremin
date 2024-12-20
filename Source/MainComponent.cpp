#include "MainComponent.h"
#include <JuceHeader.h>
#include "opencv2/opencv.hpp"

MainComponent::MainComponent() : bgSubtractor(cv::createBackgroundSubtractorMOG2(2500,10,false))
{
    setSize(800, 600);

    // Frequency Slider
    frequencySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    frequencySlider.setRange(220.0, 880.0);
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
        //creates a juce image with the size of the opencv frame.
        //attributes(RGB format, frame column size, frame row size, not single channel)
        juce::Image image(juce::Image::PixelFormat::RGB, frame.cols, frame.rows, false);

        //loops through the matrice to assign each juce Image pixel with the rgb value of the opencv frame
        for (int y = 0; y < frame.rows; ++y)
        {
            for (int x = 0; x < frame.cols; ++x)
            {
                cv::Vec3b color = frame.at<cv::Vec3b>(y, x);
                image.setPixelAt(x, y, juce::Colour(color[2], color[1], color[0]));
            }
        }

        //draws the image on the canvas
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
        //video capture is inputed into the frame matrice
        cap >> frame; 

        cv::Mat flippedFrame;
        cv::flip(frame, flippedFrame, 1); //frame is flipped because we are trying to mirror from the perspective of the user
        
        cv::Rect rectangleBounds(288, 12, 300, 300);
        cv::rectangle(flippedFrame, rectangleBounds, cv::Scalar(0, 0, 255), 2);
        cv::Mat playground = flippedFrame(rectangleBounds);
        //matrix but only in the bounds of the rectangle

        cv::Mat grayScaleCopy;
        cv::cvtColor(playground, grayScaleCopy, cv::COLOR_RGB2GRAY);
        cv::GaussianBlur(grayScaleCopy, grayScaleCopy, cv::Size(23,23), 0);
        //convert matrix to grayscale and apply a gaussian blur

        // cv::Mat grayScaleColor;
        // cv::cvtColor(grayScaleCopy, grayScaleColor, cv::COLOR_GRAY2BGR); // convert grayScaleColor to channels that flippedFrame can use
        // grayScaleColor.copyTo(flippedFrame(rectangleBounds));

        cv::Mat fgMask;
        bgSubtractor->apply(grayScaleCopy, fgMask); //applys a background subtractor to the grayscalecopy and puts it in the fgmask matrix

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(fgMask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat fgMaskColor;
        cv::cvtColor(fgMask, fgMaskColor, cv::COLOR_GRAY2BGR); // convert fgMask to channels that flippedFrame can use
        fgMaskColor.copyTo(flippedFrame(rectangleBounds));
        frame = flippedFrame;

        // finding finger through contours
        double maxArea = 0.0;
        int largestContourIdx = -1;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                largestContourIdx = i;
            }
        }

        if (largestContourIdx != -1) {
            const std::vector<cv::Point>& largestContour = contours[largestContourIdx];

            std::vector<cv::Point> offsetContour;
            for (const auto& point : largestContour) {
                offsetContour.push_back(cv::Point(point.x + rectangleBounds.x, point.y + rectangleBounds.y));
            }

            cv::drawContours(flippedFrame, std::vector<std::vector<cv::Point>>{offsetContour}, -1, cv::Scalar(0, 255, 0), 2);

            cv::Point fingertip = largestContour[0];
            for (const auto& point : largestContour) {
                if (point.y < fingertip.y) { // Find the smallest y-coordinate so that is fingertip
                    fingertip = point;
                }
            }

            int x = fingertip.x;
            int y = fingertip.y;
            //assigns x and y to the row and col of the fingertip

            double minFreq = 220.0; // corresponds to note A3
            double maxFreq = 880.0; // corresponds to note A5
            double semitoneRange = 12.0 * std::log2(maxFreq / minFreq); //represents the range of a full 2 octaves
            /* https://en.wikipedia.org/wiki/Piano_key_frequencies */

            double semitoneOffset = (x * semitoneRange) / fgMaskColor.cols; 
            pitch = minFreq * std::pow(2.0, semitoneOffset / 12.0);
            /*
                essentially maps each pixel in the box to a frequency between the 2 octaves
            */

            volume = 1.0 - static_cast<double>(y) / fgMask.rows; //assigns the volume based on the height of where the finger is positioned

            cv::Point framePoint = cv::Point(x+rectangleBounds.x, y+rectangleBounds.y);
            cv::circle(flippedFrame, framePoint, 2, cv::Scalar(0, 0, 255), -1);  
            //maps a circle to the fingertip 
        }

        int intPitch = pitch;
        int intVolume = volume * 100;
        std::string text = std::to_string(intPitch) + ',' + std::to_string(intVolume);
        cv::Point org(0, 100);
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        cv::Scalar color(255, 255, 255);
        cv::putText(frame, text, org, fontFace, 1.0, color, 2);
        //this essentially just tracks the pitch and volume for us in text

        if (!frame.empty()){
        
            /*int centerX = frame.cols / 2;
            int centerY = frame.rows / 2;
            cv::Vec3b color = frame.at<cv::Vec3b>(centerY, centerX);
            int redValue = color[2]; */

            frequencySlider.setValue(pitch);
            amplitudeSlider.setValue(volume);
            //applys the pitch and volume value to the sliders
        }
        repaint(); 
    }
}
