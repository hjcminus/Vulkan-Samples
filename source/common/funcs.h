/******************************************************************************
 helper
 *****************************************************************************/

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>

#if defined(_WIN32)
# define	PLATFORM_WINDOWS
# define	NOMINMAX
#elif defined(__linux__)
# define	PLATFORM_LINUX
#else
# error "unsupported platform."
#endif

#if defined(PLATFORM_WINDOWS)
# if defined(COMMON_MODULE)
#  define COMMON_API __declspec(dllexport)
# else
#  define COMMON_API __declspec(dllimport)
# endif
#endif

#if defined(PLATFORM_LINUX)
# define COMMON_API
# define MAX_PATH	256
#endif


#define	TEMP_ALLOC(sz)		malloc(sz)
#define TEMP_FREE(ptr)		free(ptr)

COMMON_API void		Common_Init();

COMMON_API const wchar_t * GetDataFolder();

COMMON_API FILE *	File_Open(const char* filename, const char* mod);
COMMON_API bool		File_LoadBinary32(const char* filename, void*& data, int32_t& len);
COMMON_API void		File_FreeBinary(void* data);
COMMON_API bool		File_LoadText(const char* filename, char*& data);
COMMON_API void		File_FreeText(char* data);

// string
COMMON_API int		Str_UTF8ToUTF16(const char* utf8, char16_t* utf16, int utf16_chars);
COMMON_API int		Str_UTF8ToUTF32(const char* utf8, char32_t* utf32, int utf32_chars);
COMMON_API int		Str_UTF16ToUTF8(const char16_t* utf16, char* utf8, int utf8_bytes);
COMMON_API int		Str_UTF32ToUTF8(const char32_t* utf32, char* utf8, int utf8_bytes);
