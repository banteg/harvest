// Tests the audio backend (port/src/audio/) with the original game's sounds, through CAudioDriver's
// own calls. The checks render offline (no device) so the numbers are exact; --play also plays a
// short scene on the default device, to listen to.
//
//   zig build test-audio -- <sfx path> [--wav out.wav] [--play]
//
// <sfx path> is the sound effect path the game would set: a directory such as
// ../orig/1.18-linux-amd64/harvestClientData/sfx/, or a path into a zip archive (sfx.zip/), which
// reads the files through the game's archive reader.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <SDL3/SDL_timer.h>
#include "audio/CMiniaudioDriver.h"
#include "daisy/io/CFileSystem.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"

using port::audio::CMiniaudioDriver;
using port::audio::Mixer;
typedef ox::core::CVector3d<float> Vec3;

namespace {

const unsigned int RATE = 48000;
//! The game calls periodicStreamUpdate once per frame; the tests run at 60 frames per second.
const unsigned int FRAME = RATE / 60;

int g_failures = 0;
ma_encoder* g_wav = 0;

void check(bool condition, const char* what)
{
    std::printf("%s  %s\n", condition ? "ok  " : "FAIL", what);
    if (!condition)
        ++g_failures;
}

bool near(double value, double expected, double tolerance)
{
    return std::fabs(value - expected) <= tolerance * std::fabs(expected);
}

struct Level
{
    double Left;
    double Right;
};

//! Renders the driver offline for a while, one game frame at a time; returns the RMS per channel.
Level render(CMiniaudioDriver& driver, double seconds)
{
    std::vector<float> block(FRAME * 2);
    double left = 0.0, right = 0.0;
    unsigned int frames = (unsigned int)(seconds * 60.0);
    for (unsigned int i = 0; i < frames; ++i)
    {
        driver.periodicStreamUpdate();
        driver.getMixer().mix(&block[0], FRAME);
        if (g_wav)
            ma_encoder_write_pcm_frames(g_wav, &block[0], FRAME, 0);
        for (unsigned int f = 0; f < FRAME; ++f)
        {
            left += block[f * 2] * block[f * 2];
            right += block[f * 2 + 1] * block[f * 2 + 1];
        }
    }
    Level level = { std::sqrt(left / (frames * FRAME)), std::sqrt(right / (frames * FRAME)) };
    return level;
}

//! Renders two drivers side by side; returns the RMS of their difference (left channel), and of
//! the first driver's output in first when given.
double renderDifference(CMiniaudioDriver& a, CMiniaudioDriver& b, double seconds, double* first = 0)
{
    std::vector<float> blockA(FRAME * 2), blockB(FRAME * 2);
    double sum = 0.0, sumA = 0.0;
    unsigned int frames = (unsigned int)(seconds * 60.0);
    for (unsigned int i = 0; i < frames; ++i)
    {
        a.periodicStreamUpdate();
        b.periodicStreamUpdate();
        a.getMixer().mix(&blockA[0], FRAME);
        b.getMixer().mix(&blockB[0], FRAME);
        for (unsigned int f = 0; f < FRAME; ++f)
        {
            double d = blockA[f * 2] - blockB[f * 2];
            sum += d * d;
            sumA += blockA[f * 2] * blockA[f * 2];
        }
    }
    if (first)
        *first = std::sqrt(sumA / (frames * FRAME));
    return std::sqrt(sum / (frames * FRAME));
}

//! The 50 ms repeat guard uses the wall clock; wait it out before replaying a sound.
void waitRepeatGuard()
{
    SDL_Delay(60);
}

std::vector<std::string> listSounds(ox::io::IFileSystem* fileSystem, const char* path)
{
    std::vector<std::string> names;
    ox::io::IFileList* list = fileSystem->createFileList("*.ogg", path, ox::io::EFL_FILES);
    if (!list)
        return names;
    for (int i = 0; i < list->getFileCount(); ++i)
        names.push_back(list->getFileName(i));
    list->drop();
    return names;
}

void testEffects(ox::io::IFileSystem* fileSystem, const std::string& path, const std::vector<std::string>& sounds)
{
    std::printf("\n-- effects\n");
    CMiniaudioDriver driver(fileSystem, RATE);
    driver.setSoundEffectPath(path.c_str());

    unsigned int loaded = 0;
    for (size_t i = 0; i < sounds.size(); ++i)
        loaded += driver.loadSound(sounds[i].c_str()) ? 1 : 0;
    std::printf("      %u of %u .ogg files load\n", loaded, (unsigned int)sounds.size());
    check(!sounds.empty() && loaded == sounds.size(), "every listed .ogg loads");
    check(!driver.loadSound("GunHit1.ogg"), "a missing file (GunHit1.ogg, named by the particles) fails");
    check(!driver.loadSound("BtnPressed.mp3"), "a name not ending in .ogg or .wav fails");

    Mixer& mixer = driver.getMixer();

    // Centre: OpenAL Soft renders a mono source at the centre with sqrt(1/2) on each side.
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    check(mixer.getPlayingCount() == 1, "a repeat within 50 ms is dropped");
    Level centre = render(driver, 1.0);
    std::printf("      centre rms %.5f %.5f\n", centre.Left, centre.Right);
    check(centre.Left > 0.001 && centre.Left == centre.Right, "pan 0 plays equally on both sides");
    check(mixer.getPlayingCount() == 0, "a one-shot stops at its end");

    // Full right: the source sits at (2, 0, 0.1), so it is attenuated by 1/2.0025 (inverse distance
    // clamped) and panned almost fully right.
    waitRepeatGuard();
    driver.playSound("BtnPressed.ogg", 1.0f, 1.0f, 1.0f);
    Level right = render(driver, 1.0);
    double distance = std::sqrt(4.01);
    double alpha = (std::asin(2.0 / distance) + M_PI / 2) / 2;
    std::printf("      right rms %.5f %.5f\n", right.Left, right.Right);
    check(near(right.Right, centre.Right * std::sin(alpha) / distance / std::sqrt(0.5), 1e-3) &&
              near(right.Left, centre.Left * std::cos(alpha) / distance / std::sqrt(0.5), 1e-3),
        "pan 1 is attenuated by distance and panned right (OpenAL's rendering)");

    waitRepeatGuard();
    driver.playSound("BtnPressed.ogg", 1.0f, -0.5f, 1.0f);
    Level left = render(driver, 1.0);
    check(left.Left > left.Right * 10, "pan -0.5 plays on the left");

    // Pitch: twice the pitch plays in half the time; the global modifier multiplies.
    waitRepeatGuard();
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 2.0f);
    render(driver, 0.4);
    check(mixer.getPlayingCount() == 0, "pitch 2 plays the 0.65 s sound in under 0.4 s");
    waitRepeatGuard();
    driver.setGlobalPitchModifier(2.0f);
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    render(driver, 0.4);
    check(mixer.getPlayingCount() == 0, "the global pitch modifier multiplies the pitch");
    driver.setGlobalPitchModifier(1.0f);
    waitRepeatGuard();
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    render(driver, 0.4);
    check(mixer.getPlayingCount() == 1, "pitch 1 is still playing after 0.4 s");
    render(driver, 0.5);

