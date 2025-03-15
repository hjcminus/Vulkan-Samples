/******************************************************************************
 helper
 *****************************************************************************/

#include "inc.h"

static char	g_data_folder[1024];

static wchar_t * FindDoubleDots(wchar_t* path) {
	wchar_t* pc = wcsstr(path, L"..\\");
	if (!pc) {
		pc = wcsstr(path, L"../");
	}

	return pc;
}

static wchar_t * RFindSlash(wchar_t* path) {
	wchar_t* pc = wcsrchr(path, L'\\');
	if (!pc) {
		pc = wcsrchr(path, L'/');
	}

	return pc;
}

static void Str_EraseDoubleDotsInPath(wchar_t* path) {
	wchar_t sl[2][MAX_PATH];

	wcscpy_s(sl[0], MAX_PATH, path);
	wcscpy_s(sl[1], MAX_PATH, path);

	int srcidx = 0;
	int dstidx = 1 - srcidx;

	wchar_t* pc = FindDoubleDots(sl[srcidx]);
	while (pc) {
		*pc = 0;

		wchar_t* pc1 = RFindSlash(sl[srcidx]);
		if (pc1) {
			*pc1 = 0;

			wchar_t* pc2 = RFindSlash(sl[srcidx]);
			if (pc2) {
				pc2[1] = 0;
				wcscpy_s(sl[dstidx], MAX_PATH, sl[srcidx]);
				wcscat_s(sl[dstidx], MAX_PATH, pc + 3);

				srcidx = 1 - srcidx;
				dstidx = 1 - srcidx;

				pc = FindDoubleDots(sl[srcidx]);
			}
			else {
				break;
			}

		}
		else {
			break;
		}
	}

	wcscpy_s(path, MAX_PATH, sl[srcidx]);
}

static bool IsDirectorySeparator(wchar_t c) {
	return c == L'\\' || c == L'/';
}

bool Str_ExtractFileDirSelf(wchar_t * path) {
	int l = (int)wcslen(path);
	for (int i = l - 1; i >= 0; --i) {
		if (IsDirectorySeparator(path[i])) {
			path[i] = 0;
			return true;
		}
	}
	path[0] = 0;	// return empty string
	return false;
}

void Common_Init() {
#if defined(PLATFORM_WINDOWS)
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), buffer, MAX_PATH);

	Str_ExtractFileDirSelf(buffer);
	wcscat_s(buffer, L"\\..\\..\\..\\data");

	Str_EraseDoubleDotsInPath(buffer);

	Str_UTF16ToUTF8((const char16_t*)buffer, g_data_folder, MAX_PATH);
#endif
}

COMMON_API const char * GetDataFolder() {
	return g_data_folder;
}

/*
================================================================================
file
================================================================================
*/

COMMON_API FILE* File_Open(const char* filename, const char* mod) {
#if defined(_MSC_VER)
	char16_t char16_filename[1024], char16_mod[64];
	Str_UTF8ToUTF16(filename, char16_filename, 1024);
	Str_UTF8ToUTF16(mod, char16_mod, 64);

	FILE* f = nullptr;
	errno_t e = _wfopen_s(&f, (const wchar_t*)char16_filename, (const wchar_t*)char16_mod);

	return f;
#endif

#if defined(__GNUC__)
	FILE* f = fopen(filename, mod);
	return f;
#endif
}

COMMON_API bool File_LoadBinary32(const char* filename, void*& data, int32_t& len) {
	data = nullptr;
	len = 0;

	FILE* f = File_Open(filename, "rb");

	if (!f) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = TEMP_ALLOC(sz);
	int read_size = (int)fread(data, 1, sz, f);
	fclose(f);

	if (read_size != sz) {
		TEMP_FREE(data);
		return false;
	}
	else {
		len = sz;
		return true;
	}
}

COMMON_API void File_FreeBinary(void* data) {
	if (data) {
		TEMP_FREE(data);
	}
}

COMMON_API bool File_LoadText(const char* filename, char*& data) {
	data = nullptr;

	FILE* f = File_Open(filename, "rb");

	if (!f) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = (char*)TEMP_ALLOC(sz + 1);
	int read_size = (int)fread(data, 1, sz, f);
	fclose(f);

	if (read_size != sz) {
		TEMP_FREE(data);
		return false;
	}
	else {
		data[sz] = 0;
		return true;
	}
}

