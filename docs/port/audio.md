# Audio

All sound is Ogg Vorbis, in `harvestClientData/sfx/`. The game asks for sounds by bare file name.
The backend-independent logic is `daisy::audio::CAudioDriver`
([`CAudioDriver.cpp`](../../src/daisy/audio/CAudioDriver.cpp); 67 of 70 functions exact, the rest
differ only in registers and cleanup placement). It decides what plays, at what volume, and when
other sounds are ducked. The OpenAL backend (`COpenALDriver`, which implements the `device…`
methods) loads, decodes and streams the files.

## The files

There are 109 `.ogg` files. They fall into three groups by how they are played:

| Group | Played with | Files |
|---|---|---|
| music | `playMusic` / `loadMusic` | `mus_hephaestus`, `mus_poseidon`, `mus_ares`, `mus_intro`, `ambience1`–`3` |
| voice lines | `playVoice` | `mode_{normal,wave,insane,rush,creative}`, `PlanetHover1`–`3`, `scenario_info{Welcome,Debriefing,Completed}`, `medusa{1,2,3}_phone_line`, `officer*`, `pilot*`, `piloteDone1`, `welcome{Creative,GoodLuck,LevelScores}`, `ingame_info{Boss,Victory,Wave}` |
| sound effects | `playSound`, `loopSound`, particles | everything else: buttons, buildings, aliens, weapons |

