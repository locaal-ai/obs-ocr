# OCR - OBS Plugin

<div align="center">

[![GitHub](https://img.shields.io/github/license/occ-ai/obs-ocr)](https://github.com/occ-ai/obs-ocr/blob/main/LICENSE)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/occ-ai/obs-ocr/push.yaml)](https://github.com/occ-ai/obs-ocr/actions/workflows/push.yaml)
[![Total downloads](https://img.shields.io/github/downloads/occ-ai/obs-ocr/total)](https://github.com/occ-ai/obs-ocr/releases)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/occ-ai/obs-ocr)](https://github.com/occ-ai/obs-ocr/releases)
[![Discord](https://img.shields.io/discord/1200229425141252116)](https://discord.gg/KbjGU2vvUz)

</div>

## Introduction

The OCR Plugin for OBS provides real-time Optical Characted Recognition (OCR) or Text Recognition abilities over any OBS Source that provides an image - can be Image, Video, Browser or any other Source.
It is based on the incredible [Tesseract](https://github.com/tesseract-ocr/tesseract) open source OCR engine, compiled and running directly inside OBS for real-time operation on every frame rendered.

**Reading scoreboards**? Try https://scoresight.live free OCR tool specifically made (by us) for reading and broadcasting scoreboards.

If this free plugin has been valuable to you consider adding a ⭐ to this GH repo, subscribing to [my YouTube channel](https://www.youtube.com/@royshilk) where I post updates, and supporting my work: https://www.patreon.com/RoyShilkrot https://github.com/sponsors/royshil

<!--
, rating it [on OBS](https://obsproject.com/forum/resources/url-api-source-fetch-live-data-and-display-it-on-screen.1756/)
-->

### Usage Tutorials
<div align="center">
  <a href="https://youtu.be/Lnty2vuVr4I" target="_blank">
    <img width="45%" src="https://github-production-user-asset-6210df.s3.amazonaws.com/441170/297753085-277bc972-a878-47a6-957f-da99d7a8d6d4.jpeg" />
  </a>
  <br/>
  4 Minutes
</div>

#### Do more with OCR Plugin
OCR Plugin enables many use cases for enhancing your stream or recording:
<div align="center">
  <a href="https://youtu.be/vVclLhFpBc8" target="_blank">
    <img width="27%" src="https://github.com/occ-ai/obs-ocr/assets/441170/91febd15-a155-43fb-9b24-b1da99ad8c38" />
  </a>
  <a href="https://youtu.be/hQSCJaUgwhs" target="_blank">
    <img width="27%" src="https://github.com/occ-ai/obs-ocr/assets/441170/33bf35b3-0b98-4920-920b-9694beefd822" />
  </a>
</div>


### Features
Available now:
 - Add OCR Filter to any source with image or video output
 - Choose from Scoreboard model or English, French, Spanish, German, Chinese, Japanese, Arabic, Turkish, Portugese, Hindi, Russian and Italian
 - Output OCR result to an OBS Text Source
 - Choose the segmentation mode: Word, Line, Page, etc.
 - "Semantic Smoothing": getting more consistent outputs with higher accuracy and confidence by "averaging" several text outputs
 - Timing/Running modes: per X-milliseconds
 - Output formatting (with inja): e.g. "Score: {{score}}"
 - Output text detection to image source
 - Binarization methods (threshold, Otsu, Triangle, adaptive)
 - Image Dilation
 - Rescale (optimal Tesseract performance is at 35 pixels / character)

Coming soon:
 - More languages built-in (pretrained Tesseract models)
 - Allowing external model files
 - More output capabilities e.g. Parsing, websocket event, etc.
 - Extracting text from complex image layouts
 - Different timing/run modes: per X-frames, image change, etc.
 - Image stabilization
 - Optical flow tracking for fast moving text
 - Image processing: Perspective warping, auto-cropping, etc.
 - Advanced binarization: Niblack, Sauvola

Check out our other plugins:
- [Background Removal](https://github.com/occ-ai/obs-backgroundremoval) removes background from webcam without a green screen.
- [Detect](https://github.com/occ-ai/obs-detect) will detect and track >80 types of objects in any OBS source.
- [LocalVocal](https://github.com/occ-ai/obs-localvocal) speech AI assistant plugin for real-time, local transcription (captions), translation and more language functions
- [Polyglot](https://github.com/occ-ai/obs-polyglot) translation AI plugin for real-time, local translation to hunderds of languages
- [URL/API Source](https://github.com/occ-ai/obs-urlsource) will connect to any URL/API HTTP and get the data/image/audio to your scene.
- 🚧 Experimental 🚧 [CleanStream](https://github.com/occ-ai/obs-cleanstream) for real-time filler word (uh,um) and profanity removal from live audio stream

If you like this work, which is given to you completely free of charge, please consider supporting it https://github.com/sponsors/royshil or https://www.patreon.com/RoyShilkrot

## Download
Check out the [latest releases](https://github.com/occ-ai/obs-ocr/releases) for downloads and install instructions.


## Building

The plugin was built and tested on Mac OSX  (Intel & Apple silicon), Windows and Linux.

Start by cloning this repo to a directory of your choice.

### Mac OSX

Using the CI pipeline scripts, locally you would just call the zsh script. By default this builds a universal binary for both Intel and Apple Silicon. To build for a specific architecture please see `.github/scripts/.build.zsh` for the `-arch` options.

```sh
$ ./.github/scripts/build-macos -c Release
```

#### Install
The above script should succeed and the plugin files (e.g. `obs-ocr.plugin`) will reside in the `./release/Release` folder off of the root. Copy the `.plugin` file to the OBS directory e.g. `~/Library/Application Support/obs-studio/plugins`.

To get `.pkg` installer file, run for example
```sh
$ ./.github/scripts/package-macos -c Release
```
(Note that maybe the outputs will be in the `Release` folder and not the `install` folder like `pakage-macos` expects, so you will need to rename the folder from `build_x86_64/Release` to `build_x86_64/install`)

### Linux (Ubuntu)

Use the CI scripts again
```sh
$ ./.github/scripts/build-linux.sh
```

Copy the results to the standard OBS folders on Ubuntu
```sh
$ sudo cp -R release/RelWithDebInfo/lib/* /usr/lib/x86_64-linux-gnu/
$ sudo cp -R release/RelWithDebInfo/share/* /usr/share/
```
Note: The official [OBS plugins guide](https://obsproject.com/kb/plugins-guide) recommends adding plugins to the `~/.config/obs-studio/plugins` folder.

### Windows

Use the CI scripts again, for example:

```powershell
> .github/scripts/Build-Windows.ps1 -Target x64 -CMakeGenerator "Visual Studio 17 2022"
```

The build should exist in the `./release` folder off the root. You can manually install the files in the OBS directory.
