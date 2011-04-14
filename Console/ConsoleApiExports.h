// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Console_ConsoleApiExposts_h
#define incl_Console_ConsoleApiExposts_h

#if defined (_WINDOWS)
#if defined(CONSOLE_API_EXPORTS)
#define CONSOLE_API __declspec(dllexport)
#else
#define CONSOLE_API __declspec(dllimport)
#endif
#else
#define CONSOLE_API
#endif

#endif
