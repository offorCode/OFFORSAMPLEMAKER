# OFFOR SAMPLE FORGE User Manual

## Status of this manual

This manual describes features verified in the current source and Release build. VST3 discovery is implemented; plugin loading, editors, audio processing, and instrument rendering are not yet available. VST2 hosting is unsupported in this build. WAV and ZIP export for selected or all available variations are implemented. Full source-sample/project packaging is not available.

## 1. Introduction and requirements

OFFOR SAMPLE FORGE is a Windows desktop sample browser, offline audio processor, and sample variation generator from OFFOR AUDIO. It is built with C++17, JUCE 8, and CMake. It requires Windows 10/11, an audio output device for preview, and writable disk space for project assets and rendered files.

## 2. Install and launch

Run `OFFOR SAMPLE FORGE.exe` from the Release build or packaged application folder. Keep the adjacent `Resources` folder available. A project stores its project file separately from source audio; generated audio is placed in its project assets folder.

## 3. Projects

Use **New** to create a project, **Open** to open an `.osfproj` project, **Save** to save changes, **Save As** to save another project file, **Rename** to change the project name, and **Close** to close it. Project files retain sample references, categories and manual category changes, processing settings/history, variations and their state, export settings, and cached plugin scan results. The project file does not embed source audio. If a source file moves, use the Sample Library's relink action when it is shown.

## 4. Import audio

Choose **Import Folder** to scan a folder recursively. Audio is analyzed off the UI thread. WAV is verified by the current automated test suite; other formats are handled through JUCE's registered basic audio formats and depend on successful decoder support. Duplicate paths are skipped, unsupported files are reported, progress is shown, and cancellation is available. Source audio is not modified.

## 5. Sample Library

Choose **Sample Library** in the left navigation. Use the search field and category filter to narrow the sample list. Select a row to show sample details and its real waveform. Use **Play Preview** and **Stop** to audition audio. Categories can be corrected from the category selector; manual corrections are saved with the project. Classification is a provisional feature-based foundation and is not an AI model or a guaranteed musical classification.

## 6. Offline processing

Choose **Processing** after opening a project and selecting an available sample. The waveform panel compares the source in gray with the latest rendered copy in orange. Select an effect type, choose **Add Effect**, select it in the chain, and change its visible parameters. Use **Enabled** to bypass or re-enable an effect. **Move Up** and **Move Down** change processing order; **Remove**, **Reset Effect**, and **Reset Chain** manage the chain.

Available built-in effects are Parametric EQ, Compressor, Saturation, Reverb, Delay, Filter, Gain, and Limiter. The EQ exposes frequency, gain, Q, and wet mix. Reverb exposes room size, damping, width, and wet mix. Every effect is applied by the offline renderer, not during live preview. Choose **Render Processed Copy** to create a separate WAV in the project assets. **Play Original** and **Play Processed** compare the source and render. Rendering does not overwrite the source. A linked output safety ceiling prevents full-scale WAV clipping after the chain.

Audio operations include Normalize, Reverse, Trim Silence, Fade In, and Fade Out. Operation values/settings are saved with the processing configuration. Rendering supports progress and cancellation. Existing processing history can be selected; **Restore Chain** restores its effect and operation settings.

## 7. Variations

Choose **Variations** with a source sample selected in an open project. The top section shows the original waveform and playback controls. Set the variation count and intensity, adjust randomization strength, and choose **Generate Variations**. Five is the default count. Generation uses the existing built-in DSP renderer and creates separate uniquely named audio files. The selected intensity limits randomization; results are not claims of musical quality.

Open **Advanced Effect Ranges** to choose allowed effects and adjust maximum gain, EQ, and drive ranges. Use **Regenerate Selected** for selected unlocked cards or **Regenerate Unlocked** for unlocked slots. Lock a variation to protect it. Favorite, rename, select, play, compare, and delete controls operate on actual variation records and files; right-click a card for additional per-card actions. Deleted history records remain in project history. Choose **Export Selected** to copy selected WAV variation files to a folder with unique names. Missing variation files are shown as missing rather than represented by a fabricated waveform.

## 8. VST plugin discovery

Choose **VST3 Plugins**. The application scans the standard Windows VST3 folders and configured custom directories in a background task. Use **Rescan VST3**, **Cancel Scan**, search, and manufacturer filtering. **Add Plugin Folder** adds a custom scan directory. Only VST3 discovery is implemented. The current application does not instantiate plugins, open plugin editors, route audio or MIDI, or render through plugins. VST2 is not enabled and DLLs are not treated as plugins.

## 9. Export

The Export page provides a route to the Variations page. From there select one or more variation cards and use **Export Selected** to copy WAVs. Create a ZIP from selected or all available variation WAV files from the same menu. Full source-sample/project packaging is not implemented.

## 10. Settings and help

Choose **Settings** to enable or disable startup audio. Buttons with icons show a short purpose tooltip when hovered. The UI is resizable; the sample cards and variation grid scroll when content exceeds the visible area.

## 11. Troubleshooting

- **No waveform:** confirm the source file exists and can be decoded; use the relink action if it moved.
- **Processing sounds unchanged:** confirm an effect is added to the chain, enabled, and has a non-neutral setting; processing is offline, so render a copy before auditioning it.
- **Processed playback unavailable:** render a copy first and check the status message for errors.
- **No variations:** open a project, select a source sample, and ensure the project assets directory is writable.
- **Plugin not listed:** use **Rescan VST3** or **Add Plugin Folder**. A listed plugin is discovered metadata only, not a loaded/hosted plugin.
- **Export count is lower than selection:** check the status message and verify variation files still exist.

## 12. Feature classification

- **Implemented and automated-test verified:** project save/open including legacy schema fixtures; WAV import and duplicate-path handling; built-in DSP buffer processing; offline WAV rendering; variation generation, lock behavior, cancellation, source preservation, and project round-trip; VST3 discovery code path.
- **Implemented but not yet fully verified in the interactive application:** the redesigned navigation and page layouts, start-up preference, real-time UI behavior at all sizes, and a full scan of every installed plugin bundle.
- **Planned/not implemented:** VST3 plugin instantiation, plugin editor windows, plugin effect/instrument audio routing, MIDI instrument rendering, full source-sample/project pack export.
- **Unsupported in this build:** VST2 hosting.