COMMON_API void File_FreeText(char* data) {
	if (data) {
		TEMP_FREE(data);
	}
}

/*
================================================================================
string
================================================================================
*/
COMMON_API int Str_ICmp(const char* s1, const char* s2) {
#if defined(_MSC_VER)
	return _stricmp(s1, s2);
#endif

#if defined(__GNUC__)
	return strcasecmp(s1, s2);
#endif
}

COMMON_API void Str_VSPrintf(char* dst, int dst_cap, const char* fmt, va_list argptr) {
#if defined(_MSC_VER)
	vsnprintf_s(dst, (size_t)dst_cap, _TRUNCATE, fmt, argptr);
#endif

#if defined(__GNUC__)
	vsnprintf(dst, dst_cap, fmt, argptr);
#endif
}

COMMON_API void Str_SPrintf(char* dst, int dst_cap, const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	Str_VSPrintf(dst, dst_cap, fmt, argptr);
	va_end(argptr);
}

COMMON_API int Str_UTF8ToUTF16(const char* utf8, char16_t* utf16, int utf16_chars) {
#if defined(PLATFORM_WINDOWS)
	if (!utf8) {
		if (utf16 && utf16_chars > 0) { // treat as null string
			utf16[0] = 0;
		}
		return 0;
	}

	int utf8_bytes = (int)strlen(utf8);
	if (utf8_bytes < 1) {
		if (utf16 && utf16_chars > 0) {
			utf16[0] = 0;
		}
		return 0;
	}

	int need_chars = MultiByteToWideChar(CP_UTF8, 0 /* do not using MB_PRECOMPOSED */, utf8, utf8_bytes, nullptr, 0); //to UTF-16, returns the number of characters
	if (!need_chars) {
		// SYS_SYSLASTERROR(false);
		return 0;
	}

	if (!utf16) {
		return need_chars + 1;
	}

	assert(utf16);
	assert(utf16_chars > 0);

	if (utf16_chars < need_chars + 1) {
		// SYS_ERROR("utf16 buffer too small\n");
		utf16[0] = 0;
		return 0;
	}

	int translen = MultiByteToWideChar(CP_UTF8, 0 /* do not using MB_PRECOMPOSED */, utf8, utf8_bytes, (LPWSTR)utf16, utf16_chars);
	utf16[translen] = 0;

	return translen;
#elif defined(PLATFORM_LINUX)
	std::u16string u16_conv = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(utf8);
	if (u16_conv.size() >= utf16_chars) {
		// SYS_ERROR("utf16 buffer too small\n");
		utf16[0] = 0;
		return 0;
	}
	else {
		memcpy(utf16, u16_conv.c_str(), sizeof(char16_t) * u16_conv.size());
		return (int)u16_conv.size();
	}
#else
# error "unsupported platform"
#endif
}

#if defined(PLATFORM_WINDOWS)

// https://codereview.stackexchange.com/questions/197548/convert-utf8-string-to-utf32-string-in-c
static size_t utf8_need_utf32_chars(const uint8_t* utf8_str) {
	size_t num_chars = 0;
	const uint8_t* pc = utf8_str;

	while (*pc) {
		num_chars++;

		if ((*pc & 0b10000000) == 0) {
			// 1 byte code point, ASCII
			pc++;
		}
		else if ((*pc & 0b11100000) == 0b11000000) {
			// 2 byte code point
			if (!pc[1]) {
				break;	// error
			}

			pc += 2;
		}
		else if ((*pc & 0b11110000) == 0b11100000) {
			// 3 byte code point

			if (!pc[1] || !pc[2]) {
				break;	// error
			}

			pc += 3;
		}
		else {
			// 4 byte code point

			if (!pc[1] || !pc[2] || !pc[3]) {
				break;	// error
			}

			pc += 4;
		}
	}

	return num_chars;
}

union utf32_bytes_s {
	uint8_t		bytes_[4];
	char32_t	c_;
};

static size_t utf32_need_utf8_chars(const char32_t* utf32_str) {
	size_t num_chars = 0;
	const char32_t* pc = utf32_str;

	while (*pc) {
		utf32_bytes_s u;
		u.c_ = *pc;

		if (u.bytes_[2] & 0b00011100) {
			num_chars += 4;
		}
		else if (u.bytes_[1] & 0b11110000) {
			num_chars += 3;
		}
		else if (u.bytes_[1] & 0b00000111) {
			num_chars += 2;
		}
		else {
			num_chars++;
		}

		pc++;
	}

	return num_chars;
}

