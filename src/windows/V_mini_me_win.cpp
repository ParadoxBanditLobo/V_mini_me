#include "Win32ApiLite.hpp"

// The Win32 implementation is kept in one translation unit for a tiny,
// freestanding build. Its sections live in .inc files so each subsystem stays
// easy to navigate without introducing runtime or linker overhead.
#include "Win32Core.inc"
#include "Win32Config.inc"
#include "Win32Avatar.inc"
#include "Win32AudioWindow.inc"
#include "Win32Runtime.inc"
