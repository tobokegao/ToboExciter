[Tobo Exciter](../README.md) › Troubleshooting

<!-- Generated from manual/manual_en.md by tools/make_docs.py. Change the source, not this file. -->

## Troubleshooting

| Symptom | What to check |
|---|---|
| The DAW does not list the plug-in | The whole folder `Tobo Exciter.vst3` must be in `C:\Program Files\Common Files\VST3\`. Re-scan the plug-ins. The plug-in is 64-bit only. |
| The effect is barely audible | Check whether the lamp is off (click the foot switch to turn it on) or BALANCE is turned fully left. Also try raising INPUT, as a quiet signal will not clip. |
| The sound is too harsh or too loud | If it is too harsh, lower INPUT and raise OUTPUT by the same amount, or turn COLOR and MULTIPEAK down. If it is too loud, lower OUTPUT. |
| A tone stays on one pitch | This is how the pedal behaves with COLOR, BALANCE and MULTIPEAK turned up. Turn COLOR down to weaken it. |
| The sound stops for a moment | This happens when OVERSAMPLING is changed, because the filters are rebuilt. |
| The window is too small or too large | Click SIZE and choose a size, or drag the lower right corner of the window. |

---

← [Tested environment and known limits](tested-environment-and-known-limits.md)   |   [Contents](../README.md#documentation)   |   [Building from source](building-from-source.md) →