#endif

COMMON_API int Str_UTF8ToUTF32(const char* utf8, char32_t* utf32, int utf32_chars) {
#if defined(PLATFORM_WINDOWS)

	size_t need_chars = utf8_need_utf32_chars((const uint8_t*)utf8);
	if (utf32_chars <= need_chars) {
		return 0;	// buf overflow
	}

	const uint8_t* src_pc = (const uint8_t*)utf8;
	char32_t* dst_pc = utf32;
	int converted = 0;

	while (*src_pc) {
		if ((*src_pc & 0b10000000) == 0) {
			// 1 byte code point, ASCII
			*dst_pc = (*src_pc & 0b01111111);
			src_pc++;
			dst_pc++;
			converted++;
		}
		else if ((*src_pc & 0b11100000) == 0b11000000) {
			// 2 byte code point

			if (!src_pc[1]) {
				break;	// error
			}

			*dst_pc = (src_pc[0] & 0b00011111) << 6 | (src_pc[1] & 0b00111111);
			src_pc += 2;
			dst_pc++;
			converted++;
		}
		else if ((*src_pc & 0b11110000) == 0b11100000) {
			// 3 byte code point

			if (!src_pc[1] || !src_pc[2]) {
				break;	// error
			}

			*dst_pc = (src_pc[0] & 0b00001111) << 12 | (src_pc[1] & 0b00111111) << 6 | (src_pc[2] & 0b00111111);
			src_pc += 3;
			dst_pc++;
			converted++;
		}
		else {
			// 4 byte code point

			if (!src_pc[1] || !src_pc[2] || !src_pc[3]) {
				break;	// error
			}

			*dst_pc = (src_pc[0] & 0b00000111) << 18 | (src_pc[1] & 0b00111111) << 12 | (src_pc[2] & 0b00111111) << 6 | (src_pc[3] & 0b00111111);
			src_pc += 4;
			dst_pc++;
			converted++;
		}
	}

	*dst_pc = 0;

	return converted;

#elif defined(PLATFORM_LINUX)
	std::u32string u32_conv = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>{}.from_bytes(utf8);
	if (u32_conv.size() >= utf32_chars) {
		// SYS_ERROR("utf32 buffer too small\n");
		utf32[0] = 0;
		return 0;
	}
	else {
		memcpy(utf32, u32_conv.c_str(), sizeof(char32_t) * u32_conv.size());
		return (int)u32_conv.size();
	}
#else
# error "unsupported platform"
#endif
}

COMMON_API int Str_UTF16ToUTF8(const char16_t* utf16, char* utf8, int utf8_bytes) {
#if defined(PLATFORM_WINDOWS)
	if (!utf16) {
		if (utf8 && utf8_bytes > 0) { // treat as null string
			utf8[0] = 0;
		}
		return 0;
	}

	int utf16_chars = (int)wcslen((const wchar_t*)utf16);
	if (utf16_chars < 1) {
		if (utf8 && utf8_bytes > 0) {
			utf8[0] = 0;
		}
		return 0;
	}

	int need_bytes = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)utf16, utf16_chars, nullptr, 0, nullptr, nullptr); // return bytes count
	if (!utf8) {
		return need_bytes + 1;
	}

	assert(utf8);
	assert(utf8_bytes > 0);

	if (utf8_bytes < need_bytes + 1) {
		utf8[0] = 0;
		// SYS_ERROR("utf8 buffer too small\n");
		return 0;
	}

	int translen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)utf16, utf16_chars, utf8, utf8_bytes, nullptr, nullptr);
	utf8[translen] = 0;

	return translen;
#elif defined(PLATFORM_LINUX)
	std::string u8_conv = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(utf16);
	if (u8_conv.size() >= utf8_bytes) {
		// SYS_ERROR("utf8 buffer too small\n");
		utf8[0] = 0;
		return 0;
	}
	else {
		memcpy(utf8, u8_conv.c_str(), sizeof(char) * u8_conv.size());
		return (int)u8_conv.size();
	}
#else
# error "unsupported platform"
#endif
}


