# Bifrost ambience

Six positional recordings are available through **Audio Emitter** in the GM catalogue. Place an emitter, double-click it, choose a sound, adjust its controls, and turn **Playing** on. Emitters can be moved and deleted through normal GM controls.

| Preset | Recording author | Source | Length |
| --- | --- | --- | --- |
| Indoor crowd | Ev-Dawg | [435515](https://freesound.org/people/Ev-Dawg/sounds/435515/) | 128.88 s |
| Outdoor conversation | szalonegacie | [232174](https://freesound.org/people/szalonegacie/sounds/232174/) | 68.79 s |
| Dog barking/growling | CmdRobot | [439535](https://freesound.org/people/CmdRobot/sounds/439535/) | 14.12 s |
| Shouted battle cry | WelvynZPorterSamples | [621341](https://freesound.org/people/WelvynZPorterSamples/sounds/621341/) | 7.72 s |
| Distant rifle battle | qubodup | [188839](https://freesound.org/people/qubodup/sounds/188839/) | 35.83 s |
| Single M16 shot | qubodup | [162403](https://freesound.org/people/qubodup/sounds/162403/) | 0.94 s |

These source pages identified the recordings as [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) when acquired on 5 September 2026. Attribution is retained here for provenance. They are not extracted Arma 3 assets.

The public high-quality MP3 previews were converted to mono, 48 kHz, signed 16-bit PCM WAV for Workbench import. These are lossy preview sources, not lossless masters. Conversion downmixed channels, resampled, limited peak amplitude to 0.89, and applied a 3 ms edge fade. `sources.json` records source and output hashes, exact preview URLs, formats and durations. Workbench generates platform runtime resources from the WAV metadata; no conversion package is a runtime dependency.

Controls cover preset, playing, volume, radius, fade-in, fade-out, looping, wall obstruction and reverb amount. Choose zero fade-in for an immediate gunshot. A one-shot setting stops at the recording duration; looping repeats the recording. Playback configuration is server-owned and replicated; each listener renders positional audio locally. Joining listeners receive the current configuration, but recording phase is not sample-synchronised between clients.

Wall obstruction traces between emitter and listener, reducing gain and applying a low-pass filter. Reverberation uses the game's native small/medium/large/exterior sends and environment signals. Its result depends on the map's acoustic setup. This approximates blocked and reflected sound; it is not a new physical sound-ray simulation. Indoor, outdoor and wall-transition listening tests are still required.
