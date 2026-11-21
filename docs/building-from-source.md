[Tobo Exciter](../README.md) › Building from source

<!-- Generated from manual/manual_en.md by tools/make_docs.py. Change the source, not this file. -->

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

---

← [Troubleshooting](troubleshooting.md)   |   [Contents](../README.md#documentation)   |   [License and credits](license-and-credits.md) →
