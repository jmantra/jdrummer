# jdrummer

A powerful, open-source drum machine VST3 plugin built with the JUCE framework. JDrummer features SoundFont-based drum kits, a comprehensive groove library with tempo-synced playback, a composition tool, and an intelligent Groove Matcher that analyzes audio to find matching drum patterns.

![JDrummer](https://img.shields.io/badge/Format-VST3%20%7C%20AU-blue) ![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-green) ![License](https://img.shields.io/badge/License-Open%20Source-orange)

## Table of Contents

- [Features](#features)
  - [Drum Pads](#-drum-pads)
  - [Grooves Browser](#-grooves-browser)
  - [Composer](#-composer)
  - [Groove Matcher](#-groove-matcher)
  - [Screenshots](#Screenshots)
- [Installation](#installation)
  - [Linux](#linux)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Adding Custom SoundFonts](#adding-custom-soundfonts)
  - [Adding Custom Grooves](#adding-custom-grooves)
- [Building from Source](#building-from-source)
  - [Prerequisites](#prerequisites)
  - [Building on Linux](#building-on-linux)
  - [Building on Windows (Native)](#building-on-windows-native)
  - [Building on macOS](#building-on-macos)
  - [Cross-Compiling for Windows from Linux](#cross-compiling-for-windows-from-linux)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Tested DAWs/Known Issues](#tested-dawsknown-issues)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Features

### 🥁 Drum Pads
- **16-pad drum grid** with velocity-sensitive playback
- **Multiple SoundFont kits** - Ships with 29 drum kits including:
  - Standard acoustic kits (Standard, Room Drums, Power Drums, Jazz Drums)
  - Electronic kits (808, Electronic Drums, Dance Drums, House Kit)
  - Specialty kits (Orchestral Percussion, Brush Drums, NIN Drumkit)
- **Per-pad controls**:
  - Individual volume adjustment (0-100%)
- Pan control (left/right)
- Solo and mute options
- **Dynamic kit loading** - Add your own .sf2 SoundFont files to expand your library

### 🎵 Grooves Browser
- **Extensive groove library** organized by category:
  - Basic Beats
  - Break Beats
  - Buildups
  - Double Bass Beats
  - Fills
  - Half Time Beats
  - OffBeats
  - Swing Beats
  - Tom Beats
- **Tempo-synced playback** - Grooves automatically sync to your DAW's tempo
- **Preview functionality** - Audition grooves before adding them to your project
- **Drag and drop** - Drag any groove directly into your DAW as a MIDI clip
- **Bar selection** - Choose how many bars of a groove to use (1-16 bars)

### 🎼 Composer
- **Build complete drum parts** by combining multiple grooves
- **Visual timeline** showing your arrangement
- **Reorder and remove** items from your composition
- **Export as MIDI** - Drag your entire composition to the DAW
- **Loop playback** for previewing your arrangement

### 🎯 Groove Matcher
An intelligent feature inspired by professional drum software that helps you find the perfect groove for your music:

- **Audio analysis** - Drop any audio file (WAV, MP3, FLAC, OGG, AIFF) to analyze its rhythm
- **BPM detection** - Automatically detects tempo using the minibpm library
- **Smart filename parsing** - Extracts BPM from filenames (e.g., "beat_120bpm.wav")
- **Pattern matching** - Analyzes rhythm patterns and finds matching grooves from your library
- **Similarity scoring** - Shows match percentage for each suggested groove
- **Preview with audio** - Play back the matched groove alongside your original audio
- **Automatic composition** - Best match is automatically added to the composer
- **Seamless workflow** - Found the perfect match? Drag it straight to your DAW

### Screenshots
Drum Kits Tab
![Screenshot](https://i.postimg.cc/qMnR82Ds/jdrummer-kits.png)

Grooves Tab
![Screenshot](https://i.postimg.cc/ZqJCY0L1/jdrummergrooves.png)

Groove Matcher Tab

![Screenshot](https://i.postimg.cc/t4BYTpmv/jdrummermmatch.png)




## Installation
Binary VST3 for Linux, Windows, and macOS available on the **[Releases](https://github.com/jmantra/jdrummer/releases)** page. macOS builds include both VST3 and Audio Unit (AU) formats.

### Linux
Copy the VST3 bundle to your VST3 directory:
```bash
cp -r jdrummer.vst3 ~/.vst3/
```

### Windows
Copy the `jdrummer.vst3` folder to one of these locations:
- **System-wide**: `C:\Program Files\Common Files\VST3\`
- **User only**: `C:\Users\<YourUsername>\Documents\VST3\`

### macOS
JDrummer builds as both **VST3** and **Audio Unit (AU)** on macOS.

**VST3 Installation:**
Copy the VST3 bundle to your VST3 directory:
```bash
cp -r jdrummer.vst3 ~/Library/Audio/Plug-Ins/VST3/
# Or system-wide:
sudo cp -r jdrummer.vst3 /Library/Audio/Plug-Ins/VST3/
```

**Audio Unit Installation:**
Copy the AU component to your Components directory:
```bash
cp -r jdrummer.component ~/Library/Audio/Plug-Ins/Components/
# Or system-wide:
sudo cp -r jdrummer.component /Library/Audio/Plug-Ins/Components/
```

After installing, you may need to restart your DAW or run:
```bash
killall -9 AudioComponentRegistrar
```

### Adding Custom SoundFonts
Place additional `.sf2` SoundFont files in the plugin's soundfonts directory:
- **Linux**: `~/.vst3/jdrummer.vst3/Contents/Resources/soundfonts/`
- **Windows**: `<VST3 Location>\jdrummer.vst3\Contents\Resources\soundfonts\`
- **macOS (VST3)**: `~/Library/Audio/Plug-Ins/VST3/jdrummer.vst3/Contents/Resources/soundfonts/`
- **macOS (AU)**: `~/Library/Audio/Plug-Ins/Components/jdrummer.component/Contents/Resources/soundfonts/`

The plugin will automatically detect new SoundFonts on the next load.

### Adding Custom Grooves
Place additional `.mid` MIDI files in the plugin's Grooves directory:
- **Linux**: `~/.vst3/jdrummer.vst3/Contents/Resources/Grooves/`
- **Windows**: `<VST3 Location>\jdrummer.vst3\Contents\Resources\Grooves\`
- **macOS (VST3)**: `~/Library/Audio/Plug-Ins/VST3/jdrummer.vst3/Contents/Resources/Grooves/`
- **macOS (AU)**: `~/Library/Audio/Plug-Ins/Components/jdrummer.component/Contents/Resources/Grooves/`

Organize grooves into subfolders to create categories in the browser.

## Building from Source

### Prerequisites

#### Linux (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev
sudo apt install libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev
sudo apt install libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
sudo apt install libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev
```

#### Linux (Fedora)
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git alsa-lib-devel jack-audio-connection-kit-devel
sudo dnf install freetype-devel libX11-devel libXcomposite-devel libXcursor-devel
sudo dnf install libXext-devel libXinerama-devel libXrandr-devel libXrender-devel
sudo dnf install webkit2gtk3-devel mesa-libGLU-devel mesa-libGL-devel libcurl-devel
```

#### Linux (Arch)
```bash
sudo pacman -S base-devel cmake git alsa-lib jack2
sudo pacman -S freetype2 libx11 libxcomposite libxcursor libxext libxinerama
sudo pacman -S libxrandr libxrender webkit2gtk glu mesa curl
```

#### Windows
- Visual Studio 2019 or later with C++ desktop development workload
- CMake 3.15 or later
- Git

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install CMake
brew install cmake
```

**Requirements:**
- macOS 10.13 (High Sierra) or later
- Xcode 12 or later (or Command Line Tools)
- CMake 3.15 or later

### Building on Linux

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/jdrummer.git
   cd jdrummer
   ```

2. **Create build directory and configure**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the plugin**
   ```bash
   make -j$(nproc)
   ```

4. **Install the VST3**
   ```bash
   # The VST3 bundle will be in:
   # build/jdrummer_artefacts/Release/VST3/jdrummer.vst3
   
   # Copy to your VST3 directory
   cp -r jdrummer_artefacts/Release/VST3/jdrummer.vst3 ~/.vst3/
   ```

### Building on Windows (Native)

1. **Clone the repository**
   ```powershell
   git clone https://github.com/yourusername/jdrummer.git
   cd jdrummer
   ```

2. **Create build directory and configure**
   ```powershell
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

3. **Build the plugin**
   ```powershell
   cmake --build . --config Release
   ```

4. **Install the VST3**
   ```powershell
   # The VST3 bundle will be in:
   # build\jdrummer_artefacts\Release\VST3\jdrummer.vst3
   
   # Copy to your VST3 directory
   xcopy /E /I jdrummer_artefacts\Release\VST3\jdrummer.vst3 "%COMMONPROGRAMFILES%\VST3\jdrummer.vst3"
   ```

### Building on macOS

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/jdrummer.git
   cd jdrummer
   ```

2. **Create build directory and configure**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release -G "Xcode"
   ```
   
   Or for a Makefile-based build (faster for command line):
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the plugin**
   
   Using Xcode project:
   ```bash
   cmake --build . --config Release
   ```
   
   Or using Make:
   ```bash
   make -j$(sysctl -n hw.ncpu)
   ```

4. **Install the VST3**
   ```bash
   # The VST3 bundle will be in:
   # build/jdrummer_artefacts/Release/VST3/jdrummer.vst3
   
   # Copy to your user VST3 directory
   cp -r jdrummer_artefacts/Release/VST3/jdrummer.vst3 ~/Library/Audio/Plug-Ins/VST3/
   
   # Or system-wide (requires admin password)
   sudo cp -r jdrummer_artefacts/Release/VST3/jdrummer.vst3 /Library/Audio/Plug-Ins/VST3/
   ```

5. **Install the Audio Unit**
   ```bash
   # The AU component will be in:
   # build/jdrummer_artefacts/Release/AU/jdrummer.component
   
   # Copy to your user Components directory
   cp -r jdrummer_artefacts/Release/AU/jdrummer.component ~/Library/Audio/Plug-Ins/Components/
   
   # Or system-wide (requires admin password)
   sudo cp -r jdrummer_artefacts/Release/AU/jdrummer.component /Library/Audio/Plug-Ins/Components/
   ```

6. **Refresh Audio Unit cache** (if the AU doesn't appear in your DAW)
   ```bash
   killall -9 AudioComponentRegistrar
   ```

### Cross-Compiling for Windows from Linux

This allows you to build Windows VST3 plugins on a Linux machine using MinGW.

1. **Install MinGW-w64**

   **Debian/Ubuntu:**
   ```bash
   sudo apt install mingw-w64
   ```

   **Fedora:**
   ```bash
   sudo dnf install mingw64-gcc mingw64-gcc-c++ mingw64-winpthreads-static
   ```

   **Arch:**
   ```bash
   sudo pacman -S mingw-w64-gcc
   ```

2. **Fix Windows.h case sensitivity (Fedora/some distros)**
   
   The VST3 SDK expects `Windows.h` but MinGW provides `windows.h`. Create a symlink:
   ```bash
   sudo ln -sf /usr/x86_64-w64-mingw32/sys-root/mingw/include/windows.h \
               /usr/x86_64-w64-mingw32/sys-root/mingw/include/Windows.h
   ```

3. **Create the toolchain file**
   
   Create `cmake/windows-toolchain.cmake`:
   ```cmake
   set(CMAKE_SYSTEM_NAME Windows)
   set(CMAKE_SYSTEM_PROCESSOR x86_64)
   
   set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
   set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
   set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
   
   set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
   
   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   
   set(WIN32 TRUE)
   set(MINGW TRUE)
   
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
   ```

4. **Build for Windows**
   ```bash
   mkdir build-windows && cd build-windows
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/windows-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

5. **Package the VST3**
   ```bash
   # Create distribution package
   mkdir -p dist/jdrummer.vst3/Contents/x86_64-win
   mkdir -p dist/jdrummer.vst3/Contents/Resources
   
   cp jdrummer_artefacts/Release/VST3/jdrummer.vst3/Contents/x86_64-win/jdrummer.vst3 \
      dist/jdrummer.vst3/Contents/x86_64-win/
   cp -r ../soundfonts dist/jdrummer.vst3/Contents/Resources/
   cp -r ../Grooves dist/jdrummer.vst3/Contents/Resources/
   
   # Create zip for distribution
   cd dist && zip -r ../jdrummer-windows-vst3.zip jdrummer.vst3
   ```

## Project Structure

```
jdrummer/
├── CMakeLists.txt              # Main build configuration
├── README.md                   # This file
├── Source/
│   ├── PluginProcessor.cpp/h   # Audio processing & plugin state
│   ├── PluginEditor.cpp/h      # Main UI with tab navigation
│   ├── SoundFontManager.cpp/h  # SoundFont loading & playback
│   ├── GrooveManager.cpp/h     # MIDI groove management & playback
│   ├── AudioAnalyzer.cpp/h     # Audio analysis for Groove Matcher
│   ├── JuceHeader.h            # JUCE module includes
│   └── Components/
│       ├── DrumPad.cpp/h       # Individual drum pad component
│       ├── DrumPadGrid.cpp/h   # 4x4 drum pad grid
│       ├── PadControls.cpp/h   # Volume/pan/solo/mute controls
│       ├── KitSelector.cpp/h   # SoundFont kit dropdown
│       ├── GroovesPanel.cpp/h  # Grooves browser & composer
│       └── BandmatePanel.cpp/h # Groove Matcher UI
├── libs/
│   ├── TinySoundFont/          # SoundFont synthesis library
│   └── minibpm/                # BPM detection library
├── soundfonts/                 # Included drum kit SoundFonts
├── Grooves/                    # MIDI groove library
└── cmake/
    └── windows-toolchain.cmake # Cross-compilation toolchain
```

## Dependencies

JDrummer uses the following open-source libraries:

- **[JUCE](https://juce.com/)** - Cross-platform C++ framework for audio applications
- **[TinySoundFont](https://github.com/schellingb/TinySoundFont)** - SoundFont synthesis library
- **[minibpm](https://github.com/breakfastquay/minibpm)** - Lightweight BPM detection library

## Tested DAWs/Known Issues

JDrummer has been tested with:
- Ardour (Linux, Windows, macOS)
- REAPER (Linux, Windows,macOS)
- GarageBand (macOS) - AU only
- Bitwig Studio (Linux, Windows) - Drag and Drop does not currently work in Bitwig Flatpak 5.3
- Plugin crashes when trying to load in Qtractor
- n-track (Linux)
- Studio Pro 7 Beta for Linux - Plugin loads but GUI elements don't show
- Qtractor - Plugin loads in the latest appimage version but drag and drop does not work.
- FL Studio (Windows) - Able to drag to Piano Roll window but not track


## Troubleshooting

### Plugin not appearing in DAW
1. Ensure the VST3 is in the correct directory
2. Rescan plugins in your DAW
3. Check that the Resources folder contains soundfonts and Grooves

### No sound from drum pads
1. Check that a SoundFont kit is selected
2. Verify pad volumes are not at 0%
3. Ensure pads are not muted

### Grooves not syncing to tempo
1. Make sure the DAW is playing (transport running)
2. Check that the DAW is sending tempo information

### Groove Matcher not detecting BPM correctly
1. Try audio files with clear rhythmic content
2. Use files with BPM in the filename for best results
3. Ensure the audio file is a supported format (WAV, MP3, FLAC, OGG, AIFF)

### macOS: Audio Unit not appearing in DAW
1. Run `killall -9 AudioComponentRegistrar` to refresh the AU cache
2. Restart your DAW after installing
3. Check that the `.component` bundle is in `~/Library/Audio/Plug-Ins/Components/` or `/Library/Audio/Plug-Ins/Components/`
4. Verify the plugin passes validation: `auval -v aumu Jdrm Manu`

### macOS: Gatekeeper blocking the plugin
If macOS blocks the plugin as unidentified developer:
1. Right-click (or Control-click) on the `.vst3` or `.component` bundle
2. Select "Open" from the context menu
3. Click "Open" in the dialog that appears
4. Alternatively, run: `xattr -cr ~/Library/Audio/Plug-Ins/VST3/jdrummer.vst3`

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is open source. See LICENSE file for details.

## Acknowledgments

- Thanks to the JUCE team for their excellent framework
- TinySoundFont by Bernhard Schelling
- minibpm by BreakfastQuay
- Soundfonts by:
   - S. Christian Collins  **[GeneralUser GS Soundfont](https://schristiancollins.com/generaluser.php)** 
   -  **[AVL Drumkits by Glen MacArthur ](https://www.bandshed.net/avldrumkits/)** licensed under  CC-BY-SA 3.0
   -  **[Hydrogen Drum Kits ](https://sourceforge.net/projects/hydrogen/files/Sound%20Libraries/Main%20sound%20libraries/)** 
   - **[Kor Tiny](https://www.vogons.org/viewtopic.php?f=24&t=57273)**
   - SoniNeko and NIN Kit found on Musical Artifacts 
- MIDI Grooves  by  **[TheDrumJockey](https://youtu.be/tZzl3xBaBW8?si=pIdaIxch_DVNYvuw)** 

