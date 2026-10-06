// The audio backend, mirroring COpenALDriver call for call on the port's mixer (audio/Mixer.h stands
// in for the OpenAL sources and listener). What the original did, and what the port keeps:
//
// - Effects: "<sound effect path><name>", read through the game's file system. ".ogg" names are
//   decoded whole as Ogg Vorbis, ".wav" names as WAV; any other extension fails to load. A loaded
//   effect stays until the driver is destroyed. The original sized its decode buffer as nominal
//   bitrate times duration, which cuts off the end of high-rate or stereo files; the port decodes
//   the whole file (no shipped file is affected: all are mono, and the buffer fits).
// - A pool of 32 sources, searched round-robin, plays one-shot and looping effects; a source playing
//   a sound of lower priority than the current one is stopped and taken. With no free source the
//   sound is dropped.
// - Plain effects sit listener-relative at (2 * pan, 0, 0.1) with rolloff 1 and no cone; looping
//   effects use the same position with the global rolloff and the 30/75 degree cone facing -z;
//   oriented and tracked sounds are absolute, with the global rolloff and the cone.
// - Gain is the volume passed in (the volume settings are not applied here, as in the original);
//   pitch is the pitch times the global pitch modifier.
// - Music and voice lines stream from Ogg Vorbis: decoded on the game thread in pieces of 4096 bytes
//   of 16-bit audio into a queue as long as the original's ten buffers, refilled every frame by
//   periodicStreamUpdate. Looping seeks the stream back to the start; a non-looping stream is
//   released once it has played out. The stream source is listener-relative at the origin with
//   rolloff 0.
// - deviceDampenAllSounds does nothing, as in the original: a voice line only ducks the effects
//   started while it plays (CAudioDriver scales their volume).

#include "audio/CMiniaudioDriver.h"
#include <algorithm>
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "platform/Seams.h"

namespace port {
namespace audio {

namespace {

//! Number of buffers the original queued on a stream, and the size of each in bytes of 16-bit PCM.
const int STREAM_BUFFER_COUNT = 10;
const int STREAM_BUFFER_SIZE = 4096;

//! Reads a whole file through the game's file system (disk or archive).
bool readFile(ox::io::IFileSystem* fileSystem, const char* path, std::vector<char>& data)
{
    ox::io::IReadFile* file = fileSystem->createAndOpenFile(path);
    if (!file)
        return false;
    int size = file->getSize();
    data.resize(size > 0 ? size : 0);
    int read = size > 0 ? file->read(&data[0], size) : 0;
    file->drop();
    return size > 0 && read == size;
}

bool initDecoder(const std::vector<char>& data, ma_encoding_format encoding, ma_decoder& decoder)
{
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    config.encodingFormat = encoding;
    return ma_decoder_init_memory(&data[0], data.size(), &config, &decoder) == MA_SUCCESS;
}

//! Decodes a whole file into a buffer.
bool decodeWhole(const std::vector<char>& data, ma_encoding_format encoding, Buffer& buffer)
{
    ma_decoder decoder;
    if (!initDecoder(data, encoding, decoder))
        return false;
    ma_decoder_get_data_format(&decoder, 0, &buffer.Channels, &buffer.Rate, 0, 0);

    const ma_uint64 chunk = 16384;
    buffer.Frames = 0;
    for (;;)
    {
        buffer.Samples.resize((size_t)((buffer.Frames + chunk) * buffer.Channels));
        ma_uint64 read = 0;
        ma_decoder_read_pcm_frames(&decoder, &buffer.Samples[(size_t)(buffer.Frames * buffer.Channels)], chunk, &read);
        buffer.Frames += read;
        if (read == 0)
            break;
    }
    buffer.Samples.resize((size_t)(buffer.Frames * buffer.Channels));
    ma_decoder_uninit(&decoder);
    return buffer.Frames > 0;
}

//! A loaded sound effect: its decoded data, and the pool source that loops it.
class CSoundInfo : public daisy::audio::CSoundInfoStub
{
public:
    CSoundInfo() : LoopSource(0) {}

