// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500     // The DPIO2 driver was written for Windows 2000
#endif							// Keep this at 0x500

#include <stdio.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>

#include "Windows.h"

#pragma warning (disable : 4192)
// be sure to get the latest release of the following
#include "CIUsbLib.h"
#import "_CIUsbLib.tlb" no_namespace
using namespace std;

#define NUM_ACTUATORS	USB_NUM_ACTUATORS_MULTI
#define USB_DEVNAME		"Multi"