- **Particle sounds.** 43 of the effects are named only in `gfx/particles.pfx`. A particle type
  lists its sounds separated by `/`. When a particle starts, one is picked with `CRand::rand()`
  and played through `CPlayState::playParticleSound` (see [Game usage](#game-usage)).
- **Missing files.** The particle data also names six files that are not shipped:
  `AssignmentOuterSpace.ogg`, `DesertCommando.ogg` and `GunHit1`–`4.ogg`. Loading those fails, and
  the play call does nothing.
- **Unused files.** Six shipped files are not referenced by the executable, the particle data or
  the mods: `BtnHighlighted`, `BuildingMini`, `DropShipAccelerating`, `DropShipLandingFinal`,
  `MissileMini` and `gameintro`. `Achievement.ogg` is used; its name shares storage with the tail of
  `MiniAchievement.ogg` in the executable.

## Names and lookup

`CHarvestFullMain::init` sets the sound path to the directory behind `$GAME_RESOURCES$` plus
`/harvestClientData/sfx/` (`setSoundEffectPath`). Names passed to the driver are file names
relative to that path. Joining the two and opening the file is the backend's job.

The driver keeps three tables:

- **Sounds**: effects, as `std::map<CString<char>, CSoundInfoStub*>`.
- **Music**: music, as `std::map<CString<char>, CMusicInfoStub*>`.
- **Tracked sounds**: as `std::map<int, CTrackedSoundInfoStub*>`.

Keys are the names exactly as passed, compared byte by byte (`CString::operator<`), so they are
case-sensitive. Effects and music load on first use, or ahead of time with `loadSound` and
`loadMusic`, and stay loaded until the driver is destroyed. A name that fails to load is not
remembered, so every later call tries again. Voice lines are not cached: each `playVoice` loads a
fresh stream and deletes it when the line stops.

## Volume

The settings `settings:volumesfx` and `settings:volumemusic` (0 to 5, default 3, in
[`harvest.cfg`](save-games.md#profile-and-settings-files)) are applied at startup and from the
settings screen. Each becomes a gain: `clamp(volume × 20 × 0.01, 0, 1)`, so 0.0, 0.2 … 1.0.

- Before the settings are applied, the driver starts at effects 5 (gain 1) and music 3 (gain 0.6).
- `setMusicVolume` also calls `deviceUpdateMusic(music, music.Volume)` for every loaded track, so
  the new gain applies at once.
- With an effects gain of 0, `playSound` and `playOrientedSound` return without doing anything.
  With a music gain of 0, so does `playMusic`.
- The per-call volume is passed to the backend unchanged. Multiplying it by the gain is presumably
  the backend's job; see the [placeholder](#openal-backend).

## Rules

**`playSound(name, volume, pan, pitch)`** (Linux `0x5b3700`):

1. Return if the effects gain is 0.
2. Find or load the sound.
3. Return if this sound last started less than **50 ms** ago (`os::Timer::getTime()`, per sound
   name).
4. If a voice line exists and is still playing, and ducking is on (the default), multiply the
   volume by **0.25**. If the voice line has finished, stop and free it.
5. Record the start time and call `devicePlaySound`.

**`loopSound(name, volume, pan, pitch, fade)`** (Linux `0x5b3830`) works the same way, with three
differences: it does not check the gain, it has no 50 ms guard, and it ducks by 0.25 whenever a
voice plays, even with ducking turned off. `stopLoopSound(name)` stops the loop. Only the
dropship's engine uses a loop.

**Voice lines.** `playVoice(name)` (Linux `0x5b12f0`):

1. Remember whether a voice was playing, then stop it.
2. Load a new stream.
3. Play it at volume 1, without looping, flagged as voice.
4. If no voice was playing before and ducking is on, call `deviceDampenAllSounds(0.25)` to duck the
   sounds already playing.

There is one voice at a time. `isVoicePlaying` polls the stream and frees it when it has ended.
Undoing the duck when the voice ends is not in `CAudioDriver`; it is up to the backend.

**Music.**

- `playMusic(name, volume, loop)` (Linux `0x5b2890`) stores the volume on the track and starts it.
- `updateMusic(name, volume)` stores the volume and returns the backend's result. The main menu
  treats `false` as "not playing" and then calls `playMusic`.
- `stopMusic(name)` and `stopAllMusic()` stop tracks.
- `stopAllSounds()` stops all music, all tracked sounds and every effect.

**Positional sounds.**

- `playOrientedSound(name, volume, pitch, position, velocity)` has the same gain, repeat and duck
  rules as `playSound`. The base `devicePlayOrientedSound` does not do 3D. It plays the sound
  panned **+0.5** if the source's x is less than the listener's, **−0.5** if greater, and 0 if
  equal.
- `startTrackedSound(…)` returns a handle: 1, 2, 3 … in order, or −1 on failure.
  `stopAllTrackedSounds` restarts the numbering at 1. `updateTrackedSound(handle, …)` and
  `stopTrackedSound(handle)` act on that sound.
- `set3dCameraPosition(camera)` sets the listener from a scene camera with z negated (forward and
  up too), to turn the left-handed scene coordinates into OpenAL's right-handed ones.
  `set2dOrientation()` sets forward (0, 0, 1) and up (0, −1, 0).

The game itself uses none of the positional calls; it pans by hand (below).

## Game usage

- **Particles and other world sounds.** `CPlayState::playParticleSound(name, position)` measures
  dx and dy from the centre of the view. The sound is played only if `dx² + dy² ≤ W × H` (W and H
  are the screen size), at volume `1 − (dx² + dy²)/(W × H)`, pan `clamp(dx, −W, W)/W` (positive to
  the right), and pitch 1.
- **Dropship engine.** `CDropshipEntity` loops `DropShipEngine.ogg` while the dropship is in range,
  at volume `(1 − d²/range) × volume × 0.8`, where range = (W + 200)(H + 200).
- **In-game music.** The planet's `mus_*.ogg` plays once, without looping, at volume 1, and is
  restarted every 1200 s (`playPlanetMusic`). The campaign gets no planet music; `mus_intro.ogg`
  plays on a custom event.
- **Main menu ambience.** See [menu-scene.md](menu-scene.md#animation): `ambience1`–`3` loop at
  `1 − distance/100` near each planet.
- **Other calls.** Buttons play `BtnPressed.ogg` on every button click
  (`CHarvestSuperReceiver`). Info lines play their voice line, or `message.ogg` if they have none.
  Locked actions play `BtnDenial.ogg`.
- **Streaming.** `CGameMain` calls `periodicStreamUpdate()` once per frame, before the state
  update. It is empty in `CAudioDriver`; the backend overrides it to refill its streams.

## OpenAL backend

`daisy::audio::COpenALDriver` ([`COpenALDriver.cpp`](../../src/daisy/audio/COpenALDriver.cpp)) is the
Linux backend under `CAudioDriver`.

- **Start-up.** `alutInit`. `setOpenAlDistanceModel(m)` calls `alDistanceModel(0xD000 + m)`; the other
  distance settings stay at OpenAL's defaults.
- **Sources.** A pool of 32 sources, searched round-robin for a free one; when none is free, a source
  playing a lower-priority sound is taken over.
- **Effects** are decoded whole into one buffer: `.ogg` through libvorbisfile with memory callbacks,
  `.wav` through `alutCreateBufferFromFile`. The decode buffer is sized as bitrate × duration, which
  can cut off the end of a file whose bitrate varies.
- **Music and voice lines** stream with 10 buffers of 4096 bytes, decoded with `ov_read` as 16-bit
  signed little-endian and refilled by `periodicStreamUpdate` every frame. Looping is done by seeking
  the stream back to the start; the source itself never loops.
- **Parameters.** The gain is the volume passed in; the pitch is the pitch times the global pitch
  modifier. Oriented sounds are panned by source position (see the pan quirk in
  [original-bugs.md](original-bugs.md)).
