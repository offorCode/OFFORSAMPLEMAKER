#include "AudioAnalyzer.h"
#include "../Classification/SampleClassifier.h"

AudioFeatures AudioAnalyzer::analyze(juce::AudioFormatManager& fm, const juce::File& file)
{
    AudioFeatures f; std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader) return f;
    f.durationSeconds = (double)reader->lengthInSamples / reader->sampleRate;
    const int frames = (int)juce::jmin<juce::int64>(reader->lengthInSamples, 262144);
    if (frames <= 0) return f;
    if (reader->sampleRate <= 0.0 || reader->numChannels == 0) return f;
    juce::AudioBuffer<float> b(1, frames); juce::AudioBuffer<float> temp((int)reader->numChannels, frames);
    b.clear();
    temp.clear();
    if (!reader->read(&temp, 0, frames, 0, true, true)) return f;
    for (int c = 0; c < temp.getNumChannels(); ++c) b.addFrom(0, 0, temp, c, 0, frames, 1.0f / (float)temp.getNumChannels());
    const auto* x = b.getReadPointer(0); double sumSq = 0.0, peak = 0.0, crossings = 0.0, transient = 0.0;
    for (int i = 0; i < frames; ++i) { const double a = std::abs(x[i]); peak = juce::jmax(peak, a); sumSq += (double)x[i]*x[i]; if (i && ((x[i] >= 0) != (x[i-1] >= 0))) crossings++; if (i) transient += std::abs((double)x[i] - x[i-1]); }
    f.rms = std::sqrt(sumSq / frames); f.peak = peak; f.crestFactor = f.rms > 1.0e-9 ? peak / f.rms : 0.0;
    f.zeroCrossingRate = crossings / frames; f.transientStrength = transient / frames;
    const int fftOrder = 11, fftSize = 1 << fftOrder; juce::dsp::FFT fft(fftOrder);
    std::vector<float> data((size_t)fftSize * 2, 0.0f); const int hop = juce::jmax(1, frames / fftSize);
    for (int i = 0; i < fftSize; ++i) data[(size_t)i] = x[juce::jmin(frames - 1, i * hop)];
    juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    window.multiplyWithWindowingTable(data.data(), (size_t)fftSize);
    fft.performFrequencyOnlyForwardTransform(data.data()); double total=0, weighted=0; for (int i=1;i<fftSize/2;++i){const double m=data[(size_t)i]; total+=m; weighted+=m*i*reader->sampleRate/fftSize;}
    f.spectralCentroid = total > 0 ? weighted/total : 0.0; double acc=0; for(int i=1;i<fftSize/2;++i){acc+=data[(size_t)i]; if(acc>=total*0.85){f.spectralRolloff=(double)i*reader->sampleRate/fftSize;break;}}
    if (!std::isfinite(f.rms) || !std::isfinite(f.peak) || !std::isfinite(f.spectralCentroid)
        || !std::isfinite(f.spectralRolloff) || !std::isfinite(f.zeroCrossingRate)
        || !std::isfinite(f.transientStrength)) return AudioFeatures {};
    const auto result = ProvisionalHeuristicClassifier().classify(f);
    f.predictedCategory = result.category;
    f.classificationStatus = result.status;
    f.confidence = result.confidence;
    return f;
}
