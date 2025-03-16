/******************************************************************************
 helper
 *****************************************************************************/

#pragma once

/*
================================================================================
types
================================================================================
*/
using byte_t = uint8_t;
using word_t = uint16_t;
using dword_t = uint32_t;

#define	TEMP_ALLOC(sz)		malloc(sz)
#define TEMP_FREE(ptr)		free(ptr)

COMMON_API void			Common_Init();

COMMON_API const char * GetDataFolder();

#define GET_FIELD_OFFSET(s, f) (uint32_t)(&(((s*)0)->f))

/*
================================================================================
file
================================================================================
*/

COMMON_API FILE *	File_Open(const char* filename, const char* mod);
COMMON_API bool		File_LoadBinary32(const char* filename, void*& data, int32_t& len);
COMMON_API void		File_FreeBinary(void* data);
COMMON_API bool		File_LoadText(const char* filename, char*& data);
COMMON_API void		File_FreeText(char* data);

/*
================================================================================
string
================================================================================
*/
COMMON_API int		Str_ICmp(const char* s1, const char* s2);
COMMON_API void		Str_VSPrintf(char* dst, int dst_cap, const char* fmt, va_list argptr);
COMMON_API void		Str_SPrintf(char* dst, int dst_cap, const char* fmt, ...);
COMMON_API int		Str_UTF8ToUTF16(const char* utf8, char16_t* utf16, int utf16_chars);
COMMON_API int		Str_UTF8ToUTF32(const char* utf8, char32_t* utf32, int utf32_chars);
COMMON_API int		Str_UTF16ToUTF8(const char16_t* utf16, char* utf8, int utf8_bytes);
COMMON_API int		Str_UTF32ToUTF8(const char32_t* utf32, char* utf8, int utf8_bytes);

/*
================================================================================
color
================================================================================
*/
struct rgb_s {
	double			r_;	// [0-1]
	double			g_;	// [0-1]
	double			b_;	// [0-1]
};

struct hsv_s {
	double			h_;	// Hue: [0-360]
	double			s_;	// Saturation: [0-1]
	double			v_;	// Value: [0-1]
};

COMMON_API void		RGB2HSV(const rgb_s & in, hsv_s & out);
COMMON_API void		HSV2RGB(const hsv_s & in, rgb_s & out);

// t: [0-1], 0: start, 1 stop
COMMON_API void		RGBInterpolate(const rgb_s & start, const rgb_s & stop, double t, rgb_s& out);

/*
================================================================================
image
================================================================================
*/
struct image_s {
	int				width_;
	int				height_;
	byte_t*			pixels_;	// rgba
};

COMMON_API bool		Img_Load(const char * filename, image_s & image);
COMMON_API void		Img_Free(image_s & image);

/*
================================================================================
entrance
================================================================================
*/
#define DEMO_MAIN(DEMO_CLASS)		\
int main(int argc, char** argv) {	\
	Common_Init();					\
	DEMO_CLASS demo;				\
	if (!demo.Init()) {				\
		demo.Shutdown();			\
		return 1;					\
	}								\
	demo.MainLoop();				\
	demo.Shutdown();				\
	return 0;						\
}