    // The pool: 32 sources; a 33rd sound is dropped (equal priorities never steal).
    if (sounds.size() > 32)
    {
        for (size_t i = 0; i < 33; ++i)
            driver.playSound(sounds[i].c_str(), 0.1f, 0.0f, 1.0f);
        check(mixer.getPlayingCount() == 32, "the 33rd simultaneous sound finds no free source");
        render(driver, 11.0);
        check(mixer.getPlayingCount() == 0, "the pool drains");
    }

    // Effect volume 0 silences playSound.
    driver.setSoundEffectVolume(0);
    waitRepeatGuard();
    driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    check(mixer.getPlayingCount() == 0, "effect volume 0 plays nothing");
    driver.setSoundEffectVolume(5);

    // Loops: the dropship engine loops past its 7 s length.
    driver.loopSound("DropShipEngine.ogg", 1.0f, 0.0f, 1.0f, 0.0f);
    render(driver, 7.5);
    Level looping = render(driver, 0.5);
    check(mixer.getPlayingCount() == 1 && looping.Left > 0.001, "loopSound keeps playing past the end");
    // The loop's cone (30/75 degrees, facing -z) mutes it away from the centre: original behaviour.
    driver.loopSound("DropShipEngine.ogg", 1.0f, 0.5f, 1.0f, 0.0f);
    Level coned = render(driver, 0.5);
    std::printf("      loop at pan 0.5 rms %.6f %.6f\n", coned.Left, coned.Right);
    check(coned.Left == 0.0 && coned.Right == 0.0, "a loop panned 0.5 is outside its cone (silent)");
    driver.loopSound("DropShipEngine.ogg", 1.0f, 0.01f, 1.0f, 0.0f);
    Level inside = render(driver, 0.5);
    check(inside.Right > inside.Left && inside.Left > 0.0, "a loop panned 0.01 is inside its cone");
    driver.stopLoopSound("DropShipEngine.ogg");
    check(mixer.getPlayingCount() == 0, "stopLoopSound stops it");

