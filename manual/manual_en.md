# Tobo Exciter — a 1980s exciter pedal as a plug-in, modeled on hardware measurements

Tobo Exciter (TE-02) is a free VST3 effect for Windows. It models an exciter pedal of the 1980s: a filter with a sharp peak, six fixed resonances and the way the circuit clips. It adds a bright, biting edge above the note. With the knobs turned up, the pedal sings at a single pitch of its own on top of what you play.

![Tobo Exciter](docs/img/screenshot.png)

- **Format**: VST3, 64-bit Windows, mono or stereo
- **Price**: free
- **License**: AGPL-3.0-or-later (the source is in this repository)

This plug-in is an independent work. It is not affiliated with, endorsed by, or sponsored by any hardware manufacturer.

## Features

- **Modeled from measurements**: the frequency response and the distortion of a real unit were measured with sine sweeps at several levels and at many knob positions. The model was then compared with a guitar re-amped through the same unit.
- **Four knobs, as on the pedal**: FREQUENCY, COLOR, BALANCE and MULTIPEAK. Their tapers follow the measured unit.
- **A foot switch that fades**: a click on the foot switch turns the effect on and off with a short crossfade, so there is no pop. The DAW's bypass uses the same crossfade.
- **INPUT and OUTPUT with meters**: INPUT sets how hard the pedal is driven. The meters show the level into the model and out of the plug-in.
- **Oversampling**: 2x, 4x or 8x. The model clips hard, so a higher setting gives less aliasing.
- **Presets**: eight built-in presets, and preset files that you can save and load.
- **A panel that scales**: from 60% to 200%, by a menu or by dragging the corner of the window.

## Install