    Buffer Data;
    //! The pool source looping this sound, 0 when it is not looping.
    Source* LoopSource;
};

//! A tracked sound owns its own source, outside the pool.
class CTrackedSoundInfo : public daisy::audio::CTrackedSoundInfoStub
{
public:
    CTrackedSoundInfo(daisy::audio::CSoundInfoStub* sound, float pitch)
        : daisy::audio::CTrackedSoundInfoStub(sound), Pitch(pitch)
    {
    }

    Source Src;
    //! The requested pitch, before the global pitch modifier.
    float Pitch;
};

//! A music stream or voice line, decoded piece by piece into its source's queue. The file is read
//! when it first plays and freed by release().
class CMusicInfo : public daisy::audio::CMusicInfoStub
{
public:
    CMusicInfo(Mixer& mixer, ox::io::IFileSystem* fileSystem, const char* fileName)
        : Mix(mixer), FileSystem(fileSystem), FileName(fileName), IsOpen(false), Loop(false), PieceFrames(0)
    {
    }

    ~CMusicInfo()
    {
        release();
    }

    //! Queues new pieces; returns false when the stream ended.
    bool update(float pitch);
    void open();
    bool startPlayback(bool loop);
    void release();
    bool isPlaying();
    //! Decodes up to one piece into the queue; false at the end of a non-looping stream.
    bool streamNextPiece();
    //! Queues pieces while there is room; false once the stream ended.
    bool queuePieces();

