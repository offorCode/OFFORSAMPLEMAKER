# OFFOR SAMPLE FORGE — staged implementation plan

This plan is based on the existing JUCE 8.0.14 / C++17 standalone application and preserves its CMake configuration.

## Baseline inspected

- `MainComponent` already owns an audio format manager and transport, recursively imports supported audio files, and previews one sample.
- The current UI has a sidebar and dashboard panels, but the browser is only an import count, the waveform is synthetic, and effect/variation controls are explicitly placeholders.
- VST3 hosting is enabled in CMake, but there is no scanner or host workflow.
- The existing build directory is configured for Visual Studio 18 2026 x64 and points at the declared JUCE checkout.

## Stages

1. **Library and preview foundation (current):** display imported samples in a selectable browser, show real decoded waveform data, and keep browsing/preview usable after imports. Preserve sources. Validate with the existing Release build.
2. **Project persistence and sample metadata:** introduce project model/serialization, source references, file metadata, category overrides, search and persistent settings.
3. **Audio analysis and classification:** background file analysis and transparent heuristic classifier with confidence/uncertain state; keep classifier modular for local ML later.
4. **Processing engine:** independent offline DSP chain with enable states, parameters, safe renders, and source preservation; then connect UI and preview.
5. **Variations:** seeded controlled render jobs, unique output IDs, preview/history, locking, and regeneration that never replaces originals or locked outputs.
6. **VST3 hosting and instrument rendering:** background scanning, safe effect hosting, plugin state, and a separate MIDI instrument render path.
7. **Missing category generation:** detect absent categories and offer opt-in generation through available instruments.
8. **Pack export:** preview naming/folder structure, export selected content and complete packs, documentation, ZIP, collision checks and verification.
9. **Settings, reliability, and distribution:** device/settings persistence, cancellation/progress, recovery, installer and acceptance workflow validation.

Each stage should build before the next begins. Features remain marked incomplete until their real workflow is implemented and verified.
