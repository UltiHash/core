#ifndef LS_API_DEF_H
#define LS_API_DEF_H

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _WIN32

#if defined( LS_DLL_EXPORTS )
#define LS_API __declspec(dllexport)
#elif defined( LS_STATIC )
#define LS_API
#else
#define LS_API __declspec(dllimport)
#endif

#else // Linux, MacOS X and others

#if !defined( LS_API )
/// Macros that expands to dllexport, dllimport or nothing on non Windows platforms or in case of static library
#define LS_API
#endif

#endif // Platform check

#endif // LS_API_DEF_H