    // Oriented sounds: absolute positions under the 2d listener (facing +z, up -y), with the cone.
    driver.playOrientedSound("BtnDenial.ogg", 1.0f, 1.0f, Vec3(1.0f, 0.0f, 5.0f), Vec3());
    Level oriented = render(driver, 0.5);
    check(oriented.Right > oriented.Left && oriented.Left > 0.0, "an oriented sound at +x in front plays right");
    render(driver, 2.0);
    driver.playOrientedSound("BtnDenial.ogg", 1.0f, 1.0f, Vec3(-5.0f, 0.0f, 0.0f), Vec3());
    Level outside = render(driver, 0.5);
    std::printf("      beside rms %.6f %.6f\n", outside.Left, outside.Right);
    check(outside.Left == 0.0 && outside.Right == 0.0, "an oriented sound beside the listener is outside the cone");
    render(driver, 2.0);

    // Tracked sounds: handles count from 1 and restart after stopAllTrackedSounds.
    int first = driver.startTrackedSound("Mining.ogg", 1.0f, 1.0f, Vec3(1.0f, 0.0f, 5.0f), Vec3(), 0.0f);
    int second = driver.startTrackedSound("Mining.ogg", 1.0f, 1.0f, Vec3(-1.0f, 0.0f, 5.0f), Vec3(), 0.0f);
    check(first == 1 && second == 2, "tracked sound handles are 1, 2");
    check(driver.startTrackedSound("GunHit2.ogg", 1.0f, 1.0f, Vec3(), Vec3(), 0.0f) == -1,
        "a tracked sound that does not load returns -1");
    driver.stopTrackedSound(second);
    Level tracked = render(driver, 0.5);
    check(tracked.Right > tracked.Left, "the remaining tracked sound plays on the right");
    driver.updateTrackedSound(first, 1.0f, 1.0f, Vec3(-1.0f, 0.0f, 5.0f), Vec3());
    tracked = render(driver, 0.5);
    check(tracked.Left > tracked.Right, "updateTrackedSound moves it to the left");
    render(driver, 4.0);
    check(mixer.getPlayingCount() == 1, "a tracked sound loops");
    driver.stopAllTrackedSounds();
    check(mixer.getPlayingCount() == 0, "stopAllTrackedSounds stops it");
    check(driver.startTrackedSound("Mining.ogg", 1.0f, 1.0f, Vec3(), Vec3(), 0.0f) == 1,
        "handles restart at 1");
    driver.stopAllSounds();
    check(mixer.getPlayingCount() == 0, "stopAllSounds stops tracked sounds");
}

