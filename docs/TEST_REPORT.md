# Automated and Manual Test Report

Date: 2026-11-09
Build: Windows x64 Release, JUCE 8.0.14, Visual Studio 2026 toolchain

## Build

`cmake --build build --config Release --target OfforSampleForge OfforStage2Tests -j6` — PASS. `OFFOR SAMPLE FORGE.exe` and `OfforStage2Tests.exe` were produced.

## Automated regressions

`build/OfforStage2Tests_artefacts/Release/OfforStage2Tests.exe` — PASS. The test output covers Stage 1–4 sample import, classification, project schema compatibility/persistence, DSP, render, and variation regressions.

Additional assertions now render the imported WAV fixture through all eight built-in effects (Parametric EQ, Compressor, Saturation, Reverb, Delay, Filter, Gain, Limiter), decode each output, require measurable difference/non-silence/peak safety, and compare disabled-effect output with its source sample-for-sample. Existing direct-buffer assertions cover EQ boost/cut, frequency/Q changes, reverb tail and wet/dry behavior, compressor dynamic range, saturation, delay, gain, limiter, and effect ordering. Five generated variation files are reopened and feature-compared for diversity; source bytes and locked variations are checked; cancellation and project round-trip are checked. WAV export is repeated to confirm collision-safe names; a ZIP is reopened and checked for five entries.

## VST3 discovery

Launching the Release application triggered its automatic scan. The saved plugin catalog contains 14 valid VST3 plugin descriptions. The default VST3 directory contains five `.vst3` bundles at its top level. The standard VST2 directory contains two DLL files, which were not treated as plugins. This verifies discovery/catalog persistence in this environment. No plugin instance was loaded.

## VST3 hosting

NOT IMPLEMENTED / NOT TESTED. There are no effect/instrument instance controls, plugin editor hosting, audio/MIDI routing, plugin state restoration, or offline plugin-render path. Therefore no real plugin processing claim is made and the required VST3 hosting acceptance test is not met.

## Interactive UI

The Release app was launched and Home, Sample Library, Processing, and Variations pages were visually inspected before the final layout changes. The Processing empty-chain phantom sliders were corrected. Variations was then reorganized and an advanced section added, but final visual verification was interrupted by another foreground window and could not be completed. No real sample was imported through the UI in this test run. No screenshots have been saved to `docs/screenshots/`.

## Limitations

VST3 hosting remains the main milestone blocker. VST2 remains disabled. Pack export includes available variation WAVs only; source sample packaging and full project export are not implemented. The Export/ZIP UI action itself is not automated end-to-end; the extracted export functions are covered by the regression executable.
