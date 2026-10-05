// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source. The file name
// is ours; Mac has the table as the internal-linkage ox::input::KEY_NAMES.

#ifndef OX_INPUT_KEYNAMES_H
#define OX_INPUT_KEYNAMES_H

namespace ox {
namespace input {

//! Display names of the key codes, indexed by ox::EKEY_CODE.
static const wchar_t* const KEY_NAMES[] =
{
    L"", // 0x00
    L"Left mouse button", // 0x01
    L"Right mouse button", // 0x02
    L"Control-break", // 0x03
    L"Middle mouse button", // 0x04
    L"X1 mouse button", // 0x05
    L"X2 mouse button", // 0x06
    L"", // 0x07
    L"BACKSPACE key", // 0x08
    L"TAB key", // 0x09
    L"", // 0x0A
    L"", // 0x0B
    L"CLEAR key", // 0x0C
    L"ENTER key", // 0x0D
    L"", // 0x0E
    L"", // 0x0F
    L"SHIFT key", // 0x10
    L"CTRL key", // 0x11
    L"ALT key", // 0x12
    L"PAUSE key", // 0x13
    L"CAPS LOCK key", // 0x14
    L"IME Kana/Hangul mode", // 0x15
    L"", // 0x16
    L"IME Junja mode", // 0x17
    L"IME final mode", // 0x18
    L"IME Hanja/Kanji mode", // 0x19
    L"", // 0x1A
    L"ESC key", // 0x1B
    L"IME convert", // 0x1C
    L"IME nonconvert", // 0x1D
    L"IME accept", // 0x1E
    L"IME mode change request", // 0x1F
    L"SPACEBAR", // 0x20
    L"PAGE UP key", // 0x21
    L"PAGE DOWN key", // 0x22
    L"END key", // 0x23
    L"HOME key", // 0x24
    L"LEFT ARROW key", // 0x25
    L"UP ARROW key", // 0x26
    L"RIGHT ARROW key", // 0x27
    L"DOWN ARROW key", // 0x28
    L"SELECT key", // 0x29
    L"PRINT key", // 0x2A
    L"EXECUTE key", // 0x2B
    L"PRINT SCREEN key", // 0x2C
    L"INS key", // 0x2D
    L"DEL key", // 0x2E
    L"HELP key", // 0x2F
    L"0 key", // 0x30
    L"1 key", // 0x31
    L"2 key", // 0x32
    L"3 key", // 0x33
    L"4 key", // 0x34
    L"5 key", // 0x35
    L"6 key", // 0x36
    L"7 key", // 0x37
    L"8 key", // 0x38
    L"9 key", // 0x39
    L"", // 0x3A
    L"", // 0x3B
    L"", // 0x3C
    L"", // 0x3D
    L"", // 0x3E
    L"", // 0x3F
    L"", // 0x40
    L"A key", // 0x41
    L"B key", // 0x42
    L"C key", // 0x43
    L"D key", // 0x44
    L"E key", // 0x45
    L"F key", // 0x46
    L"G key", // 0x47
    L"H key", // 0x48
    L"I key", // 0x49
    L"J key", // 0x4A
    L"K key", // 0x4B
    L"L key", // 0x4C
    L"M key", // 0x4D
    L"N key", // 0x4E
    L"O key", // 0x4F
    L"P key", // 0x50
    L"Q key", // 0x51
    L"R key", // 0x52
    L"S key", // 0x53
    L"T key", // 0x54
    L"U key", // 0x55
    L"V key", // 0x56
    L"W key", // 0x57
    L"X key", // 0x58
    L"Y key", // 0x59
    L"Z key", // 0x5A
    L"Left Windows key", // 0x5B
    L"Right Windows key", // 0x5C
    L"Applications key", // 0x5D
    L"", // 0x5E
    L"Computer Sleep key", // 0x5F
    L"Numeric keypad 0 key", // 0x60
    L"Numeric keypad 1 key", // 0x61
    L"Numeric keypad 2 key", // 0x62
    L"Numeric keypad 3 key", // 0x63
    L"Numeric keypad 4 key", // 0x64
    L"Numeric keypad 5 key", // 0x65
    L"Numeric keypad 6 key", // 0x66
    L"Numeric keypad 7 key", // 0x67
    L"Numeric keypad 8 key", // 0x68
    L"Numeric keypad 9 key", // 0x69
    L"Multiply key", // 0x6A
    L"Add key", // 0x6B
    L"Separator key", // 0x6C
    L"Subtract key", // 0x6D
    L"Decimal key", // 0x6E
    L"Divide key", // 0x6F
    L"F1 key", // 0x70
    L"F2 key", // 0x71
    L"F3 key", // 0x72
    L"F4 key", // 0x73
    L"F5 key", // 0x74
    L"F6 key", // 0x75
    L"F7 key", // 0x76
    L"F8 key", // 0x77
    L"F9 key", // 0x78
    L"F10 key", // 0x79
    L"F11 key", // 0x7A
    L"F12 key", // 0x7B
    L"F13 key", // 0x7C
    L"F14 key", // 0x7D
    L"F15 key", // 0x7E
    L"F16 key", // 0x7F
    L"F17 key", // 0x80
    L"F18 key", // 0x81
    L"F19 key", // 0x82
    L"F20 key", // 0x83
    L"F21 key", // 0x84
    L"F22 key", // 0x85
    L"F23 key", // 0x86
    L"F24 key", // 0x87
    L"", // 0x88
    L"", // 0x89
    L"", // 0x8A
    L"", // 0x8B
    L"", // 0x8C
    L"", // 0x8D
    L"", // 0x8E
    L"", // 0x8F
    L"NUM LOCK key", // 0x90
    L"SCROLL LOCK key", // 0x91
    L"", // 0x92
    L"", // 0x93
    L"", // 0x94
    L"", // 0x95
    L"", // 0x96
    L"", // 0x97
    L"", // 0x98
    L"", // 0x99
    L"", // 0x9A
    L"", // 0x9B
    L"", // 0x9C
    L"", // 0x9D
    L"", // 0x9E
    L"", // 0x9F
    L"Left SHIFT key", // 0xA0
    L"Right SHIFT key", // 0xA1
    L"Left CONTROL key", // 0xA2
    L"Right CONTROL key", // 0xA3
    L"Left MENU key", // 0xA4
    L"Right MENU key", // 0xA5
    L"", // 0xA6
    L"", // 0xA7
    L"", // 0xA8
    L"", // 0xA9
    L"", // 0xAA
    L"", // 0xAB
    L"", // 0xAC
    L"", // 0xAD
    L"", // 0xAE
    L"", // 0xAF
    L"", // 0xB0
    L"", // 0xB1
    L"", // 0xB2
    L"", // 0xB3
    L"", // 0xB4
    L"", // 0xB5
    L"", // 0xB6
    L"", // 0xB7
    L"", // 0xB8
    L"", // 0xB9
    L"", // 0xBA
    L"Plus Key   (+)", // 0xBB
    L"Comma Key  (,)", // 0xBC
    L"Minus Key  (-)", // 0xBD
    L"Period Key (.)", // 0xBE
    L"", // 0xBF
    L"", // 0xC0
    L"", // 0xC1
    L"", // 0xC2
    L"", // 0xC3
    L"", // 0xC4
    L"", // 0xC5
    L"", // 0xC6
    L"", // 0xC7
    L"", // 0xC8
    L"", // 0xC9
    L"", // 0xCA
    L"", // 0xCB
    L"", // 0xCC
    L"", // 0xCD
    L"", // 0xCE
    L"", // 0xCF
    L"", // 0xD0
    L"", // 0xD1
    L"", // 0xD2
    L"", // 0xD3
    L"", // 0xD4
    L"", // 0xD5
    L"", // 0xD6
    L"", // 0xD7
    L"", // 0xD8
    L"", // 0xD9
    L"", // 0xDA
    L"", // 0xDB
    L"", // 0xDC
    L"", // 0xDD
    L"", // 0xDE
    L"", // 0xDF
    L"", // 0xE0
    L"", // 0xE1
    L"", // 0xE2
    L"", // 0xE3
    L"", // 0xE4
    L"", // 0xE5
    L"", // 0xE6
    L"", // 0xE7
    L"", // 0xE8
    L"", // 0xE9
    L"", // 0xEA
    L"", // 0xEB
    L"", // 0xEC
    L"", // 0xED
    L"", // 0xEE
    L"", // 0xEF
    L"", // 0xF0
    L"", // 0xF1
    L"", // 0xF2
    L"", // 0xF3
    L"", // 0xF4
    L"", // 0xF5
    L"Attn key", // 0xF6
    L"CrSel key", // 0xF7
    L"ExSel key", // 0xF8
    L"Erase EOF key", // 0xF9
    L"Play key", // 0xFA
    L"Zoom key", // 0xFB
    L"", // 0xFC
    L"PA1 key", // 0xFD
    L"Clear key", // 0xFE
    0, // 0xFF
};

} // end namespace input
} // end namespace ox

#endif
