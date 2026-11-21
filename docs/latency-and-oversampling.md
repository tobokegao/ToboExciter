[Tobo Exciter](../README.md) › Latency and oversampling

<!-- Generated from manual/manual_en.md by tools/make_docs.py. Change the source, not this file. -->

## Latency and oversampling

The plug-in reports its delay to the DAW, which compensates for it.

| Sample rate | 2x | 4x | 8x |
|---|---|---|---|
| 44.1 and 48 kHz | 3 samples | 4 samples | 5 samples |
| 88.2 and 96 kHz | 0 (no oversampling) | 3 samples (2x) | 4 samples (4x) |
| 176.4 and 192 kHz | 0 (no oversampling) | 0 (no oversampling) | 3 samples (2x) |

At sample rates of 88.2 and 96 kHz the plug-in oversamples one step less than the setting says (4x becomes 2x, and 2x becomes no oversampling), and at higher rates two steps less. The model then runs at the same sample rate as it does at 44.1 or 48 kHz.

With the effect switched off, the sound still passes through the oversampling filters. Its level does not change, but its phase shifts slightly near the top of the spectrum.

---

← [Parameters for automation](parameters-for-automation.md)   |   [Contents](../README.md#documentation)   |   [Tested environment and known limits](tested-environment-and-known-limits.md) →
