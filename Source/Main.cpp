#include <JuceHeader.h>
#include "MainComponent.h"

class OfforSampleForgeApplication final : public juce::JUCEApplication
{
public:
    OfforSampleForgeApplication() = default;

    const juce::String getApplicationName() override
    {
        return "OFFOR SAMPLE FORGE";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override
    {
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : juce::DocumentWindow(
                name,
                juce::Colour::fromRGB(24, 24, 27),
                juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            centreWithSize(1180, 760);
            setResizable(true, true);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            if (auto* app = juce::JUCEApplication::getInstance())
                app->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(OfforSampleForgeApplication)
