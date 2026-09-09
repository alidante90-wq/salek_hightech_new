# SALEK HIGHTECH

A real JUCE/C++ VST3 synthesizer foundation designed for High-Tech / Darkpsy / Psychedelic sound design.

## Current real engine
- 3 polyphonic wavetable oscillators
- 64-frame wavetable banks with frame morphing
- sine/saw/square/triangle/custom harmonic tables
- per-oscillator warp / phase distortion
- oscillator 2/3 FM into oscillator 1
- oscillator 2 AM/RM-style ring modulation against oscillator 1
- sub + noise
- 2-pole multimode filter
- amp ADSR + filter envelope
- tempo-synced LFO with free/Hz and beat divisions
- modulation matrix routes LFO / filter envelope / velocity / mod wheel to real destinations
- 8 macros
- arpeggiator
- distortion, chorus, delay and reverb
- MIDI and DAW automation through JUCE AudioProcessorValueTreeState
- preset save/load through JUCE state
- real-time spectrum/wave display

## Build
Requirements:
- CMake 3.22+
- C++20 compiler
- Git (CMake FetchContent downloads JUCE 8.0.8)

From the project directory:
    cmake -B build -S .
    cmake --build build --config Release

The VST3 target is `SalekHightech`.


## GitHub Actions

The repository includes `.github/workflows/build.yml`.

After pushing this project to GitHub, open **Actions → Build SALEK HIGHTECH → Run workflow**.
The workflow builds both **VST3** and **Standalone** on a Windows runner and publishes them as downloadable GitHub Actions artifacts.
