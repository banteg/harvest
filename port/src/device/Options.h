// The port's command line, parsed by main.cpp before the game creates its device.

#ifndef PORT_DEVICE_OPTIONS_H
#define PORT_DEVICE_OPTIONS_H

namespace port {

struct SOptions
{
    SOptions() : DataDirectory(0), NullVideo(false), NoAudio(false), VSync(true) {}

    //! The directory holding harvestClientData/ ($GAME_RESOURCES$): --data, else $HARVEST_DATA,
    //! else 0 for the executable's directory.
    const char* DataDirectory;
    //! --null-video: daisy's null driver instead of the renderer; the window is only cleared.
    bool NullVideo;
    //! --no-audio: an audio driver that loads and plays nothing.
    bool NoAudio;
    //! --no-vsync turns it off. The original never asked for vsync.
    bool VSync;
};

extern SOptions g_options;

//! Reads argv into g_options. Returns false after printing the usage on --help or a bad argument.
bool parseOptions(int argc, char** argv);

} // end namespace port

#endif
