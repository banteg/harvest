#ifndef PORT_GAME_SAVEFILE_H
#define PORT_GAME_SAVEFILE_H

namespace harvest { namespace settings { struct SSavestateHeader; } }
namespace ox { namespace io { class CMemReadFile; class CMemWriteFile; class IReadFile; } }

namespace port {
// Read the encrypted payload following a validated v26–30 save header.
ox::io::CMemReadFile* readSavePayload(ox::io::IReadFile* file);
bool writeSaveFile(const char* path, const harvest::settings::SSavestateHeader& header,
    ox::io::CMemWriteFile& payload);
}
#endif