COMMON_API int Str_UTF32ToUTF8(const char32_t* utf32, char* utf8, int utf8_bytes) {
#if defined(PLATFORM_WINDOWS)

	size_t need_chars = utf32_need_utf8_chars(utf32);
	if (utf8_bytes <= need_chars) {
		return 0;	// buf overflow
	}

	const char32_t* src_pc = utf32;
	char* dst_pc = utf8;
	int converted = 0;

	while (*src_pc) {
		utf32_bytes_s u;
		u.c_ = *src_pc;

		if (u.bytes_[2] & 0b00011100) {
			dst_pc[0] = 0b11110000 | ((u.bytes_[2] & 0b00011100) >> 2);
			dst_pc[1] = 0b10000000 | ((u.bytes_[2] & 0b00000011) << 4) | ((u.bytes_[1] & 0b11110000) >> 4);
			dst_pc[2] = 0b10000000 | ((u.bytes_[1] & 0b00001111) << 2) | ((u.bytes_[0] & 0b11000000) >> 6);
			dst_pc[3] = 0b10000000 | (u.bytes_[0] & 0b00111111);

			dst_pc += 4;
			converted += 4;
		}
		else if (u.bytes_[1] & 0b11110000) {
			dst_pc[0] = 0b11100000 | ((u.bytes_[1] & 0b11110000) >> 4);
			dst_pc[1] = 0b10000000 | ((u.bytes_[1] & 0b00001111) << 2) | ((u.bytes_[0] & 0b11000000) >> 6);
			dst_pc[2] = 0b10000000 | (u.bytes_[0] & 0b00111111);

			dst_pc += 3;
			converted += 3;
		}
		else if (u.bytes_[1] & 0b00000111) {
			dst_pc[0] = 0b11000000 | ((u.bytes_[1] & 0b00000111) << 2) | ((u.bytes_[0] & 0b11000000) >> 6);
			dst_pc[1] = 0b10000000 | (u.bytes_[0] & 0b00111111);

			dst_pc += 2;
			converted += 2;
		}
		else {
			dst_pc[0] = 0b01111111 & u.bytes_[0];

			dst_pc++;
			converted++;
		}

		src_pc++;
	}

	*dst_pc = 0;

	return converted;

#elif defined(PLATFORM_LINUX)
	std::string u8_conv = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>{}.to_bytes(utf32);
	if (u8_conv.size() >= utf8_bytes) {
		// SYS_ERROR("utf8 buffer too small\n");
		utf8[0] = 0;
		return 0;
	}
	else {
		memcpy(utf8, u8_conv.c_str(), sizeof(char) * u8_conv.size());
		return (int)u8_conv.size();
	}
#else
# error "unsupported platform"
#endif
}

/*
================================================================================
color
================================================================================
*/

COMMON_API void	RGB2HSV(const rgb_s& in, hsv_s& out) {
	double min, max, delta;

	min = in.r_ < in.g_ ? in.r_ : in.g_;
	min = min < in.b_ ? min : in.b_;

	max = in.r_ > in.g_ ? in.r_ : in.g_;
	max = max > in.b_ ? max : in.b_;

	// v
	out.v_ = max;	

	// s
	delta = max - min;
	if (max > 0.0) { // NOTE: if max == 0, this divide would cause a crash
		out.s_ = (delta / max);
	}
	else {
		// if max is 0, then r = g = b = 0              
		// s = 0, v is undefined
		out.s_ = 0.0;
		out.h_ = 1e30f;
		return;
	}

	// h
	if (in.r_ >= max) {							// > is bogus, just keeps compilor happy
		out.h_ = (in.g_ - in.b_) / delta;		// between yellow & magenta
	}
	else if (in.g_ >= max) {
		out.h_ = 2.0 + (in.b_ - in.r_) / delta;	// between cyan & yellow
	}
	else {
		out.h_ = 4.0 + (in.r_ - in.g_) / delta;	// between magenta & cyan
	}

	out.h_ *= 60.0;	// degrees

	if (out.h_ < 0.0) {
		out.h_ += 360.0;
	}
}

