[Tobo Exciter](../README.md) › Tested environment and known limits

<!-- Generated from manual/manual_en.md by tools/make_docs.py. Change the source, not this file. -->

## Tested environment and known limits

- Tested on Windows 11 with REAPER 7.86 at 48 kHz. Other DAWs have not been tested yet.
- A test program without a DAW (`StressTest.exe`) runs the plug-in at sample rates from 8 to 384 kHz and block sizes from 1 to 4096 samples, feeds it invalid input values, corrupted projects and corrupted preset files, and operates the panel with random mouse movements.
- Windows only, VST3 only.
- In stereo, the two channels are two separate pedals. The hardware is mono.
- The model is based on a single unit. The noise of the hardware is not modeled.
- When the pedal is driven very hard, the peaks of the output can be about twice as high as the level at which the model clips. Use OUTPUT to keep the level in range.
- The labels use the font Bahnschrift, which is part of Windows 10 and Windows 11.

If something does not work, please open an [issue](https://github.com/tobokegao/ToboExciter/issues) with the name and version of your DAW.

---

← [Latency and oversampling](latency-and-oversampling.md)   |   [Contents](../README.md#documentation)   |   [Troubleshooting](troubleshooting.md) →