    Mixer& Mix;
    ox::io::IFileSystem* FileSystem;
    ox::core::CString<char> FileName;
    bool IsOpen;
    bool Loop;
    std::vector<char> File;
    ma_decoder Decoder;
    StreamQueue Queue;
    ma_uint64 PieceFrames;
    Source Src;
};

void CMusicInfo::open()
{
    if (IsOpen || !Mix.isAvailable())
        return;
    if (!readFile(FileSystem, FileName.c_str(), File) || !initDecoder(File, ma_encoding_format_vorbis, Decoder))
    {
        std::vector<char>().swap(File);
        return;
    }

    ma_decoder_get_data_format(&Decoder, 0, &Queue.Channels, &Queue.Rate, 0, 0);
    PieceFrames = STREAM_BUFFER_SIZE / (2 * Queue.Channels);
    Queue.Capacity = PieceFrames * STREAM_BUFFER_COUNT;
    Queue.Samples.assign((size_t)(Queue.Capacity * Queue.Channels), 0.0f);
    Queue.Written = 0;
    Queue.Ended = false;

    Src = Source();
    Src.Queue = &Queue;
    Src.Relative = true;
    Src.Rolloff = 0.0f;
    MixerLock lock(Mix);
    Mix.addSource(&Src);
    IsOpen = true;
}

bool CMusicInfo::startPlayback(bool loop)
{
    if (!IsOpen)
        return false;
    Loop = loop;
    if (!isPlaying())
    {
        {
            MixerLock lock(Mix);
            Queue.Written = 0;
            Queue.Ended = false;
            Src.Cursor = 0.0;
        }
        bool active = queuePieces();
        MixerLock lock(Mix);
        Queue.Ended = !active;
        Src.play();
    }
    return true;
}

bool CMusicInfo::update(float pitch)
{
    if (!IsOpen)
        return false;

    bool active = !Queue.Ended && queuePieces();
    bool drained;
    {
        MixerLock lock(Mix);
        if (!active)
            Queue.Ended = true;
        drained = !active && !Src.Playing;
        Src.setPitch(pitch);
    }
    // Released once everything queued has played.
    if (drained)
        release();
    return active;
}

bool CMusicInfo::queuePieces()
{
    for (;;)
    {
        ma_uint64 queued;
        {
            MixerLock lock(Mix);
            queued = Queue.Written - (ma_uint64)Src.Cursor;
        }
        if (Queue.Capacity - queued < PieceFrames)
            return true;
        if (!streamNextPiece())
            return false;
    }
}

bool CMusicInfo::streamNextPiece()
{
    // The frames after Written are not read by the mixer until Written moves past them, so the
    // decoder writes into the queue without the lock.
    ma_uint64 frames = 0;
    bool loop = Loop;
    while (frames < PieceFrames)
    {
        ma_uint64 start = (Queue.Written + frames) % Queue.Capacity;
        ma_uint64 count = std::min(PieceFrames - frames, Queue.Capacity - start);
        ma_uint64 read = 0;
        ma_result result =
            ma_decoder_read_pcm_frames(&Decoder, &Queue.Samples[(size_t)(start * Queue.Channels)], count, &read);
        frames += read;
        if (read > 0)
            continue;
        if (result != MA_SUCCESS && result != MA_AT_END)
            return false;
        if (frames == 0 && loop)
        {
            // Looping is done by the stream: back to the start, once per piece.
            ma_decoder_seek_to_pcm_frame(&Decoder, 0);
            loop = false;
        }
        else
            break;
    }
    if (frames == 0)
        return false;

    MixerLock lock(Mix);
    Queue.Written += frames;
    return true;
}

void CMusicInfo::release()
{
    if (!IsOpen)
        return;
    {
        MixerLock lock(Mix);
        Src.Playing = false;
        Mix.removeSource(&Src);
    }
    ma_decoder_uninit(&Decoder);
    std::vector<char>().swap(File);
    std::vector<float>().swap(Queue.Samples);
    IsOpen = false;
}

bool CMusicInfo::isPlaying()
{
    if (!IsOpen)
        return false;
    MixerLock lock(Mix);
    return Src.Playing;
}

} // end namespace

CMiniaudioDriver::CMiniaudioDriver(ox::io::IFileSystem* fileSystem, unsigned int offlineRate)
    : FileSystem(fileSystem), NextSource(0), GlobalPitchModifier(1.0f)
{
    FileSystem->grab();

    ConeInnerAngle = 30;
    ConeOuterAngle = 75;
    ConeDirection.set(0.0f, 0.0f, -1.0f);

    for (int i = 0; i < NUM_SOURCES; ++i)
    {
        SourcePitch[i] = 1.0f;
        SourcePriority[i] = 1.0f;
        Mix.addSource(&Sources[i]);
    }

    if (offlineRate)
        Mix.openOffline(offlineRate);
    else
        Mix.openDevice();
    if (Mix.isAvailable())
        set2dOrientation();
}

CMiniaudioDriver::~CMiniaudioDriver()
{
    Mix.close();
    stopAllSounds();
    // The base destructor deletes the voice after the mixer is gone; release it while it exists.
    stopVoice();
    FileSystem->drop();
}

void CMiniaudioDriver::periodicStreamUpdate()
{
    float pitch = UseGlobalPitchModifierForMusic ? GlobalPitchModifier : 1.0f;

    for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
        static_cast<CMusicInfo*>(it->second)->update(pitch);

    if (Voice)
        static_cast<CMusicInfo*>(Voice)->update(pitch);
}

void CMiniaudioDriver::setGlobalPitchModifier(float pitch)
{
    GlobalPitchModifier = pitch;
    MixerLock lock(Mix);

    for (int i = 0; i < NUM_SOURCES; ++i)
    {
        if (Sources[i].Playing)
            Sources[i].setPitch(pitch * SourcePitch[i]);
    }

    for (TrackedSoundMap::iterator it = TrackedSounds.begin(); it != TrackedSounds.end(); ++it)
    {
        CTrackedSoundInfo* tracked = static_cast<CTrackedSoundInfo*>(it->second);
        tracked->Src.setPitch(pitch * tracked->Pitch);
    }

    if (UseGlobalPitchModifierForMusic)
    {
        for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
            static_cast<CMusicInfo*>(it->second)->Src.setPitch(pitch);
    }
}

void CMiniaudioDriver::setOpenAlDistanceModel(int model)
{
    MixerLock lock(Mix);
    Mix.setDistanceModel(Mixer::INVERSE_DISTANCE - 1 + model);
}

ox::core::CString<char> CMiniaudioDriver::getPath(const char* name) const
{
    ox::core::CString<char> path = SoundEffectPath;
    path.append(ox::core::CString<char>(name));
    return path;
}

daisy::audio::CSoundInfoStub* CMiniaudioDriver::deviceLoadSound(const char* name)
{
    ox::core::CString<char> path = getPath(name);
    ma_encoding_format encoding;
    if (path.endsWith(ox::core::CString<char>(".ogg")))
        encoding = ma_encoding_format_vorbis;
    else if (path.endsWith(ox::core::CString<char>(".wav")))
        encoding = ma_encoding_format_wav;
    else
        return 0;

    std::vector<char> file;
    CSoundInfo* sound = new CSoundInfo();
    if (!readFile(FileSystem, path.c_str(), file) || !decodeWhole(file, encoding, sound->Data))
    {
        delete sound;
        return 0;
    }
    return sound;
}

unsigned int CMiniaudioDriver::findFreeSoundSource()
{
    if (!Mix.isAvailable())
        return NUM_SOURCES;

    unsigned int start = NextSource;
    do
    {
        unsigned int i = NextSource;
        ++NextSource;
        if (NextSource >= NUM_SOURCES)
            NextSource = 0;

        if (SoundPriority > SourcePriority[i])
            Sources[i].Playing = false;

        if (!Sources[i].Playing)
        {
            SourcePriority[i] = SoundPriority;
            return i;
        }
    } while (NextSource != start);

    return NUM_SOURCES;
}

void CMiniaudioDriver::devicePlaySound(daisy::audio::CSoundInfoStub* sound, float volume, float pan, float pitch)
{
    MixerLock lock(Mix);
    unsigned int i = findFreeSoundSource();
    if (i >= NUM_SOURCES)
        return;

    SourcePitch[i] = pitch;
    Source& source = Sources[i];
    source.Data = &static_cast<CSoundInfo*>(sound)->Data;
    source.setPitch(pitch * GlobalPitchModifier);
    source.setGain(volume);
    source.Looping = false;
    source.Relative = true;
    source.Rolloff = 1.0f;
    source.Direction.set(0.0f, 0.0f, 0.0f);
    source.ConeInner = 360.0f;
    source.ConeOuter = 360.0f;
    // Panned left or right of the listener, slightly behind it.
    source.Position.set(pan + pan, 0.0f, 0.1f);
    source.play();
}

void CMiniaudioDriver::devicePlayOrientedSound(daisy::audio::CSoundInfoStub* sound, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    MixerLock lock(Mix);
    unsigned int i = findFreeSoundSource();
    if (i >= NUM_SOURCES)
        return;

    SourcePitch[i] = pitch;
    Source& source = Sources[i];
    source.Data = &static_cast<CSoundInfo*>(sound)->Data;
    source.setPitch(pitch * GlobalPitchModifier);
    source.setGain(volume);
    source.Looping = false;
    source.Relative = false;
    source.Rolloff = GlobalRollOffFactor;
    source.Position = position;
    source.Velocity = velocity;
    source.Direction = ConeDirection;
    source.ConeInner = (float)ConeInnerAngle;
    source.ConeOuter = (float)ConeOuterAngle;
    source.play();
}

void CMiniaudioDriver::deviceDampenAllSounds(float)
{
}

void CMiniaudioDriver::deviceLoopSound(daisy::audio::CSoundInfoStub* sound, float volume, float pan, float pitch,
    float fade)
{
    CSoundInfo* info = static_cast<CSoundInfo*>(sound);
    MixerLock lock(Mix);
    Source* source = info->LoopSource;
    if (!source)
    {
        unsigned int i = findFreeSoundSource();
        if (i >= NUM_SOURCES)
            return;
        info->LoopSource = &Sources[i];
        SourcePitch[i] = pitch;
        source = info->LoopSource;
    }

    source->setPitch(pitch * GlobalPitchModifier);
    source->setGain(volume);
    source->Looping = true;
    source->Relative = true;
    source->Rolloff = GlobalRollOffFactor;
    source->Position.set(pan + pan, 0.0f, 0.1f);
    source->Direction = ConeDirection;
    source->ConeInner = (float)ConeInnerAngle;
    source->ConeOuter = (float)ConeOuterAngle;

    // An already looping sound only has its parameters updated.
    if (!source->Playing)
    {
        source->Data = &info->Data;
        source->setOffset(fade);
        source->play();
    }
}

void CMiniaudioDriver::deviceStopLoopSound(daisy::audio::CSoundInfoStub* sound)
{
    CSoundInfo* info = static_cast<CSoundInfo*>(sound);
    if (info->LoopSource)
    {
        MixerLock lock(Mix);
        info->LoopSource->Playing = false;
        info->LoopSource = 0;
    }
}

void CMiniaudioDriver::deviceStopSound(daisy::audio::CSoundInfoStub* sound)
{
    // Only a looping effect is stopped; one-shots play out, as in the original.
    deviceStopLoopSound(sound);
}

daisy::audio::CMusicInfoStub* CMiniaudioDriver::deviceLoadMusic(const char* name)
{
    // Always succeeds: the file is opened when the stream first plays.
    return new CMusicInfo(Mix, FileSystem, getPath(name).c_str());
}

void CMiniaudioDriver::devicePlayMusic(daisy::audio::CMusicInfoStub* music, float volume, bool loop, bool)
{
    CMusicInfo* info = static_cast<CMusicInfo*>(music);
    info->open();
    if (info->startPlayback(loop))
    {
        MixerLock lock(Mix);
        info->Src.setGain(volume);
        // Looping is done by the stream, not the source.
        info->Src.Looping = false;
    }
}

void CMiniaudioDriver::deviceStopMusic(daisy::audio::CMusicInfoStub* music)
{
    static_cast<CMusicInfo*>(music)->release();
}

bool CMiniaudioDriver::deviceUpdateMusic(daisy::audio::CMusicInfoStub* music, float volume)
{
    CMusicInfo* info = static_cast<CMusicInfo*>(music);
    if (!info->isPlaying())
        return false;
    MixerLock lock(Mix);
    info->Src.setGain(volume);
    return true;
}

bool CMiniaudioDriver::deviceIsMusicPlaying(daisy::audio::CMusicInfoStub* music)
{
    return static_cast<CMusicInfo*>(music)->isPlaying();
}

void CMiniaudioDriver::deviceSetListenerPosition(const ox::core::CVector3d<float>& position)
{
    MixerLock lock(Mix);
    Mix.ListenerPosition = position;
}

void CMiniaudioDriver::deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity)
{
    MixerLock lock(Mix);
    Mix.ListenerVelocity = velocity;
}

void CMiniaudioDriver::deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
    const ox::core::CVector3d<float>& up)
{
    MixerLock lock(Mix);
    Mix.ListenerForward = forward;
    Mix.ListenerUp = up;
}