void testMusic(ox::io::IFileSystem* fileSystem, const std::string& path)
{
    std::printf("\n-- music and voice lines\n");
    CMiniaudioDriver driver(fileSystem, RATE);
    driver.setSoundEffectPath(path.c_str());
    Mixer& mixer = driver.getMixer();

    CMiniaudioDriver reference(fileSystem, RATE);
    reference.setSoundEffectPath(path.c_str());
    reference.playMusic("mus_intro.ogg", 1.0f, true);
    driver.playMusic("mus_intro.ogg", 1.0f, true);
    double level = 0.0;
    double difference = renderDifference(reference, driver, 3.0, &level);
    Level music = render(reference, 0.5);
    std::printf("      music rms %.5f %.5f\n", music.Left, music.Right);
    check(level > 0.001 && music.Left == music.Right, "music streams, centred");
    check(difference == 0.0, "streaming is deterministic");
    render(driver, 0.5);
    check(driver.updateMusic("mus_intro.ogg", 0.5f), "updateMusic reports a playing track");
    difference = renderDifference(reference, driver, 3.0, &level);
    check(level > 0.001 && near(difference, level * 0.5, 1e-4), "updateMusic sets the gain");
    driver.stopMusic("mus_intro.ogg");
    check(!driver.updateMusic("mus_intro.ogg", 1.0f) && mixer.getPlayingCount() == 0, "stopMusic stops it");
    driver.playMusic("mus_intro.ogg", 1.0f, true);
    check(driver.updateMusic("mus_intro.ogg", 1.0f), "a stopped track plays again");
    driver.stopAllMusic();
    check(!driver.updateMusic("mus_intro.ogg", 1.0f), "stopAllMusic stops it");

    // A track played once ends and is released; a looping one seeks back to the start.
    driver.playMusic("mode_rush.ogg", 1.0f, false);
    render(driver, 0.5);
    check(driver.updateMusic("mode_rush.ogg", 1.0f), "a non-looping track plays");
    render(driver, 1.0);
    check(!driver.updateMusic("mode_rush.ogg", 1.0f), "and stops at its end (0.95 s)");
    driver.playMusic("MiniPoff.ogg", 1.0f, true);
    render(driver, 1.0);
    Level loop = render(driver, 0.5);
    check(driver.updateMusic("MiniPoff.ogg", 1.0f) && loop.Left > 0.001, "a looping 0.13 s track keeps playing");
    driver.stopAllMusic();
    check(!driver.updateMusic("missing.ogg", 1.0f), "updateMusic on an unknown track is false");
    driver.playMusic("missing.ogg", 1.0f, true);
    check(!driver.updateMusic("missing.ogg", 1.0f), "a missing track loads (as the original) but does not play");

    driver.setMusicVolume(0);
    driver.playMusic("mus_intro.ogg", 1.0f, true);
    check(!driver.updateMusic("mus_intro.ogg", 1.0f), "music volume 0 plays nothing");
    driver.setMusicVolume(3);

    // Voice lines: one at a time, freed when they end.
    driver.playVoice("mode_normal.ogg");
    check(driver.isVoicePlaying(), "a voice line plays");
    render(driver, 1.0);
    check(driver.isVoicePlaying(), "still playing after 1 s");
    render(driver, 0.5);
    check(!driver.isVoicePlaying(), "ended after 1.5 s (1.23 s line)");
    driver.playVoice("medusa1_phone_line.ogg");
    driver.playVoice("mode_wave.ogg");
    render(driver, 0.5);
    check(driver.isVoicePlaying() && mixer.getPlayingCount() == 1, "a new voice line replaces the old one");
    driver.stopVoice();
    check(!driver.isVoicePlaying() && mixer.getPlayingCount() == 0, "stopVoice stops it");
}

