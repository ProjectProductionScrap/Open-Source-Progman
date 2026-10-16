#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#pragma pack(push, 1)

typedef struct tagPROGMAN_ITEM {
    WORD x;             // X position inside group
    WORD y;             // Y position inside group
    WORD iIcon;         // Icon index
    WORD cbHeader;      // Header size
    WORD cbANDPlane;    // AND plane mask size (128 bytes)
    WORD cbXORPlane;    // XOR plane mask size (512 bytes)
    WORD pHeader;       // Offset to icon header
    WORD pANDPlane;     // Offset to AND mask
    WORD pXORPlane;     // Offset to XOR mask
    WORD pName;         // Offset to Item Title string
    WORD pCommand;      // Offset to Executable Command Line string
    WORD pIconPath;     // Offset to Icon File Path string
} PROGMAN_ITEM, * PPROGMAN_ITEM;

typedef struct tagPROGMAN_GROUP {
    char  magic[4];       // "PMCC" Magic Bytes
    WORD  wChecksum;     // 16-bit word-additive checksum
    WORD  cbGroup;       // Total file size
    WORD  nCmdShow;      // Window show state
    SHORT rcLeft;        // Window coordinates
    SHORT rcTop;         // Window coordinates
    SHORT rcRight;       // Window coordinates
    SHORT rcBottom;      // Window coordinates
    SHORT ptMinX;        // Minimized coordinates
    SHORT ptMinY;        // Minimized coordinates
    WORD  pName;         // Offset to Group Title string
    WORD  wLogPixelsX;   // DPI X
    WORD  wLogPixelsY;   // DPI Y
    WORD  wBitsPerPixel; // BPP
    WORD  wPlanes;       // Color planes
    WORD  cItems;        // Item count
    WORD  rgiItems[1];   // Array of item offsets
} PROGMAN_GROUP, * PPROGMAN_GROUP;

#pragma pack(pop)