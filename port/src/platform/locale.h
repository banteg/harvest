// The C library's character encoding for the game's narrow strings.

#ifndef HARVEST_PORT_PLATFORM_LOCALE_H
#define HARVEST_PORT_PLATFORM_LOCALE_H

namespace port {

//! Makes the C library's character type locale UTF-8, the encoding of every narrow string the game
//! gets from SDL (paths, the command line, the clipboard, text input). The C locale breaks text in
//! three places:
//! - ansiToWide and wideToAnsi (mbstowcs, wcstombs): glibc's C locale stops at the first non-ASCII
//!   byte, Windows' maps each byte to one character, so pasted UTF-8 came out garbled;
//! - wide printf: macOS's C locale fails on characters past U+00FF, so every localized text of the
//!   Korean language file (and any text with a Korean player name) came out empty;
//! - Windows' narrow file functions (fopen, mkdir, chdir, getcwd) take ANSI code page names unless
//!   the locale is UTF-8, so a user or data folder with a non-ASCII name was unreachable.
//! The original's Linux build ran in the user's locale (gtk_init set it), which is that, or the C
//! locale's behaviour when that is not UTF-8. Only the character type changes: numbers keep the C
//! locale's decimal point.
void useUtf8Locale();

} // end namespace port

#endif