daisy::audio::CTrackedSoundInfoStub* CMiniaudioDriver::deviceStartTrackedSound(daisy::audio::CSoundInfoStub* sound,
    float volume, float pitch, const ox::core::CVector3d<float>& position,
    const ox::core::CVector3d<float>& velocity, float fade)
{
    if (!Mix.isAvailable())
        return 0;

    CTrackedSoundInfo* tracked = new CTrackedSoundInfo(sound, pitch);
    Source& source = tracked->Src;
    source.Data = &static_cast<CSoundInfo*>(sound)->Data;
    source.setPitch(pitch * GlobalPitchModifier);
    source.setGain(volume);
    source.Looping = true;
    source.Relative = false;
    source.Rolloff = GlobalRollOffFactor;
    source.Direction = ConeDirection;
    source.ConeInner = (float)ConeInnerAngle;
    source.ConeOuter = (float)ConeOuterAngle;
    source.Position = position;
    source.Velocity = velocity;
    source.setOffset(fade);
    source.play();

    MixerLock lock(Mix);
    Mix.addSource(&source);
    return tracked;
}

void CMiniaudioDriver::deviceStopTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound)
{
    MixerLock lock(Mix);
    Mix.removeSource(&static_cast<CTrackedSoundInfo*>(sound)->Src);
}

void CMiniaudioDriver::deviceUpdateTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound, float volume,
    float pitch, const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    CTrackedSoundInfo* tracked = static_cast<CTrackedSoundInfo*>(sound);
    MixerLock lock(Mix);
    tracked->Pitch = pitch;
    tracked->Src.setPitch(pitch * GlobalPitchModifier);
    tracked->Src.setGain(volume);
    tracked->Src.Position = position;
    tracked->Src.Velocity = velocity;
}

} // end namespace audio

ox::audio::IAudioDriver* createAudioDriver(ox::io::IFileSystem* fileSystem)
{
    return new audio::CMiniaudioDriver(fileSystem);
}

} // end namespace port
