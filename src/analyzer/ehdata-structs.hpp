// https://github.com/geode-sdk/geode/blob/main/loader/src/platform/windows/ehdata_structs.hpp
#pragma once

// _ThrowInfo and all of those other structs are hardcoded into MSVC (the compiler itself, unavailable in any header),
// but don't exist in other compilers like Clang, causing <ehdata.h> to not compile.
//
// We define them manually in order to be able to use them.
// sources:
// https://www.geoffchappell.com/studies/msvc/language/predefined/index.htm
// https://github.com/gnustep/libobjc2/blob/377a81d23778400b5306ee490451ed68b6e8db81/eh_win32_msvc.cc

struct _MSVC_PMD {
    int mdisp;
    int pdisp;
    int vdisp;
};

// silence the warning C4200: nonstandard extension used: zero-sized array in struct/union
#pragma warning (disable:4200)
struct _MSVC_TypeDescriptor {
    const void *pVFTable;
    void *spare;
    char name[0];
};
#pragma warning (default:4200)

struct _MSVC_CatchableType {
    unsigned int properties;
    unsigned long pType;
    _MSVC_PMD thisDisplacement;
    int sizeOrOffset;
    unsigned long copyFunction;
};

#pragma warning (disable:4200)
struct _MSVC_CatchableTypeArray {
    int nCatchableTypes;
    unsigned long arrayOfCatchableTypes[0];
};
#pragma warning (default:4200)

struct _MSVC_ThrowInfo {
    unsigned int attributes;
    unsigned long pmfnUnwind;
    unsigned long pfnForwardCompat;
    unsigned long pCatchableTypeArray;
};

#if defined(__clang__)
# define _ThrowInfo _MSVC_ThrowInfo
#endif

#include <ehdata.h> // for EH_EXCEPTION_NUMBER