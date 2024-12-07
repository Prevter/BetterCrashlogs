#pragma once

// disable unused features
#include <wx/setup.h>
#undef wxUSE_LOG
#define wxUSE_LOG 0
#undef wxUSE_SOCKETS
#define wxUSE_SOCKETS 0
#undef wxUSE_WEBREQUEST
#define wxUSE_WEBREQUEST 0
#undef wxUSE_PROTOCOL
#define wxUSE_PROTOCOL 0
#undef wxUSE_URL
#define wxUSE_URL 0
#undef wxUSE_FS_INET
#define wxUSE_FS_INET 0

// include wxWidgets
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif