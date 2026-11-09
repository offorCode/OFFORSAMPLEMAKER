# OFFOR SAMPLE FORGE — Starter Project

Windows standalone JUCE application for the OFFOR AUDIO sample-production workstation.

## Requirements

- Windows 10/11 x64
- Visual Studio 2026 with Desktop development with C++
- CMake 3.22 or newer
- JUCE 8.0.14 source checkout

## 1. Set the JUCE source path

The `JUCE_SOURCE_DIR` value must point to the JUCE root directory containing `CMakeLists.txt`.

Example only (replace this with the actual location on your computer):

```powershell
-DJUCE_SOURCE_DIR="C:/AudioDevelopment/JUCE"
```

## 2. Configure

Open PowerShell in this project directory:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_SOURCE_DIR="C:/PATH/TO/JUCE"
```

Replace `C:/PATH/TO/JUCE` with the real JUCE 8.0.14 directory.

If your installed CMake does not recognize the Visual Studio 18 2026 generator, run:

```powershell
cmake --help
```

and use the exact Visual Studio generator listed by your CMake installation.

## 3. Build

```powershell
cmake --build build --config Release
```

## 4. Run

The executable is normally produced under a configuration-specific directory inside `build`, for example:

```text
build/OfforSampleForge_artefacts/Release/
```

Use the actual output location reported by CMake/Visual Studio if it differs.

## Important architecture note

This target is a **standalone application that hosts VST3 plugins**. It is not itself a VST3 effect plugin.

`JUCE_PLUGINHOST_VST3=1` enables JUCE's VST3 hosting classes at compile time. It does not yet implement plugin scanning, plugin loading, plugin UI, audio routing, offline rendering, or plugin-state persistence. Those are separate development tasks.

## Stage 2 project and library foundation

Projects use the versioned `.osfproj` JSON format and retain references to original audio paths. Generated assets belong in the adjacent `<project>_assets` directory; importing never copies or modifies source audio. Folder scans and metadata/feature extraction run on a worker thread, with duplicate-path filtering and progress/cancellation controls.

The classifier interface is in `Source/Classification/`. Its current heuristic emits only a low-confidence provisional percussion cue; other samples remain uncertain. It is not a trained AI classifier.

To run the Stage 2 regression checks, configure with `-DOFFOR_BUILD_STAGE2_TESTS=ON`, build `OfforStage2Tests`, and run `build/OfforStage2Tests_artefacts/Release/OfforStage2Tests.exe`. The checks cover decoded WAV features, background recursive import, unsupported/duplicate files, metadata, and project JSON round-trip.

## Planned source modules

The remaining implementation stages will add:

- `Core/` — project settings, paths, source permissions and naming templates
- `Audio/` — decoding, analysis, DSP and rendering
- `Classification/` — local model support beyond the current provisional baseline
- `Project/` — project serialization, source references, and project-managed assets (foundation implemented)
- `Samples/` — import, sample records, metadata, and analysis (foundation implemented)
- `Plugins/` — VST3 scanner, plugin instances, state and routing
- `Generation/` — synthesizer MIDI generation and offline instrument rendering
- `Batch/` — variation jobs, seeds, locking and regeneration
- `Export/` — folder creation, metadata, previews and ZIP packaging
- `UI/` — browser, waveform, effect rack, queue and export screens

Original source files must remain unchanged. Export eligibility should be based on documented rights to modify and redistribute the source material; effects or renaming do not create those rights.
