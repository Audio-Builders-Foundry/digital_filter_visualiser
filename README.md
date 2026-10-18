# Digital Filter Visualizer

[![Version](https://img.shields.io/badge/version-0.1.0-blue)](https://github.com/Audio-Builders-Foundry/digital_filter_visualiser)
[![License?](https://img.shields.io/badge/license-AGPLv3-green)](LICENSE)
[![Year](https://img.shields.io/badge/year-2026-brightgreen)](https://github.com/Audio-Builders-Foundry/digital_filter_visualiser)
![Linux](https://github.com/Audio-Project-Workgroup/digital_filter_visualiser/workflows/Linux/badge.svg)
![Windows](https://github.com/Audio-Project-Workgroup/digital_filter_visualiser/workflows/Windows/badge.svg)
![MacOS](https://github.com/Audio-Project-Workgroup/digital_filter_visualiser/workflows/macOS/badge.svg)

<img src="data\UI.png" alt="Digital Filter Visualiser UI" width="634" />

## Description

Digital filters are a foundational concept in digital audio, but designing them presents certain challenges: from finding the right coefficients to checking for stability.

We present DigitalFilterVisualizer: a tool that allows you to see the mathematical structure of linear time-invariant filters, calculate filter coefficients from your poles and zeros and vice versa, automatically ensure the stability, and instantly hear the result.

## Contents

- [Features](#features)
- [Building from source](#building-from-source)
- [Usage](#usage)
    - [Menu Controls](#menu-controls)
    - [Coefficients Table Editor](#coefficients-table-editor)
    - [Complex Plane Editor](#complex-plane-editor)
    - [Frequency and Phase Viewer](#frequency-and-phase-viewer)
<!-- - [FAQ](#faq) -->
- [Future Work](#future-work)
- [Become Contributor](#become-contributor)
- [Acknowledgments](#acknowledgments)

## Features

DigitalFilterVisualizer features 5 autosynchronized representations of a filter, including interactive visual interfaces, to provide utility in both learning and creating. These include:
- a graphical editor for zeros and poles position in a complex plane
- a coefficients table editor
- a formula representation of the polynomial 
- phase responce viewer
- frequency response viewer

A method to convert coefficients to roots is provided using the Francis implicit double shift QR algorithm. 

## Building from Source

Build supports both Standalone and VST3 binaries. To build Filterlyco **CMake** is required.

To download the source code:

```
$ git clone --recurse-submodules https://github.com/Audio-Project-Workgroup/digital_filter_visualiser.git && cd digital_filter_visualizer
```

To build on Windows platform, simply run:

```
build
```

To build on MacOS & Linux, run:
```
./build.sh
```

## Usage

**Digital Filter Visualiser** can be used either as a standalone application or as a VST3 plugin from within your DAW.

Before using the application, have in mind the following. Drawing from theory, for an IIR filter to be stable, all poles must lie strictly inside the unit circle in the complex plane. The **Digital Filter Visualizer** automatically enforces this constraint after every change, preventing poles from being placed on or outside the unit circle.

The following sections describe the available controls and their functionality.

#### Menu Controls

Controls | Description
:-- | --:
Options | Export, load or reset filter state and Audio/MIDI settings
Undo | Revert the last change
Redo | Restore the last undone change
Export | Export the current **Filter Coeffiicients** or **Processor Chain Parameters**
Add root | Add a new pole or zero to the filter (see note below)
Delete root	| Remove the selected pole or zero
Import | Load an audio file
Playback | Start audio playback
Stop | Stop audio playback
Loop | Repeat playback when it finishess
Level slider | Adjust the audio playback level

> Note: Adding a new root adds a new zero at (1,0), corresponding to 0 degrees on the polar axis. If the number of poles becomes smaller than the number of zeros, a new pole is also added at the centre of the unit circle, that is at (0,0). In general, the number of zeros can never exceed the number of poles.

#### Complex Plane Editor

The **Complex Plane Editor** allows users to move poles and zeros by dragging them. Poles and zeros can also be merged by dropping them onto one another. Zeros can be dragged outside the unit circle, but poles cannot.

#### Coefficients Table Editor 

The **Coefficients Table Editor** allows users to have an overview and edit filter coefficients by double-clicking the corresponding cells. Pressing the **Recalculate** button updates the filter state. If the changes result in an unstable filter, the changes are aborted and automatically reverted back to the previous state.

#### Frequency and Phase Viewer

Controls | Description
:-- | --:
Frequency | Display the filter's frequency response
Phase | Display the filter's phase response
Both | Display both frequency and phase responsess
Linear | Use a linear frequency scale
Log | Use a logarithmic frequency scale
Zoom In | Magnify the response plot
Zoom Out | Reduce the magnification of the response plot

## Future Work

**Digital Filter Visualizer** is a tool with room for future development. The followinng are some potential areas for future work:

- Implement audio loading support for Standalone application.
- Implement message display (i.e. informing user when changes resulted in unstable filter)
- [Acessibility support](https://github.com/Audio-Builders-Foundry/digital_filter_visualiser/discussions/2s)

## Become Contributor

Contributions to the project are welcome. Feel free to open an issue, submit a pull request, suggest features, to help improve the project and get involved. 

## Acknowledgments

Born from connections made at [ADC 24](https://audio.dev/archive/adc24-bristol/), this project exists thanks to the [Audio-Builders-Foundry's](https://github.com/Audio-Builders-Foundry) initiative to unite developers with shared interests under the scope of music and IT. Special appreciation to all the people who contributed their time to it.
### Mac & Linux
- run `./build.sh`