1. Download `ToboExciter_{VERSION}_win64.zip` from the [latest release](https://github.com/tobokegao/ToboExciter/releases/latest) and unzip it.
2. Copy the folder `Tobo Exciter.vst3` (the whole folder, not just the file inside it) to `C:\Program Files\Common Files\VST3\`.
3. Start your DAW and let it scan for new plug-ins. In REAPER: `Options` > `Preferences` > `Plug-ins` > `VST` > `Re-scan`.
4. The plug-in appears as **Tobo Exciter** (Tobokegao).

To update, close the DAW and replace the folder with the new one. To uninstall, delete the folder. Preset files that you saved stay where you saved them (by default in `Documents\Tobo Exciter Presets`).

No installer or extra runtime is needed. The plug-in is not code-signed.

## Quick start

1. Put Tobo Exciter on a guitar track, before your amp or distortion. The pedal was made to sit between the guitar and the amplifier.
2. Play. With all four knobs at 12 o'clock, the sound gets a bright edge.
3. Turn **BALANCE** to the right for more of the filtered sound and less of the direct sound.
4. Turn **COLOR** to the right to sharpen the filter's peak, and move **FREQUENCY** to choose where the peak sits.
5. Turn **MULTIPEAK** to the right to add the six fixed peaks.
6. Set **INPUT** so that the pedal is driven as hard as you like, then bring the level back with **OUTPUT**.
7. Click the **foot switch** to compare it with the sound without the effect.

To hear a sound that stays on one pitch, try the presets **One Note** and **Thin Scream**.

## The panel

![The panel](docs/img/panel_parts.png)

| Part | What it does |
|---|---|
| **FREQUENCY** | Where the filter begins. The scale on the panel reads 600 Hz to 7 kHz, as on the hardware. The modeled filter moves from about 690 Hz to 6.8 kHz, matching the measurements. |
| **COLOR** | How sharp the filter's peak is. Fully left, the filter is smooth. Fully right, it rings. |
| **BALANCE** | Mixes the direct sound and the filtered sound. Fully left is the direct sound alone, and fully right is the filtered sound alone. |
| **MULTIPEAK** | Adds six fixed peaks between 1 and 5 kHz (at about 1.0, 1.4, 1.9, 2.4, 3.5 and 5.0 kHz). |
| **Foot switch** | A click turns the effect on and off. The lamp above the name plate lights while the effect is on. |
| **INPUT** | The level into the model, from -30 to +30 dB. It sets how hard the pedal is driven. |
| **OUTPUT** | The level out of the model, from -30 to +30 dB. |
| **Meters** | Left: the level into the model, after INPUT. Right: the level out of the plug-in. The bottom of each meter is -60 dB. The part above 0 dBFS is red. |
| **Preset box** | Shows the name of the preset. The arrows switch to the previous or next built-in preset. |
| **OVERSAMPLING** | Clicking cycles through 2x, 4x and 8x. |
| **SIZE** | Opens a menu with the sizes of the panel (75%, 100%, 125%, 150% and 200%). Sizes that do not fit on your screen are greyed out. |
| **LOAD** | Loads a preset from a file. |
| **SAVE** | Saves the settings as a preset file. |
| **INFO** | Shows a short description of the plug-in and its controls. A click anywhere closes it. |

- Drag a knob up or down to turn it. A double-click sets it back to its default.
- Drag the lower right corner of the window to make the panel larger or smaller. The proportions stay the same. The size is saved with the project.

### INPUT and OUTPUT at their defaults

The pedal reacts to the level of the guitar: a louder signal clips more. The defaults (INPUT -4.9 dB, OUTPUT +4.9 dB) match the recording setup the model was checked with. Your recordings may be louder or quieter than that. If the effect is too tame, raise INPUT and lower OUTPUT by the same amount. If it is too harsh, do the opposite.

## Presets and preset files

The eight built-in presets set the four knobs. INPUT, OUTPUT and OVERSAMPLING stay as they are.

| Preset | FREQUENCY | COLOR | BALANCE | MULTIPEAK |
|---|---|---|---|---|
| Default | 5 | 5 | 5 | 5 |
| Gentle Air | 6.5 | 3 | 3.5 | 4 |
| Bite | 5 | 7.5 | 5 | 5 |
| Six Peaks | 5 | 5 | 5 | 10 |
| Filter Only | 5 | 5 | 10 | 5 |
| Low Honk | 0 | 7.5 | 5 | 5 |
| One Note | 1.6 | 10 | 10 | 10 |
| Thin Scream | 4.4 | 10 | 10 | 10 |

**SAVE** writes the four knobs, INPUT, OUTPUT and OVERSAMPLING to a file with the `.tepreset` extension. The first time, it opens the folder `Documents\Tobo Exciter Presets`. **LOAD** loads one of these files. The preset box then shows the name of the file. The state of the foot switch is not part of a preset.

## Parameters for automation

| Name in the DAW | Range | Default |
|---|---|---|
| Frequency | 0 to 10 | 5 |
| Color | 0 to 10 | 5 |
| Balance | 0 to 10 | 5 |
| Multipeak | 0 to 10 | 5 |
| Input | -30 to +30 dB | -4.9 dB |
| Output | -30 to +30 dB | +4.9 dB |
| Effect | off, on | on |
| Bypass | off, on | off |
| Oversampling | 2x, 4x, 8x | 4x |

For the four knobs, 0 is fully left, 5 is 12 o'clock and 10 is fully right. Knob movements are smoothed over about 10 ms. **Effect** is the foot switch. **Bypass** is the DAW's bypass. Both fade over 12 ms. Changing **Oversampling** interrupts the sound for a moment, so it is not meant to be automated.

## Latency and oversampling

The plug-in reports its delay to the DAW, which compensates for it.

| Sample rate | 2x | 4x | 8x |
|---|---|---|---|
{LATENCY}

At sample rates of 88.2 and 96 kHz the plug-in oversamples one step less than the setting says (4x becomes 2x, and 2x becomes no oversampling), and at higher rates two steps less. The model then runs at the same sample rate as it does at 44.1 or 48 kHz.

With the effect switched off, the sound still passes through the oversampling filters. Its level does not change, but its phase shifts slightly near the top of the spectrum.

## Tested environment and known limits

- Tested on Windows 11 with REAPER 7.86 at 48 kHz. Other DAWs have not been tested yet.
- A test program without a DAW (`StressTest.exe`) runs the plug-in at sample rates from 8 to 384 kHz and block sizes from 1 to 4096 samples, feeds it invalid input values, corrupted projects and corrupted preset files, and operates the panel with random mouse movements.
- Windows only, VST3 only.
- In stereo, the two channels are two separate pedals. The hardware is mono.
- The model is based on a single unit. The noise of the hardware is not modeled.
- When the pedal is driven very hard, the peaks of the output can be about twice as high as the level at which the model clips. Use OUTPUT to keep the level in range.
- The labels use the font Bahnschrift, which is part of Windows 10 and Windows 11.

If something does not work, please open an [issue](https://github.com/tobokegao/ToboExciter/issues) with the name and version of your DAW.

## Troubleshooting

| Symptom | What to check |
|---|---|
| The DAW does not list the plug-in | The whole folder `Tobo Exciter.vst3` must be in `C:\Program Files\Common Files\VST3\`. Re-scan the plug-ins. The plug-in is 64-bit only. |
| The effect is barely audible | Check whether the lamp is off (click the foot switch to turn it on) or BALANCE is turned fully left. Also try raising INPUT, as a quiet signal will not clip. |
| The sound is too harsh or too loud | If it is too harsh, lower INPUT and raise OUTPUT by the same amount, or turn COLOR and MULTIPEAK down. If it is too loud, lower OUTPUT. |
| A tone stays on one pitch | This is how the pedal behaves with COLOR, BALANCE and MULTIPEAK turned up. Turn COLOR down to weaken it. |
| The sound stops for a moment | This happens when OVERSAMPLING is changed, because the filters are rebuilt. |
| The window is too small or too large | Click SIZE and choose a size, or drag the lower right corner of the window. |

## Building from source

You need Visual Studio 2022 or later (or the Build Tools) with the C++ workload and CMake.

```
git clone https://github.com/tobokegao/ToboExciter.git
cd ToboExciter
git clone --depth 1 --branch 9.0.5 https://github.com/juce-framework/JUCE.git
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

The plug-in is then in `build/ToboExciter_artefacts/Release/VST3/Tobo Exciter.vst3`.

- `Source/Engine.h`: the model.
- `Source/PluginProcessor.*`: the parameters, the oversampling, the presets and the saved state.
- `Source/PluginEditor.*`: the panel.
- `Assets/`: the pictures of the panel. They are renders of a 3D model made for this plug-in.
- `test/panel_shot.cpp`: builds `PanelShot.exe`, which draws the panel to PNG files without a DAW.
- `test/stress.cpp`: builds `StressTest.exe`, the test program described above.
- `manual/` and `tools/make_docs.py`: the source of this manual and the script that makes `README.md`, `README_ja.md`, `docs/` and the PDF files from it.

## License and credits

Tobo Exciter is free software under the GNU Affero General Public License, version 3 or later (`LICENSE`). Copyright (C) 2026 tobokegao.

It is built with the JUCE framework (AGPLv3) and the VST 3 SDK (MIT License). VST is a registered trademark of Steinberg Media Technologies GmbH. The licenses of the libraries in the binary are listed in `CREDITS.md` and `THIRD_PARTY_LICENSES.txt`.

This plug-in is an independent work. It is not affiliated with, endorsed by, or sponsored by any hardware manufacturer. All product names belong to their respective owners.

## Version history

### 0.1.1 ({DATE})

- The SIZE menu no longer offers sizes that do not fit on the screen. In 0.1.0, choosing such a size made the DAW cut the window short, which left an empty strip beside the panel.

### 0.1.0 (2026-11-21)

- First release.