COMMON_API void	HSV2RGB(const hsv_s& in, rgb_s& out) {
	double	hh, p, q, t, ff;
	long	i;

	if (in.s_ <= 0.0) {       // < is bogus, just shuts up warnings
		out.r_ = in.v_;
		out.g_ = in.v_;
		out.b_ = in.v_;
		return;
	}

	hh = in.h_;

	if (hh >= 360.0) {
		hh = 0.0;
	}
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v_ * (1.0 - in.s_);
	q = in.v_ * (1.0 - (in.s_ * ff));
	t = in.v_ * (1.0 - (in.s_ * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r_ = in.v_;
		out.g_ = t;
		out.b_ = p;
		break;
	case 1:
		out.r_ = q;
		out.g_ = in.v_;
		out.b_ = p;
		break;
	case 2:
		out.r_ = p;
		out.g_ = in.v_;
		out.b_ = t;
		break;
	case 3:
		out.r_ = p;
		out.g_ = q;
		out.b_ = in.v_;
		break;
	case 4:
		out.r_ = t;
		out.g_ = p;
		out.b_ = in.v_;
		break;
	case 5:
	default:
		out.r_ = in.v_;
		out.g_ = p;
		out.b_ = q;
		break;
	}
}

COMMON_API void RGBInterpolate(const rgb_s& start, const rgb_s& stop, double t, rgb_s& out) {
	hsv_s hsv_start, hsv_stop, hsv_int;

	// convert to hsv format
	RGB2HSV(start, hsv_start);
	RGB2HSV(stop, hsv_stop);

	// clamp
	if (t < 0.0) {
		t = 0.0;
	}

	if (t > 1.0) {
		t = 1.0;
	}

	hsv_int.h_ = hsv_start.h_ + (hsv_stop.h_ - hsv_start.h_) * t;
	hsv_int.s_ = hsv_start.s_ + (hsv_stop.s_ - hsv_start.s_) * t;
	hsv_int.v_ = hsv_start.v_ + (hsv_stop.v_ - hsv_start.v_) * t;

	HSV2RGB(hsv_int, out);
}

/*
================================================================================
image
================================================================================
*/

static bool Image_LoadBMP(const char* filename, image_s& image);


COMMON_API bool Img_Load(const char* filename, image_s& image) {
	const char * ext = strrchr(filename, '.');
	if (!ext) {
		printf("Could not get filename extension.\n");
		return false;
	}

	if (Str_ICmp(ext + 1, "bmp") == 0) {
		return Image_LoadBMP(filename, image);
	}
	else {
		printf("Unsupported image file format %s.\n", ext + 1);
		return false;
	}
}

COMMON_API void Img_Free(image_s& image) {
	if (image.pixels_) {
		TEMP_FREE(image.pixels_);
		image.pixels_ = nullptr;
	}

	image.width_ = image.height_ = 0;
}

#pragma pack(push, 1)

struct bmpfilehead_s {
	word_t				bfType_;
	dword_t				bfSize;
	word_t				bfReserved1;
	word_t				bfReserved2;
	dword_t				bfOffBits;
};

struct bmpinfohead_s {
	dword_t				biSize;
	int					biWidth;
	int					biHeight;
	word_t				biPlanes;
	word_t				biBitCount;
	dword_t				biCompression;
	dword_t				biSizeImage;
	int					biXPelsPerMeter;
	int					biYPelsPerMeter;
	dword_t				biClrUsed;
	dword_t				biClrImportant;
};

#pragma pack(pop)

#ifndef BI_RGB
#define BI_RGB			0L
#endif

#ifndef BI_RLE8
#define BI_RLE8			1L
#endif

#ifndef BI_RLE4
#define BI_RLE4			2L
#endif

#ifndef BI_BITFIELDS
#define BI_BITFIELDS	3L
#endif

#ifndef BI_JPEG
#define BI_JPEG			4L
#endif

#ifndef BI_PNG
#define BI_PNG			5L
#endif


static bool Image_LoadBMP(const char* filename, image_s& image) {
	memset(&image, 0, sizeof(image));

	void* buffer = nullptr;
	int32_t file_size = 0;
	if (!File_LoadBinary32(filename, buffer, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	bmpfilehead_s* filehead = (bmpfilehead_s*)buffer;
	bmpinfohead_s* infohead = (bmpinfohead_s*)(filehead + 1);

	if (8 == infohead->biBitCount) {
		if (BI_RGB == infohead->biCompression) { // uncompressed mode
			int src_line_len = (infohead->biWidth + 3) & ~3;

			int valid_size = (int)sizeof(bmpfilehead_s) + (int)sizeof(bmpinfohead_s) 
				           + 4 * 256 + src_line_len * infohead->biHeight;
			if (valid_size != file_size) {
				File_FreeBinary(buffer);
				printf("Bad size\n");
				return false;
			}

			image.width_ = infohead->biWidth;
			image.height_ = infohead->biHeight;
			image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

			if (!image.pixels_) {
				File_FreeBinary(buffer);
				printf("Could not allocate memory for image\n");
				return false;
			}

			byte_t* palette = (byte_t*)(infohead + 1);
			byte_t* src = palette + 4 * 256;

			int dst_line_len = infohead->biWidth * 4;

			for (int h = 0; h < infohead->biHeight; h++) { // OpenGL store bottom row first, same as BMP
				byte_t* src_line = src + src_line_len * h;
				byte_t* dst_line = image.pixels_ + dst_line_len * h;

				for (int w = 0; w < infohead->biWidth; w++) {
					byte_t idx = src_line[w];

					byte_t* src_clr = palette + idx * 4;
					byte_t* dst_clr = dst_line + w * 4;

					dst_clr[0] = src_clr[2]; // red
					dst_clr[1] = src_clr[1]; // green
					dst_clr[2] = src_clr[0]; // blue
					dst_clr[3] = 255;		 // opaque
				}
			}
		}
		else if (BI_RLE8 == infohead->biCompression) { 
			// http://msdn.microsoft.com/en-us/library/windows/desktop/dd183383%28v=vs.85%29.aspx
			image.width_ = infohead->biWidth;
			image.height_ = infohead->biHeight;
			image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

			if (!image.pixels_) {
				File_FreeBinary(buffer);
				printf("Could not allocate memory for image\n");
				return false;
			}

			// fill to white
			memset(image.pixels_, 255, infohead->biWidth * 4 * infohead->biHeight);

			byte_t* palette = (byte_t*)(infohead + 1);
			byte_t* src = palette + 4 * 256;

			int dst_line_len = infohead->biWidth * 4;

			byte_t* s = src;

			int line = 0;
			int clrxpos = 0;
			byte_t* dst_line = (byte_t*)image.pixels_;

			bool breakloop = false;
			while (!breakloop) {
				byte_t byte1 = s[0];
				byte_t byte2 = s[1];
				s += 2;

				if (byte1 > 0) {
					byte_t runlen = byte1;
					byte_t clridx = byte2;

					byte_t* src_clr = palette + clridx * 4;

					for (int i = 0; i < (int)runlen; i++) {
						byte_t* dst_clr = dst_line + clrxpos * 4;

						dst_clr[0] = src_clr[2]; // red
						dst_clr[1] = src_clr[1]; // green
						dst_clr[2] = src_clr[0]; // blue
						dst_clr[3] = 255;		 // opaque

						clrxpos++;
					}
				}
				else { // 0 == byte1
					if (byte2 >= 0x03) {
						byte_t clrlen = byte2;
						for (int i = 0; i < (int)clrlen; i++) {
							byte_t clridx = s[i];

							byte_t* src_clr = palette + clridx * 4;
							byte_t* dst_clr = dst_line + clrxpos * 4;

							dst_clr[0] = src_clr[2]; // red
							dst_clr[1] = src_clr[1]; // green
							dst_clr[2] = src_clr[0]; // blue
							dst_clr[3] = 255;		 // opaque

							clrxpos++;
						}
						s += clrlen;
					}
					else {
						switch (byte2) {
						case 0: // end of line
							line++;
							dst_line = (byte_t*)image.pixels_ + dst_line_len * line;
							if (clrxpos != infohead->biWidth) {
								TEMP_FREE(image.pixels_);
								image.pixels_ = nullptr;
								File_FreeBinary(buffer);
								printf("Bad data\n");
								return false;
							}
							clrxpos = 0;
							break;
						case 1: // end of bitmap
							breakloop = true;
							break;
						case 2: // Delta.The 2 bytes following the escape contain unsigned values indicating the horizontal and vertical offsets of the next pixel from the current position.
							TEMP_FREE(image.pixels_);
							image.pixels_ = nullptr;
							File_FreeBinary(buffer);
							printf("Did not know how to handle delta yet\n");
							return false;
						}
					}
				}
			}
		}
		else {
			File_FreeBinary(buffer);
			printf("Unsupported compression mode %d\n", infohead->biCompression);
			return false;
		}

	}
	else if (16 == infohead->biBitCount) {
		int src_line_len = (infohead->biWidth * 2 + 3) & ~3;

		int valid_size = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s) + src_line_len * infohead->biHeight;
		if (valid_size > file_size) {
			File_FreeBinary(buffer);
			printf("Bad size\n");
			return false;
		}

		image.width_ = infohead->biWidth;
		image.height_ = infohead->biHeight;
		image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

		if (!image.pixels_) {
			File_FreeBinary(buffer);
			printf("Could not allocate memory for image\n");
			return false;
		}

		byte_t * src = (byte_t*)(infohead + 1);

		int dst_line_len = infohead->biWidth * 4;

		for (int h = 0; h < infohead->biHeight; h++) { // OpenGL store bottom row first, same as BMP
			byte_t* src_line = src + src_line_len * h;
			byte_t* dst_line = (byte_t*)image.pixels_ + dst_line_len * h;

			for (int w = 0; w < infohead->biWidth; w++) {
				uint16_t src_clr = *(uint16_t*)(src_line + w * 2);
				byte_t * dst_clr = dst_line + w * 4;

				byte_t src_b = (byte_t)(src_clr & 0x001f);
				byte_t src_g = (byte_t)((src_clr & 0x07e0) >> 5);
				byte_t src_r = (byte_t)((src_clr & 0xf800) >> 11);

				dst_clr[0] = src_r; // red
				dst_clr[1] = src_g; // green
				dst_clr[2] = src_b; // blue
				dst_clr[3] = 255;   // opaque
			}
		}
	}
	else if (24 == infohead->biBitCount) {
		int src_line_len = (infohead->biWidth * 3 + 3) & ~3;

		int valid_size = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s) + src_line_len * infohead->biHeight;
		if (valid_size > file_size) {
			File_FreeBinary(buffer);
			printf("Bad size\n");
			return false;
		}

		image.width_ = infohead->biWidth;
		image.height_ = infohead->biHeight;
		image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

		if (!image.pixels_) {
			File_FreeBinary(buffer);
			printf("Could not allocate memory for image\n");
			return false;
		}

		byte_t * src = (byte_t*)(infohead + 1);

		int dst_line_len = infohead->biWidth * 4;

		for (int h = 0; h < infohead->biHeight; h++) { //OpenGL store bottom row first, same as BMP
			byte_t* src_line = src + src_line_len * h;
			byte_t* dst_line = image.pixels_ + dst_line_len * h;

			for (int w = 0; w < infohead->biWidth; w++) {
				byte_t* src_clr = src_line + w * 3;
				byte_t* dst_clr = dst_line + w * 4;

				dst_clr[0] = src_clr[2]; // red
				dst_clr[1] = src_clr[1]; // green
				dst_clr[2] = src_clr[0]; // blue
				dst_clr[3] = 255;        //opaque
			}
		}
	}
	else if (32 == infohead->biBitCount) {
		int src_line_len = infohead->biWidth * 4;

		int valid_size = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s) + src_line_len * infohead->biHeight;
		if (valid_size > file_size) {
			File_FreeBinary(buffer);
			printf("Bad size\n");
			return false;
		}

		image.width_ = infohead->biWidth;
		image.height_ = infohead->biHeight;
		image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

		if (!image.pixels_) {
			File_FreeBinary(buffer);
			printf("Could not allocate memory for image\n");
			return false;
		}

		byte_t* src = (byte_t*)(infohead + 1);

		int dst_line_len = infohead->biWidth * 4;

		for (int h = 0; h < infohead->biHeight; h++) { //OpenGL store bottom row first, same as BMP
			byte_t* src_line = src + src_line_len * h;
			byte_t* dst_line = image.pixels_ + dst_line_len * h;

			for (int w = 0; w < infohead->biWidth; w++) {
				byte_t* src_clr = src_line + w * 4;
				byte_t* dst_clr = dst_line + w * 4;
				dst_clr[0] = src_clr[2]; // red
				dst_clr[1] = src_clr[1]; // green
				dst_clr[2] = src_clr[0]; // blue
				dst_clr[3] = src_clr[3]; // alpha
			}
		}
	}

	File_FreeBinary(buffer);

	return true;
}
