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
<!-- - [Quick Start Guide](#quick-start-guide) -->
<!-- - [FAQ](#faq) -->
- [Future Work](#future-work)
<!-- - [Become Contributor](#documentation) -->
<!-- - [Acknowledgments](#acknowledgments) -->

## Features

DigitalFilterVisualizer features 5 autosynchronized representations of a filter, including interactive visual interfaces, to provide utility in both learning and creating. These include:
- a graphical editor for zeros and poles position in a complex plane
- a coefficients table editor
- a formula representation of the polynomial 
- phase responce viewer
- frequency response viewer

A method to convert coefficients to roots is provided using the Francis implicit double shift QR algorithm. 

## Building from source

To build Filterlyco **CMake** is required.

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

## Future Work

- [Acessibility support](https://github.com/Audio-Builders-Foundry/digital_filter_visualiser/discussions/2s)