void testDucking(ox::io::IFileSystem* fileSystem, const std::string& path)
{
    std::printf("\n-- voice ducking\n");
    // The effect alone, and the difference an effect makes on top of a voice line.
    CMiniaudioDriver alone(fileSystem, RATE), silent(fileSystem, RATE);
    CMiniaudioDriver withVoice(fileSystem, RATE), voiceOnly(fileSystem, RATE);
    CMiniaudioDriver* drivers[] = { &alone, &silent, &withVoice, &voiceOnly };
    for (int i = 0; i < 4; ++i)
        drivers[i]->setSoundEffectPath(path.c_str());

    alone.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    double reference = renderDifference(alone, silent, 0.5);

    withVoice.playVoice("mode_normal.ogg");
    voiceOnly.playVoice("mode_normal.ogg");
    withVoice.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    double ducked = renderDifference(withVoice, voiceOnly, 0.5);
    std::printf("      effect rms %.5f, under a voice %.5f\n", reference, ducked);
    check(near(ducked, reference * 0.25, 1e-3), "an effect started under a voice line plays at 0.25");

    render(withVoice, 1.0);
    render(voiceOnly, 1.0);
    waitRepeatGuard();
    withVoice.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    render(alone, 1.0);
    alone.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
    double after = renderDifference(withVoice, alone, 0.5);
    check(!withVoice.isVoicePlaying() && after < 1e-6, "after the voice line ends effects play at full volume");
}

void play(ox::io::IFileSystem* fileSystem, const std::string& path)
{
    std::printf("\n-- playing on the default device\n");
    CMiniaudioDriver driver(fileSystem);
    if (!driver.getMixer().isAvailable())
    {
        check(false, "the playback device opens");
        return;
    }
    std::printf("      device rate %u\n", driver.getMixer().getSampleRate());
    driver.setSoundEffectPath(path.c_str());
    driver.setMusicVolume(3);
    driver.setSoundEffectVolume(3);
    driver.loadSound("BtnPressed.ogg");
    driver.loadSound("AlienDeath.ogg");
    driver.playMusic("mus_intro.ogg", 1.0f, true);

    struct Cue
    {
        int Frame;
        const char* What;
    } cues[] = {
        { 60, "left" }, { 90, "centre" }, { 120, "right" }, { 180, "voice" }, { 200, "ducked" },
        { 330, "loop" }, { 480, "stop loop" }, { 540, "end" },
    };
    int cue = 0;
    for (int frame = 0; frame <= cues[7].Frame; ++frame)
    {
        if (frame == cues[cue].Frame)
        {
            std::printf("      %s\n", cues[cue].What);
            switch (cue)
            {
            case 0: driver.playSound("AlienDeath.ogg", 1.0f, -0.8f, 1.0f); break;
            case 1: driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f); break;
            case 2: driver.playSound("AlienDeath.ogg", 1.0f, 0.8f, 1.0f); break;
            case 3: driver.playVoice("scenario_infoWelcome.ogg"); break;
            case 4: driver.playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f); break;
            case 5: driver.loopSound("DropShipEngine.ogg", 0.8f, 0.0f, 1.0f, 0.0f); break;
            case 6: driver.stopLoopSound("DropShipEngine.ogg"); break;
            }
            ++cue;
        }
        driver.periodicStreamUpdate();
        SDL_Delay(16);
    }
    check(driver.updateMusic("mus_intro.ogg", 1.0f), "the music played through");
}

} // end namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <sfx path> [--wav out.wav] [--play]\n", argv[0]);
        return 2;
    }
    std::string path = argv[1];
    if (path[path.size() - 1] != '/')
        path += '/';
    bool playOnDevice = false;
    ma_encoder wav;
    for (int i = 2; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--play") == 0)
            playOnDevice = true;
        else if (std::strcmp(argv[i], "--wav") == 0 && i + 1 < argc)
        {
            ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, RATE);
            if (ma_encoder_init_file(argv[++i], &config, &wav) == MA_SUCCESS)
                g_wav = &wav;
        }
    }

    ox::io::IFileSystem* fileSystem = daisy::io::createFileSystem();
    std::vector<std::string> sounds = listSounds(fileSystem, path.c_str());
    std::printf("sound path %s, %u .ogg files listed\n", path.c_str(), (unsigned int)sounds.size());

    testEffects(fileSystem, path, sounds);
    testMusic(fileSystem, path);
    testDucking(fileSystem, path);
    if (g_wav)
        ma_encoder_uninit(g_wav);
    if (playOnDevice)
        play(fileSystem, path);

    fileSystem->drop();
    std::printf("\n%s (%d failed)\n", g_failures ? "FAILED" : "passed", g_failures);
    return g_failures ? 1 : 0;
}
