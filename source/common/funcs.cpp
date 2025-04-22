/******************************************************************************
 helper
 *****************************************************************************/

#include "inc.h"
#include <pnglibconf.h>
#include <png.h>
#include <atomic>

/*
================================================================================
common
================================================================================
*/

static char	g_data_folder[MAX_PATH];
static char g_shaders_folder[MAX_PATH];

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

static bool Str_ExtractFileDirSelf(wchar_t * path) {
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

static void InitHalfFloatToFloatTables();
static void InitFloatToHalfFloatTables();

void Common_Init() {
#if defined(PLATFORM_WINDOWS)
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), buffer, MAX_PATH);

	Str_ExtractFileDirSelf(buffer);
	wcscat_s(buffer, L"\\..\\..\\..");
	Str_EraseDoubleDotsInPath(buffer);

	char cbuffer[MAX_PATH];
	Str_UTF16ToUTF8((const char16_t*)buffer, cbuffer, MAX_PATH);

	Str_SPrintf(g_data_folder, COUNT_OF(g_data_folder), "%s\\data", cbuffer);
	Str_SPrintf(g_shaders_folder, COUNT_OF(g_shaders_folder), "%s\\shaders", cbuffer);
#endif

	// randomize
	SRand((unsigned)time(NULL));

	InitHalfFloatToFloatTables();
	InitFloatToHalfFloatTables();
}

COMMON_API const char * GetDataFolder() {
	return g_data_folder;
}

COMMON_API const char* GetShadersFolder() {
	return g_shaders_folder;
}

COMMON_API bool IsBigEndian() {
	union foo_s {
		struct {
			unsigned char b1_;
			unsigned char b2_;
		};
		uint16_t	u16_;
	} foo;

	foo.b1_ = 1;
	foo.b2_ = 0;

	return foo.u16_ != 1;
}

// Jeroen van der Zijp, 2010, Fast Half Float Conversions

static uint32_t g_mantissa_table[2048];
static uint32_t g_exponent_table[64];
static uint32_t g_offset_table[64];

static void InitHalfFloatToFloatTables() {

	auto convert_mantissa = [](uint32_t i) {
		uint32_t m = i << 13;	// Zero pad mantissa bits
		uint32_t e = 0;			// Zero exponent

		while (!(m & 0x00800000)) {	// While not normalized
			e -= 0x00800000;		// Decrement exponent (1 << 23)
			m <<= 1;				// Shift mantissa
		}

		m &= ~0x00800000;			// Clear leading 1 bit
		e += 0x38800000;			// Adjust bias ((127-14) << 23)
		return m | e;				// Return combined number
	};

	// mantissa table
	g_mantissa_table[0] = 0;

	for (uint32_t i = 1; i < 1024; ++i) {
		g_mantissa_table[i] = convert_mantissa(i);
	}

	for (uint32_t i = 1024; i < 2048; ++i) {
		g_mantissa_table[i] = 0x38000000 + ((i - 1024) << 13);
	}

	// exponent table
	g_exponent_table[0] = 0;
	g_exponent_table[32] = 0x8000000;

	for (uint32_t i = 1; i <= 30; ++i) {
		g_exponent_table[i] = i << 23;
	}

	for (uint32_t i = 33; i <= 62; ++i) {
		g_exponent_table[i] = 0x80000000 + ((i-32) << 23);
	}

	g_exponent_table[31] = 0x47800000;
	g_exponent_table[63] = 0xC7800000;

	// offset table
	g_offset_table[0] = 0;
	g_offset_table[32] = 0;

	for (uint32_t i = 1; i < 32; ++i) {
		g_offset_table[i] = 1024;
	}
}

static float16_t g_base_table[512];
static uint8_t g_shift_table[512];

static void InitFloatToHalfFloatTables() {
	for (uint32_t i = 0; i < 256; ++i) {
		int32_t e = i - 127;
		if (e < -24) {		// Very small numbers map to zero
			g_base_table[i | 0x000] = 0x0000;
			g_base_table[i | 0x100] = 0x8000;
			g_shift_table[i | 0x000] = 24;
			g_shift_table[i | 0x100] = 24;
		}
		else if (e < -14) {	// Small numbers map to denorms
			g_base_table[i | 0x000] = (0x0400 >> (-e-14));
			g_base_table[i | 0x100] = (0x0400 >> (-e - 14)) | 0x8000;
			g_shift_table[i | 0x000] = -e-1;
			g_shift_table[i | 0x100] = -e-1;
		}
		else if (e <= 15) {	// Normal numbers just lose precision
			g_base_table[i | 0x000] = ((e+15) << 10);
			g_base_table[i | 0x100] = ((e + 15) << 10) | 0x8000;
			g_shift_table[i | 0x000] = 13;
			g_shift_table[i | 0x100] = 13;
		}
		else if (e < 128) {	// Large numbers map to Infinity
			g_base_table[i | 0x000] = 0x7C00;
			g_base_table[i | 0x100] = 0xFC00;
			g_shift_table[i | 0x000] = 24;
			g_shift_table[i | 0x100] = 24;
		}
		else {	// Infinity and NaN's stay Infinity and NaN's
			g_base_table[i | 0x000] = 0x7C00;
			g_base_table[i | 0x100] = 0xFC00;
			g_shift_table[i | 0x000] = 13;
			g_shift_table[i | 0x100] = 13;
		}
	}
}

union float_uint_s {
	uint32_t i_;
	float	f_;
} fu;

COMMON_API float HalfFloatToFloat(float16_t h) {
	float_uint_s fu;

	fu.i_ = g_mantissa_table[g_offset_table[h >> 10] + (h & 0x3ff)] + g_exponent_table[h >> 10];
	
	return fu.f_;
}

COMMON_API float16_t FloatToHalfFloat(float f) {
	float_uint_s fu;

	fu.f_ = f;

	return g_base_table[(fu.i_ >> 23) & 0x1ff] + ((fu.i_ & 0x007fffff) >> g_shift_table[(fu.i_ >> 23) & 0x1ff]);
}

/*
================================================================================
random number generator
================================================================================
*/
static std::atomic<uint32_t> g_rand_next = 1;

COMMON_API void SRand(uint32_t seed) {
	g_rand_next.store(seed);
}

static const uint32_t RAND_MAX_VALUE = 32767;

COMMON_API uint32_t Rand() {
	uint32_t next = g_rand_next.load();
	next = next * 1103515245 + 12345;
	g_rand_next.store(next);
	return next / 65536 % 32768;
}

// return [0, 1]
COMMON_API float Rand01() {
	return Rand() / (float)RAND_MAX_VALUE;
}

// return [-1, 1]
COMMON_API float RandNeg1Pos1() {
	float f = Rand() / (float)RAND_MAX_VALUE;
	return f * 2.0f - 1.0f;
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
	if (!data) {
		fclose(f);
		return false;
	}

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

COMMON_API bool File_LoadText(const char* filename, char*& data, int32_t& len) {
	data = nullptr;
	len = 0;

	FILE* f = File_Open(filename, "rb");

	if (!f) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = (char*)TEMP_ALLOC(sz + 1);
	if (!data) {
		fclose(f);
		return false;
	}

	int read_size = (int)fread(data, 1, sz, f);
	fclose(f);

	if (read_size != sz) {
		TEMP_FREE(data);
		return false;
	}
	else {
		data[sz] = 0;
		len = sz;
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
COMMON_API void Str_Copy(char* dst, int dst_cap, const char* src) {
	if (dst == src) {
		return;
	}

#if defined(_MSC_VER)
	strncpy_s(dst, dst_cap, src, _TRUNCATE);
#endif

#if defined(__GNUC__)
	strncpy(dst, src, dst_cap - 1);
	size_t l = strlen(dst);
	dst[l] = 0;
#endif
}

COMMON_API void Str_Cat(char* dst, int dst_cap, const char* src) {
#if defined(_MSC_VER)
	errno_t e = strncat_s(dst, dst_cap, src, _TRUNCATE);
	if (e) {
		printf("strncat_s error: %d\n", e);
	}
#endif

#if defined(__GNUC__)
	int l = (int)strlen(dst);
	if (dst_cap > (l + 1)) {
		strncat(dst, src, dst_cap - l - 1);
	}
#endif
}

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

COMMON_API char* Str_SkipWhiteSpace(char* pc) {
	while (*pc && (*pc == ' ' || *pc == '\t' || *pc == '\r' || *pc == '\n')) {
		pc++;
	}
	return pc;
}

COMMON_API char* Str_SkipCharactor(char* pc) {
	while (*pc && *pc != ' ' && *pc != '\t' && *pc != '\r' && *pc != '\n') {
		pc++;
	}
	return pc;
}

COMMON_API bool Str_ExtractFileDir(const char* path, char* dir, int dir_cap) {
	dir[0] = 0;
	int l = (int)strlen(path);
	for (int i = l - 1; i >= 0; --i) {
		if (IsDirectorySeparator(path[i])) {
			int filedir_element_count = i;
			if (dir_cap <= filedir_element_count) {
				printf("dir buffer overflow\n");
				return false;
			}
			memcpy(dir, path, filedir_element_count * sizeof(char));
			dir[filedir_element_count] = 0;
			return true;
		}
	}
	// directory separator not found
	return false;
}

COMMON_API bool Str_ExtractFileName(const char* path, char* filename, int filename_cap) {
	filename[0] = 0;
	int l = (int)strlen(path);
	for (int i = l - 1; i >= 0; --i) {
		if (IsDirectorySeparator(path[i])) {
			int filename_element_count = l - i - 1;
			if (filename_cap <= filename_element_count) {
				printf("Filename buffer overflow\n");
				return false;
			}
			memcpy(filename, path + i + 1, filename_element_count);
			filename[filename_element_count] = 0;
			return true;
		}
	}

	// no \ or / character been found
	if (filename_cap <= l) {
		printf("filename buffer overflow\n");
		return false;
	}

	Str_Copy(filename, filename_cap, path);
	return true;
}

COMMON_API bool Str_ReplaceFileNameExt(char* path, int path_cap, const char* new_ext) {
	char* p_dot = strrchr(path, '.');
	if (p_dot) {
		p_dot[1] = '\0';
		Str_Cat(path, path_cap, new_ext);
		return true;
	}
	else {
		return false;
	}
}

// return number tokens

COMMON_API int Str_Tokenize(const char* s, char** buffers, int each_buffer_size, int buffer_count) {
	char local_buffer[1024];
	char * copy_s = local_buffer;

	int s_len = (int)strlen(s);
	if (s_len >= 1024) {
		copy_s = (char*)TEMP_ALLOC(s_len + 1);
	}

	if (!copy_s) {
		printf("Could not allocate memory to copy source string\n");
		return 0;
	}

	memcpy(copy_s, s, s_len);
	copy_s[s_len] = 0;

	for (int i = 0; i < buffer_count; ++i) {
		buffers[i][0] = 0;
	}

	int token_count = 0;
	char * pc = copy_s;
	while (*pc) {
		pc = Str_SkipWhiteSpace(pc);
		if (!*pc) {
			break;
		}

		char* first = pc;
		pc = Str_SkipCharactor(pc);

		if (!*pc) {
			Str_Copy(buffers[token_count++], each_buffer_size, first);
			if (token_count >= buffer_count) {
				break;
			}
			break;
		}
		else {
			*pc = 0;
			Str_Copy(buffers[token_count++], each_buffer_size, first);
			if (token_count >= buffer_count) {
				break;
			}
			pc++;
		}
	}

	if (copy_s != local_buffer) {
		TEMP_FREE(copy_s);
	}

	return token_count;
}

COMMON_API int Str_Split(char* s, char delimiter, char** items, int max_item) {
	int item_count = 0;
	char* pc = s;

	while (item_count < max_item) {
		if (delimiter) {
			items[item_count++] = pc;

			char* comma = strchr(pc, delimiter);
			if (comma) {
				*comma = 0;
				pc = comma + 1;
			}
			else {
				break;
			}
		}
		else {
			pc = Str_SkipWhiteSpace(pc);

			if (*pc) {
				items[item_count++] = pc;
			}

			while (*pc && *pc != ' ' && *pc != '\t') {
				pc++;
			}

			if (*pc == 0) {
				break;
			}
			else {
				*pc = 0;
				pc++;
			}
		}
	}

	return item_count;
}

/*
================================================================================
Lexer
================================================================================
*/
Lexer::Lexer() : s_(nullptr), pc_(nullptr)
{
	buffer_[0] = '\0';
}

Lexer::~Lexer() {
	// do nothing
}

void Lexer::SetPtr(const char * s) {
	s_ = s;
	pc_ = s;
}

const char * Lexer::GetToken() {
	if (!pc_) {
		return nullptr;
	}

	// skip white char
	char c = *pc_;
	while (c && (c == 32 || c == 9 || c == 13 || c == 10)) { // space, table, return, new line
		pc_++;
		c = *pc_;
	}

	if (!c) {
		return nullptr;
	}

	int token_len = 0;
	while (c && c != 32 && c != 9 && c != 13 && c != 10) {
		if (token_len >= MAX_TOKEN_LEN) {
			printf("Token too long\n");
			return nullptr;
		}

		buffer_[token_len++] = c;
		pc_++;
		c = *pc_;
	}

	buffer_[token_len] = 0;
	return buffer_;
}

void Lexer::SkipLine() {
	if (!pc_) {
		return;
	}

	while (*pc_) {
		if (*pc_ == 13) {
			pc_++;
			if (*pc_ == 10) {
				pc_++;
			}
			break;
		}
		else if (*pc_ == 10) {
			pc_++;
			break;
		}
		else {
			pc_++;
		}
	}
}

const char * Lexer::ReadLine() {
	if (!pc_) {
		return nullptr;
	}

	if (!*pc_) {
		return nullptr;
	}

	int line_len = 0;
	char c = *pc_;
	while (c && c != 13 && c != 10) {
		if (line_len >= MAX_TOKEN_LEN) {
			printf("Line too long\n");
			return nullptr;
		}
		buffer_[line_len++] = c;
		pc_++;
		c = *pc_;
	}

	buffer_[line_len] = 0;

	if (*pc_) {
		if (*pc_ == 13) {
			pc_++;
		}
		if (*pc_ == 10) {
			pc_++;
		}
	}

	return buffer_;
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
static bool Image_LoadPNG(const char* filename, image_s& image);
static bool Image_LoadTGA(const char* filename, image_s& image);

COMMON_API bool Img_Create(int width, int height, image_format_t fmt, image_s& image) {
	image.width_ = width;
	image.height_ = height;
	image.format_ = fmt;
	image.pixels_ = nullptr;

	if (fmt == image_format_t::R8G8B8A8) {
		image.pixels_ = (byte_t*)TEMP_ALLOC(width * height * 4);
	}
	else if (fmt == image_format_t::R16G16B16A16_FLOAT) {
		image.pixels_ = (byte_t*)TEMP_ALLOC(width * height * 8);
	}
	else {
		printf("Unknown image format %d\n", (int)fmt);
	}

	return image.pixels_ != nullptr;
}

COMMON_API bool Img_Load(const char* filename, image_s& image) {
	const char * ext = strrchr(filename, '.');
	if (!ext) {
		printf("Could not get filename extension.\n");
		return false;
	}

	if (Str_ICmp(ext, ".bmp") == 0) {
		return Image_LoadBMP(filename, image);
	}
	else if (Str_ICmp(ext, ".png") == 0) {
		return Image_LoadPNG(filename, image);
	}
	else if (Str_ICmp(ext, ".tga") == 0) {
		return Image_LoadTGA(filename, image);
	}
	else if (Str_ICmp(ext, ".dds") == 0) {
		bool Image_LoadDDS(const char* filename, image_s & image);
		return Image_LoadDDS(filename, image);
	}
	else {
		printf("Unsupported image file format %s.\n", ext + 1);
		return false;
	}
}

static bool Image_SaveBMP(const char* filename, const image_s& image);
static bool Image_SavePNG(const char* filename, const image_s& image);

COMMON_API bool Img_Save(const char* filename, const image_s& image) {
	const char* ext = strrchr(filename, '.');
	if (!ext) {
		printf("Could not get filename extension.\n");
		return false;
	}

	if (Str_ICmp(ext, ".bmp") == 0) {
		return Image_SaveBMP(filename, image);
	}
	else if (Str_ICmp(ext, ".png") == 0) {
		return Image_SavePNG(filename, image);
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
	word_t				bfType;
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

			int palette_size = file_size - ((int)sizeof(bmpfilehead_s) + (int)sizeof(bmpinfohead_s)
				+ src_line_len * infohead->biHeight);

			if (palette_size < 0) {
				File_FreeBinary(buffer);
				printf("Bad size\n");
				return false;
			}

			image.width_ = infohead->biWidth;
			image.height_ = infohead->biHeight;
			image.format_ = image_format_t::R8G8B8A8;
			image.pixels_ = (byte_t*)TEMP_ALLOC(infohead->biWidth * 4 * infohead->biHeight);

			if (!image.pixels_) {
				File_FreeBinary(buffer);
				printf("Could not allocate memory for image\n");
				return false;
			}

			byte_t* palette = (byte_t*)(infohead + 1);
			byte_t* src = palette + palette_size;

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
			image.format_ = image_format_t::R8G8B8A8;
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
		image.format_ = image_format_t::R8G8B8A8;
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
		image.format_ = image_format_t::R8G8B8A8;
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
		image.format_ = image_format_t::R8G8B8A8;
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

static bool Image_LoadPNG(const char* filename, image_s& image) {
	memset(&image, 0, sizeof(image));

	bool result = false;

	png_image image_png;
	memset(&image_png, 0, sizeof(image_png));
	image_png.version = PNG_IMAGE_VERSION;

	if (png_image_begin_read_from_file(&image_png, filename)) {
		png_bytep buffer_png;

		image_png.format = PNG_FORMAT_RGBA;
		size_t size = PNG_IMAGE_SIZE(image_png); // get image size
		buffer_png = (png_bytep)TEMP_ALLOC(size);

		if (png_image_finish_read(&image_png, NULL /*background*/, buffer_png, 0 /*row_stride*/, NULL /*colormap*/)) {
			// test write to bmp

			image.width_ = (int)image_png.width;
			image.height_ = (int)image_png.height;
			image.format_ = image_format_t::R8G8B8A8;
			image.pixels_ = (byte_t*)TEMP_ALLOC(image_png.height * image_png.width * 4 /* rgba */);

			if (image.pixels_) {
				// revert y
				for (int y = 0; y < (int)image_png.height; ++y) {
					byte_t* src_line = buffer_png + (image_png.height - y - 1) * image_png.width * 4;
					byte_t* dst_line = image.pixels_ + y * image_png.width * 4;
					memcpy(dst_line, src_line, image_png.width * 4);
				}

				result = true;

			}
			else {
				printf("Could not allocate memory to load image \"%s\"\n", filename);
			}
		}
		else {
			printf("png_image_finish_read failed\n");
		}

		png_image_free(&image_png); //2017-01-20 Fri.

		TEMP_FREE(buffer_png);
		buffer_png = nullptr;
	}
	else {
		printf("png_image_begin_read_from_file failed\n");
	}

	return result;
}

static bool Image_HasAlpha(const image_s& image);

static bool Image_LoadTGA(const char* filename, image_s& image) {
	static const byte_t IMAGE_TYPE_UNCOMPRESSED_TRUE_COLOR = 2;
	static const byte_t IMAGE_TYPE_RUN_LENGTH_TRUE_COLOR = 10;

#pragma pack(push, 1)
	struct head_t {
		byte_t	id_length;
		byte_t	colormap_type;
		byte_t	image_type;
		word_t	colormap_index;
		word_t	colormap_length;
		byte_t	colormap_size;
		word_t	x_origin;
		word_t	y_origin;
		word_t	width;
		word_t	height;
		byte_t	pixel_size;
		byte_t	attributes;
	};
#pragma pack(pop)

	struct stream_t {
		byte_t*	data;
		int		pos;
		byte_t	GetByte() { return data[pos++]; }
	};

	memset(&image, 0, sizeof(image));

	void* buffer = nullptr;
	int32_t file_size = 0;
	if (!File_LoadBinary32(filename, buffer, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	head_t* head = (head_t*)buffer;
	byte_t a = head->attributes & (1 << 5); // 0: left-bottom is origin, 1: left-top is origin
	if (a != 0 || (head->image_type != IMAGE_TYPE_UNCOMPRESSED_TRUE_COLOR && head->image_type != IMAGE_TYPE_RUN_LENGTH_TRUE_COLOR)) {
		File_FreeBinary(buffer);
		printf("Unsupported tga image type\n");
		return false;
	}

	if (head->colormap_type != 0 || (head->pixel_size != 24 && head->pixel_size != 32)) {
		File_FreeBinary(buffer);
		printf("Unsupported tga pixel size\n");
		return false;
	}

	int columns = head->width;
	int rows = head->height;
	int numpixels = columns * rows;

	//setup reading stream
	stream_t stream;
	stream.data = (byte_t*)buffer;
	stream.pos = sizeof(*head);

	byte_t * targa_rgba = (byte_t*)TEMP_ALLOC(numpixels * 4); // destination rgba data
	stream.pos += head->id_length; // skip TARGA image comment

	if (IMAGE_TYPE_UNCOMPRESSED_TRUE_COLOR == head->image_type) {
		for (int row = rows - 1; row >= 0; row--) {
			byte_t * pixbuf = targa_rgba + row * columns * 4;
			for (int column = 0; column < columns; ++column) {
				if (24 == head->pixel_size) {
					pixbuf[2] = stream.GetByte(); // blue
					pixbuf[1] = stream.GetByte(); // green
					pixbuf[0] = stream.GetByte(); // red
					pixbuf[3] = 255;
				}
				else { // 32
					pixbuf[2] = stream.GetByte(); // blue
					pixbuf[1] = stream.GetByte(); // green
					pixbuf[0] = stream.GetByte(); // red
					pixbuf[3] = stream.GetByte(); // alpha
				}
				pixbuf += 4;
			}
		}
	}
	else { // IMAGE_TYPE_RUN_LENGTH_TRUE_COLOR
		for (int row = rows - 1; row >= 0; row--) {
			byte_t * pixbuf = targa_rgba + row * columns * 4;
			for (int column = 0; column < columns;) {
				byte_t packet_header = stream.GetByte();
				int packet_size = 1 + (packet_header & 0x7f);

				byte_t r, g, b, a;

				if (packet_header & 0x80) { // first bit
					if (24 == head->pixel_size) {
						b = stream.GetByte(); // blue
						g = stream.GetByte(); // green
						r = stream.GetByte(); // red
						a = 255;
					}
					else { // 32
						b = stream.GetByte(); // blue
						g = stream.GetByte(); // green
						r = stream.GetByte(); // red
						a = stream.GetByte(); // alpha
					}

					for (int j = 0; j < packet_size; ++j) {
						pixbuf[0] = r;
						pixbuf[1] = g;
						pixbuf[2] = b;
						pixbuf[3] = a;
						pixbuf += 4;

						++column;
						if (column == columns) {  // run spans across rows
							column = 0;
							if (row > 0) {
								row--;
							}
							else {
								goto break_out;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else { // non run-length packet
					for (int j = 0; j < packet_size; ++j) {
						if (24 == head->pixel_size) {
							pixbuf[2] = stream.GetByte(); // blue
							pixbuf[1] = stream.GetByte(); // green
							pixbuf[0] = stream.GetByte(); // red
							pixbuf[3] = 255;
						}
						else { // 32
							pixbuf[2] = stream.GetByte(); // blue
							pixbuf[1] = stream.GetByte(); // green
							pixbuf[0] = stream.GetByte(); // red
							pixbuf[3] = stream.GetByte(); // alpha
						}
						pixbuf += 4;

						++column;
						if (column == columns) { // pixel packet run spans across rows
							column = 0;
							if (row > 0) {
								row--;
							}
							else {
								goto break_out;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		}
	break_out:;
	}

	image.width_ = head->width;
	image.height_ = head->height;
	image.format_ = image_format_t::R8G8B8A8;
	image.pixels_ = (byte_t*)TEMP_ALLOC(numpixels * 4);

	File_FreeBinary(buffer);

	if (!image.pixels_) {
		TEMP_FREE(targa_rgba);
		return false;
	}

	// revert y
	for (int y = 0; y < image.height_; ++y) {
		const byte_t* src_line = targa_rgba + (image.height_ - y - 1) * image.width_ * 4;
		byte_t * dst_line = image.pixels_ + y * image.width_ * 4;

		memcpy(dst_line, src_line, image.width_ * 4);
	}
	
	TEMP_FREE(targa_rgba);

	return true;
}

static bool Image_HasAlpha(const image_s& image) {
	if (image.format_ == image_format_t::R8G8B8A8) {
		int linewidth = image.width_ * 4;
		for (int y = 0; y < image.height_; ++y) {
			const byte_t* line = image.pixels_ + y * linewidth;
			for (int x = 0; x < image.width_; ++x) {
				const byte_t* pixel = line + x * 4;
				if (pixel[3] < 255) {
					return true;
				}
			}
		}
	}

	return false;
}

static bool Image_SaveBMP(const char* filename, const image_s& image) {
	if (image.format_ == image_format_t::R8G8B8A8) {
		if (Image_HasAlpha(image)) {
			int linewidth = image.width_ * 4;

			bmpfilehead_s filehead;

			filehead.bfType = 0x4d42;
			filehead.bfOffBits = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s);
			filehead.bfSize = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s) + linewidth * image.height_;
			filehead.bfReserved1 = 0;
			filehead.bfReserved2 = 0;

			bmpinfohead_s infohead;

			infohead.biBitCount = 32;
			infohead.biClrImportant = 0;
			infohead.biClrUsed = 0;
			infohead.biCompression = 0;
			infohead.biHeight = image.height_;
			infohead.biPlanes = 1;
			infohead.biSize = sizeof(infohead);
			infohead.biSizeImage = 0;
			infohead.biWidth = image.width_;
			infohead.biXPelsPerMeter = 0;
			infohead.biYPelsPerMeter = 0;

			unsigned char* dst = (unsigned char*)TEMP_ALLOC(image.height_ * linewidth);
			if (!dst) {
				return false;
			}

			for (int h = 0; h < image.height_; ++h) {
				unsigned char* dstline = dst + h * linewidth;
				const unsigned char* srcline = image.pixels_ + h * linewidth;
				for (int w = 0; w < image.width_; ++w) {
					unsigned char* dstclr = dstline + w * 4;
					const unsigned char* srcclr = srcline + w * 4;
					dstclr[0] = srcclr[2]; // B
					dstclr[1] = srcclr[1]; // G
					dstclr[2] = srcclr[0]; // R
					dstclr[3] = srcclr[3];
				}
			}

			FILE* f = File_Open(filename, "wb");
			if (f) {
				fwrite(&filehead, sizeof(filehead), 1, f);
				fwrite(&infohead, sizeof(infohead), 1, f);
				fwrite(dst, image.height_ * linewidth, 1, f);
				fclose(f);

				TEMP_FREE(dst);

				return true;
			}
			else {
				TEMP_FREE(dst);

				return false;
			}
		}
		else {
			int src_linewidth = image.width_ * 4;
			int dst_linewidth = (image.width_ * 3 + 3) & ~3;

			bmpfilehead_s filehead;

			filehead.bfType = 0x4d42;
			filehead.bfOffBits = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s);
			filehead.bfSize = sizeof(bmpfilehead_s) + sizeof(bmpinfohead_s) + dst_linewidth * image.height_;
			filehead.bfReserved1 = 0;
			filehead.bfReserved2 = 0;

			bmpinfohead_s infohead;

			infohead.biBitCount = 24;
			infohead.biClrImportant = 0;
			infohead.biClrUsed = 0;
			infohead.biCompression = 0;
			infohead.biHeight = image.height_;
			infohead.biPlanes = 1;
			infohead.biSize = sizeof(infohead);
			infohead.biSizeImage = 0;
			infohead.biWidth = image.width_;
			infohead.biXPelsPerMeter = 0;
			infohead.biYPelsPerMeter = 0;

			unsigned char* dst = (unsigned char*)TEMP_ALLOC(image.height_ * dst_linewidth);
			if (!dst) {
				return false;
			}

			for (int h = 0; h < image.height_; ++h) {
				unsigned char* dstline = dst + h * dst_linewidth;
				const unsigned char* srcline = image.pixels_ + h * src_linewidth;
				for (int w = 0; w < image.width_; ++w) {
					unsigned char* dstclr = dstline + w * 3;
					const unsigned char* srcclr = srcline + w * 4;
					dstclr[0] = srcclr[2]; // B
					dstclr[1] = srcclr[1]; // G
					dstclr[2] = srcclr[0]; // R
				}
			}

			FILE* f = File_Open(filename, "wb");
			if (f) {
				fwrite(&filehead, sizeof(filehead), 1, f);
				fwrite(&infohead, sizeof(infohead), 1, f);
				fwrite(dst, image.height_ * dst_linewidth, 1, f);
				fclose(f);

				TEMP_FREE(dst);

				return true;
			}
			else {
				TEMP_FREE(dst);

				return false;
			}
		}
	}
	else if (image.format_ == image_format_t::R16G16B16A16_FLOAT) {
		printf("TODO: save image_format_t::R16G16B16A16_FLOAT to bmp");
		return false;
	}
	else {
		printf("Unknown image format %d\n", (int)image.format_);
		return false;
	}
}

static bool Image_SavePNG(const char* filename, const image_s& image) {
	if (image.format_ == image_format_t::R8G8B8A8) {
		png_uint_32 dst_format = PNG_FORMAT_RGBA;	// PNG_FORMAT_RGB

		png_image image_png;
		memset(&image_png, 0, sizeof(image_png));
		image_png.version = PNG_IMAGE_VERSION;
		image_png.width = image.width_;
		image_png.height = image.height_;
		image_png.format = dst_format;

		size_t size = PNG_IMAGE_SIZE(image_png); // get image size
		png_bytep buffer_png = (png_bytep)TEMP_ALLOC(size);

		// convert height direction
		for (int h = 0; h < image.height_; ++h) {
			const byte_t* src_line = image.pixels_ + h * image.width_ * 4;
			png_bytep dst_line = buffer_png + (image.height_ - h - 1) * image.width_ * 4;

			memcpy(dst_line, src_line, image.width_ * 4);
		}

		// save to file
		bool ok = false;

		if (png_image_write_to_file(&image_png, filename, 0, buffer_png, PNG_IMAGE_ROW_STRIDE(image_png), nullptr)) {
			png_image_free(&image_png);
			ok = true;
		}
		else {
			printf("png_image_write_to_file failed\n");
		}

		TEMP_FREE(buffer_png);
		buffer_png = nullptr;

		return ok;
	}
	else if (image.format_ == image_format_t::R16G16B16A16_FLOAT) {

		png_uint_32 dst_format = PNG_FORMAT_RGBA;

		png_image image_png;
		memset(&image_png, 0, sizeof(image_png));
		image_png.version = PNG_IMAGE_VERSION;
		image_png.width = image.width_;
		image_png.height = image.height_;
		image_png.format = dst_format;

		size_t size = PNG_IMAGE_SIZE(image_png); // get image size
		png_bytep buffer_png = (png_bytep)TEMP_ALLOC(size);

		const float gamma_correction = 2.2f;

		// convert height direction
		for (int h = 0; h < image.height_; ++h) {
			const float16_t * src_line = (const float16_t*)image.pixels_ + h * image.width_ * 4;
			png_bytep dst_line = buffer_png + (image.height_ - h - 1) * image.width_ * 4;

			for (int x = 0; x < image.width_; ++x) {
				const float16_t* src_p = src_line + x * 4;
				png_bytep dst_p = dst_line + x * 4;

				float fr = HalfFloatToFloat(src_p[0]);
				float fg = HalfFloatToFloat(src_p[1]);
				float fb = HalfFloatToFloat(src_p[2]);
				float fa = HalfFloatToFloat(src_p[3]);

				// printf("%f,%f,%f,%f\n", r, g, b, a);

				byte_t br = (byte_t)((fr + 1.0f) * 0.5f * 255.0f);
				byte_t bg = (byte_t)((fg + 1.0f) * 0.5f * 255.0f);
				byte_t bb = (byte_t)((fb + 1.0f) * 0.5f * 255.0f);
				byte_t ba = (byte_t)((fa + 1.0f) * 0.5f * 255.0f);

				byte_t br2 = (byte_t)(255.0f * std::powf((float)br / 255.0f, gamma_correction));
				byte_t bg2 = (byte_t)(255.0f * std::powf((float)bg / 255.0f, gamma_correction));
				byte_t bb2 = (byte_t)(255.0f * std::powf((float)bb / 255.0f, gamma_correction));
				byte_t ba2 = (byte_t)(255.0f * std::powf((float)ba / 255.0f, gamma_correction));

				dst_p[0] = br2;
				dst_p[1] = bg2;
				dst_p[2] = bb2;
				dst_p[3] = ba2;
			}
		}

		// save to file
		bool ok = false;

		if (png_image_write_to_file(&image_png, filename, 0, buffer_png, PNG_IMAGE_ROW_STRIDE(image_png), nullptr)) {
			png_image_free(&image_png);
			ok = true;
		}
		else {
			printf("png_image_write_to_file failed\n");
		}

		TEMP_FREE(buffer_png);
		buffer_png = nullptr;

		return ok;
	}
	else {
		printf("Unknown image format %d\n", (int)image.format_);
		return false;
	}
}

/*
================================================================================
terrain
================================================================================
*/

bool gen_terrain_fault_formation(terrain_size_t sz, float min_z, float max_z, 
	int iterations, float filter, terrain_s& terrain);
bool gen_terrain_mid_point(terrain_size_t sz, float min_z, float max_z, float roughness, terrain_s& terrain);

COMMON_API uint32_t Terrain_GetVertexCountPerEdge(terrain_size_t sz) {
	// determine the vertex count per edge of the squared terrain
	switch (sz) {
	case terrain_size_t::TS_32:
		return 33;
	case terrain_size_t::TS_64:
		return 65;
	case terrain_size_t::TS_128:
		return 129;
	case terrain_size_t::TS_256:
		return 257;
	case terrain_size_t::TS_512:
		return 513;
	case terrain_size_t::TS_1K:
		return 1025;
	default:
		printf("unknown size %d, set vertex_count_per_edge to 33\n", sz);
		return 33;
	}
}

COMMON_API bool Terrain_Generate(const terrain_gen_params_s& params, terrain_s& terrain)
{
	switch (params.algo_) {
		case terrain_gen_algorithm_t::FAULT_FORMATION:
			return gen_terrain_fault_formation(params.sz_, 
				params.min_z_, params.max_z_, params.iterations_, params.filter_, terrain);
		case terrain_gen_algorithm_t::MID_POINT:
			return gen_terrain_mid_point(params.sz_, 
				params.min_z_, params.max_z_, params.roughness_, terrain);
		default:
			printf("Unsupported terrain generation algorithm\n");
			return false;
	}
}

COMMON_API void Terrain_Free(terrain_s& terrain) {
	if (terrain.heights_) {
		delete[] terrain.heights_;
		terrain.heights_ = nullptr;
	}
}

COMMON_API bool	Terrain_Texture(const terrain_texture_tiles_s& tile_images,
	const terrain_s& terrain, 
	int width, int height, image_s& texture) 
{
	struct tile_s {
		const char* filename_;
		image_s		image_;
		float		full_lower_;
		float		full_upper_;
		float		vanish_lower_;
		float		vanish_upper_;
	} tiles[4] = {};

	/*
	
	tile 
	
	|----- vanish_upper_
	|
	|----- full_upper_
	|      100%
	|----- full_lower_
	|
	|----- vanish_lower_
	
	*/

	auto GetPrecentage = [](const tile_s& tile, float z) -> float {
		if (z > tile.vanish_upper_ || z < tile.vanish_lower_) {
			return 0.0f;
		}

		if (z > tile.full_upper_) {
			return (tile.vanish_upper_ - z) / (tile.vanish_upper_ - tile.full_upper_);
		}

		if (z < tile.full_lower_) {
			return (z - tile.vanish_lower_) / (tile.full_lower_ - tile.vanish_lower_);
		}

		return 1.0f;
	};

	auto GetPixel = [](const image_s& image, float u, float v) -> glm::uvec3 {

		int x = int((image.width_ * u) + 0.5f);
		int y = int((image.height_ * v) + 0.5f);

		if (x >= image.width_) {
			x = image.width_ - 1;
		}

		if (y >= image.height_) {
			y = image.height_ - 1;
		}

		const byte_t* pixel = image.pixels_ + y * image.width_ * 4 + x * 4;

		return glm::uvec3(pixel[0], pixel[1], pixel[2]);
	};

	int vertex_count_per_edge = terrain.vertex_count_per_edge_;
	int z_count = vertex_count_per_edge * vertex_count_per_edge;

	float min_z = 9999999999.0f;
	float max_z = -9999999999.0f;

	for (int i = 0; i < z_count; ++i) {
		float z = terrain.heights_[i];
		if (z < min_z) {
			min_z = z;
		}
		if (z > max_z) {
			max_z = z;
		}
	}

	float step = (max_z - min_z) / 7.0f;

	tiles[0].filename_ = tile_images.lowest_;
	tiles[0].full_lower_ = min_z;
	
	tiles[1].filename_ = tile_images.low_;
	tiles[1].full_lower_ = min_z + step * 2.0f;

	tiles[2].filename_ = tile_images.high_;
	tiles[2].full_lower_ = min_z + step * 4.0f;

	tiles[3].filename_ = tile_images.hightest_;
	tiles[3].full_lower_ = min_z + step * 6.0f;

	for (int i = 0; i < 4; ++i) {
		tiles[i].full_upper_ = tiles[i].full_lower_ + step;
		tiles[i].vanish_lower_ = tiles[i].full_lower_ - step;
		tiles[i].vanish_upper_ = tiles[i].full_upper_ + step;
	}

	for (int i = 0; i < 4; ++i) {
		if (!Img_Load(tiles[i].filename_, tiles[i].image_)) {

			for (int i = 0; i < 4; ++i) {
				if (tiles[i].image_.pixels_) {
					Img_Free(tiles[i].image_);
				}
			}

			return false;
		}
	}

	if (!Img_Create(width, height, image_format_t::R8G8B8A8, texture)) {
		for (int i = 0; i < 4; ++i) {
			if (tiles[i].image_.pixels_) {
				Img_Free(tiles[i].image_);
			}
		}
		return false;
	}

	for (int row = 0; row < height; ++row) {
		float v = (float)row / (height - 1);
		float y = v * (vertex_count_per_edge - 1);
		for (int col = 0; col < width; ++col) {
			float u = (float)col / (width - 1);
			float x = u * (vertex_count_per_edge - 1);

			int ix = (int)x;
			int iy = (int)y;

			float z = terrain.heights_[iy * vertex_count_per_edge + ix];

			float percentage[4];
			glm::uvec3 pixels[4];

			for (int i = 0; i < 4; ++i) {
				percentage[i] = GetPrecentage(tiles[i], z);
				pixels[i] = GetPixel(tiles[i].image_, u, v);
			}

			glm::vec3 combined = glm::uvec3(0);

			for (int i = 0; i < 4; ++i) {
				combined.r += (float)pixels[i].r * percentage[i];
				combined.g += (float)pixels[i].g * percentage[i];
				combined.b += (float)pixels[i].b * percentage[i];
			}

			if (combined.r > 255.0f) {
				combined.r = 255.0f;
			}

			if (combined.g > 255.0f) {
				combined.g = 255.0f;
			}

			if (combined.b > 255.0f) {
				combined.b = 255.0f;
			}

			byte_t* dst_pixel = texture.pixels_ + row * width * 4 + col * 4;
			dst_pixel[0] = (byte_t)combined.r;
			dst_pixel[1] = (byte_t)combined.g;
			dst_pixel[2] = (byte_t)combined.b;
			dst_pixel[3] = 255;
		}
	}

	for (int i = 0; i < 4; ++i) {
		if (tiles[i].image_.pixels_) {
			Img_Free(tiles[i].image_);
		}
	}

	return true;
}

/*
================================================================================
model
================================================================================
*/

static bool Model_LoadPLY(const char* filename, model_s & model);
static bool Model_LoadObj(const char* filename, model_s & model);

COMMON_API bool Model_Load(const char* filename, 
	bool move_to_origin, model_s& model, const glm::mat4* transform)
{
	memset(&model, 0, sizeof(model));

	const char* ext = strrchr(filename, '.');
	if (!ext) {
		printf("Could not get filename extension.\n");
		return false;
	}

	bool load_ok = false;

	if (Str_ICmp(ext + 1, "ply") == 0) {
		load_ok = Model_LoadPLY(filename, model);
	}
	else if (Str_ICmp(ext + 1, "obj") == 0) {
		load_ok = Model_LoadObj(filename, model);
	}
	else {
		printf("Unsupported model file format %s.\n", ext + 1);
		return false;
	}
	
	if (!load_ok) {
		Model_Free(model);
		return false;
	}

	// calculate min max
	size_t stride = sizeof(glm::vec3);
	uint32_t normal_offset = 0;

	switch (model.vertex_format_) {
	case vertex_format_t::VF_POS:
		stride = sizeof(vertex_pos_s);
		break;
	case vertex_format_t::VF_POS_COLOR:
		stride = sizeof(vertex_pos_color_s);
		break;
	case vertex_format_t::VF_POS_NORMAL:
		stride = sizeof(vertex_pos_normal_s);
		normal_offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_);
		break;
	case vertex_format_t::VF_POS_UV:
		stride = sizeof(vertex_pos_uv_s);
		break;
	case vertex_format_t::VF_POS_NORMAL_COLOR:
		stride = sizeof(vertex_pos_normal_color_s);
		normal_offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV:
		stride = sizeof(vertex_pos_normal_uv_s);
		normal_offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_);
		break;
	case vertex_format_t::VF_POS_NORMAL_UV_TANGENT:
		stride = sizeof(vertex_pos_normal_uv_tangent_s);
		normal_offset = GET_FIELD_OFFSET(vertex_pos_normal_s, normal_);
		break;
	default:
		Model_Free(model);
		printf("bad vertex format\n");
		return false;
	}

	char* v = (char*)model.vertices_;

	float min_pos[3] = { 1.0e30f, 1.0e30f, 1.0e30f };
	float max_pos[3] = { -1.0e30f, -1.0e30f, -1.0e30f };

	for (uint32_t i = 0; i < model.num_vertex_; ++i) {
		glm::vec3* v_pos = (glm::vec3*)v;
		if (transform) {
			glm::vec4 v_new_pos = *transform * glm::vec4(*v_pos, 1.0f);
			v_pos->x = v_new_pos.x;
			v_pos->y = v_new_pos.y;
			v_pos->z = v_new_pos.z;

			if (normal_offset) {
				glm::vec3* v_normal = (glm::vec3*)(v + normal_offset);
				glm::vec4 v_new_normal = *transform * glm::vec4(*v_normal, 0.0f);
				v_normal->x = v_new_normal.x;
				v_normal->y = v_new_normal.y;
				v_normal->z = v_new_normal.z;
			}
		}

		float* pos = (float*)v_pos;

		for (int j = 0; j < 3; ++j) {
			if (pos[j] < min_pos[j]) {
				min_pos[j] = pos[j];
			}
			if (pos[j] > max_pos[j]) {
				max_pos[j] = pos[j];
			}
		}

		v += stride;
	}

	model.min_[0] = min_pos[0];
	model.min_[1] = min_pos[1];
	model.min_[2] = min_pos[2];

	model.max_[0] = max_pos[0];
	model.max_[1] = max_pos[1];
	model.max_[2] = max_pos[2];

	if (move_to_origin) {
		float ctr[3];

		for (int j = 0; j < 3; ++j) {
			ctr[j] = (min_pos[j] + max_pos[j]) * 0.5f;
		}

		v = (char*)model.vertices_;
		for (uint32_t i = 0; i < model.num_vertex_; ++i) {

			float* pos = (float*)v;

			pos[0] -= ctr[0];
			pos[1] -= ctr[1];
			pos[2] -= ctr[2];

			v += stride;
		}

		model.min_[0] -= ctr[0];
		model.min_[1] -= ctr[1];
		model.min_[2] -= ctr[2];

		model.max_[0] -= ctr[0];
		model.max_[1] -= ctr[1];
		model.max_[2] -= ctr[2];
	}

	return true;
}

COMMON_API void Model_Free(model_s& model) {
	SAFE_FREE(model.parts_);
	SAFE_FREE(model.materials_);
	SAFE_FREE(model.indices_);
	SAFE_FREE(model.vertices_);

	model.num_vertex_ = model.num_index_ = model.num_material_ = model.num_parts_ = 0;
}

static bool Model_LoadPLY(const char* filename, model_s& model) {
	memset(&model, 0, sizeof(model));

	PLY ply;
	if (!ply.Load(filename)) {
		return false;
	}

	const glm::vec3* pos_lst = ply.GetPos();
	const glm::vec3* normal_lst = ply.GetNormal();
	const glm::vec2* uv_lst = ply.GetUV();
	const glm::vec4* color_lst = ply.GetColor();

	uint32_t num_vertex = ply.NumberVertex();
	model.num_vertex_ = num_vertex;

	if (color_lst) {
		model.vertex_format_ = vertex_format_t::VF_POS_NORMAL_COLOR;

		vertex_pos_normal_color_s * buf = (vertex_pos_normal_color_s*)TEMP_ALLOC(sizeof(vertex_pos_normal_color_s) * num_vertex);
		if (!buf) {
			return false;
		}
		
		model.vertices_ = buf;

		for (uint32_t i = 0; i < num_vertex; ++i) {
			buf[i].pos_ = pos_lst[i];
			buf[i].normal_ = normal_lst[i];
			buf[i].color_ = color_lst[i];
		}
	}
	else if (uv_lst) {
		model.vertex_format_ = vertex_format_t::VF_POS_NORMAL_UV;

		vertex_pos_normal_uv_s * buf = (vertex_pos_normal_uv_s*)TEMP_ALLOC(sizeof(vertex_pos_normal_uv_s) * num_vertex);
		if (!buf) {
			return false;
		}

		model.vertices_ = buf;

		for (uint32_t i = 0; i < num_vertex; ++i) {
			buf[i].pos_ = pos_lst[i];
			buf[i].normal_ = normal_lst[i];
			buf[i].uv_ = uv_lst[i];
		}
	}
	else {
		model.vertex_format_ = vertex_format_t::VF_POS_NORMAL;

		vertex_pos_normal_s * buf = (vertex_pos_normal_s*)TEMP_ALLOC(sizeof(vertex_pos_normal_s) * num_vertex);
		if (!buf) {
			return false;
		}

		model.vertices_ = buf;

		for (uint32_t i = 0; i < num_vertex; ++i) {
			buf[i].pos_ = pos_lst[i];
			buf[i].normal_ = normal_lst[i];
		}
	}

	uint32_t num_triangle = ply.NumberTriangle();
	model.num_index_ = num_triangle * 3;

	model.indices_ = (uint32_t*)TEMP_ALLOC(sizeof(uint32_t) * num_triangle * 3);
	if (!model.indices_) {
		return false;
	}

	memcpy(model.indices_, ply.GetIndices(), sizeof(uint32_t) * num_triangle * 3);

	model.materials_ = nullptr; // no material
	model.num_material_ = 0;

	// one part
	model.parts_ = (model_part_s*)TEMP_ALLOC(sizeof(model_part_s));
	model.num_parts_ = 1;

	if (!model.parts_) {
		return false;
	}

	model.parts_->material_idx_ = VK_INVALID_INDEX;
	model.parts_->index_offset_ = 0;
	model.parts_->index_count_ = num_triangle * 3;

	return true;
}

static bool Model_LoadObj(const char* filename, model_s& model) {
	memset(&model, 0, sizeof(model));

	Obj obj;
	if (!obj.Load(filename)) {
		return false;
	}

	const glm::vec3* pos_lst = obj.GetPos();
	const glm::vec3* normal_lst = obj.GetNormal();
	const glm::vec2* uv_lst = obj.GetUV();
	const Obj::material_s* material_lst = obj.GetMaterial();
	const Obj::face_group_s* face_group = obj.GetFaceGroup();

	if (!pos_lst || !normal_lst || !face_group) {
		printf("Bad model \"%s\"\n", filename);
		return false;
	}

	uint32_t num_material = obj.NumberMaterial();
	uint32_t num_face_group = obj.NumFaceGroup();

	// allocate buffers

	uint32_t num_vertex = 0;
	uint32_t num_index = 0;
	for (uint32_t i = 0; i < num_face_group; ++i) {
		const Obj::face_group_s* fg = face_group + i;
		for (uint32_t j = 0; j < fg->face_count_; ++j) {
			const Obj::face_s* f = fg->faces_ + j;
			if (f->vertex_count_ == 3) {
				num_vertex += 3;
				num_index += 3;
			}
			else if (f->vertex_count_ == 4) {
				num_vertex += 4;
				num_index += 6;
			}
			// ignore other faces
		}
	}

	size_t vertex_size = 0;

	if (uv_lst) {
		model.vertex_format_ = vertex_format_t::VF_POS_NORMAL_UV;
		vertex_size = sizeof(vertex_pos_normal_uv_s);
	}
	else {
		model.vertex_format_ = vertex_format_t::VF_POS_NORMAL;
		vertex_size = sizeof(vertex_pos_normal_s);
	}

	model_material_s* materials = nullptr;
	if (num_material) {
		size_t materials_size = sizeof(model_material_s) * num_material;
		materials = (model_material_s*)TEMP_ALLOC(materials_size);
		if (!materials) {
			printf("Could not allocate materials\n");
			return false;
		}
	}

	model_part_s* parts = nullptr;
	if (num_face_group) {
		size_t parts_size = sizeof(model_part_s) * num_face_group;
		parts = (model_part_s*)TEMP_ALLOC(parts_size);
		if (!parts) {
			SAFE_FREE(materials);
			printf("Could not allocate parts\n");
			return false;
		}
	}

	size_t vertices_size = vertex_size * num_vertex;
	size_t indices_size = sizeof(uint32_t) * num_index;

	byte_t* vertices = (byte_t*)TEMP_ALLOC(vertices_size);
	uint32_t* indices = (uint32_t*)TEMP_ALLOC(indices_size);

	if (!vertices || !indices) {
		SAFE_FREE(indices);
		SAFE_FREE(vertices);
		SAFE_FREE(parts);
		SAFE_FREE(materials);
		return false;
	}

	model.vertices_ = vertices;
	model.indices_ = indices;
	model.materials_ = materials;
	model.parts_ = parts;
	model.num_vertex_ = num_vertex;
	model.num_index_ = num_index;
	model.num_material_ = num_material;
	model.num_parts_ = num_face_group;

	// start convert

	// convert material
	if (num_material) {
		char file_folder[MAX_PATH];
		Str_ExtractFileDir(filename, file_folder, MAX_PATH);

		for (uint32_t i = 0; i < num_material; ++i) {
			const Obj::material_s* src = material_lst + i;
			model_material_s* dst = materials + i;

			if (src->map_Kd_[0]) {
				Str_SPrintf(dst->tex_file_, MAX_PATH, "%s/%s", file_folder, src->map_Kd_);
			}
			else {
				dst->tex_file_[0] = 0;
			}

			dst->ambient_ = src->Ka_;
			dst->diffuse_ = src->Kd_;
			dst->specular_ = src->Ks_;
			dst->shiness_ = src->shiness_;
			dst->alpha_ = src->alpha_;
		}
	}

	// faces
	uint32_t written_vet = 0;
	uint32_t written_idx = 0;
	
	for (uint32_t i = 0; i < num_face_group; ++i) {
		const Obj::face_group_s* src_fg = face_group + i;
		model_part_s* dst_fg = parts + i;

		dst_fg->material_idx_ = src_fg->material_idx_;
		dst_fg->index_offset_ = written_idx;

		for (uint32_t j = 0; j < src_fg->face_count_; ++j) {
			const Obj::face_s* f = src_fg->faces_ + j;

			// copy vertex
			uint32_t face_vet0 = written_vet;

			for (uint32_t k = 0; k < f->vertex_count_; ++k) {
				const Obj::face_vertex_s* fv = f->vertices_ + k;

				if (uv_lst) {
					vertex_pos_normal_uv_s* dst_vertex = (vertex_pos_normal_uv_s*)(vertices + vertex_size * (face_vet0 + k));
					
					dst_vertex->pos_ = pos_lst[fv->pos_idx_];
					dst_vertex->normal_ = normal_lst[fv->normal_idx_];
					dst_vertex->uv_ = uv_lst[fv->uv_idx_];
				}
				else {
					vertex_pos_normal_s* dst_vertex = (vertex_pos_normal_s*)(vertices + vertex_size * (face_vet0 + k));

					dst_vertex->pos_ = pos_lst[fv->pos_idx_];
					dst_vertex->normal_ = normal_lst[fv->normal_idx_];
				}
			}

			written_vet += f->vertex_count_;

			// setup index
			if (f->vertex_count_ == 3) {
				indices[written_idx + 0] = face_vet0;
				indices[written_idx + 1] = face_vet0 + 1;
				indices[written_idx + 2] = face_vet0 + 2;

				written_idx += 3;
			}
			else if (f->vertex_count_ == 4) {
				indices[written_idx + 0] = face_vet0;
				indices[written_idx + 1] = face_vet0 + 1;
				indices[written_idx + 2] = face_vet0 + 2;

				indices[written_idx + 3] = face_vet0 + 2;
				indices[written_idx + 4] = face_vet0 + 3;
				indices[written_idx + 5] = face_vet0;

				written_idx += 6;
			}
			// ignore other faces
		}

		dst_fg->index_count_ = written_idx - dst_fg->index_offset_;
	}

	return true;
}

/*
================================================================================
helper
================================================================================
*/
COMMON_API void CalculateFrustumCorners(const frustum_s& frustum, glm::vec3 corners[8]) {
	float tan = std::tanf(glm::radians(frustum.fovy_ * 0.5f));

	float z_near = frustum.z_near_;
	float z_far = frustum.z_far_;

	float y_near = z_near * tan;
	float y_far = z_far * tan;

	float x_near = y_near * frustum.ratio_;
	float x_far = y_far * frustum.ratio_;

	corners[0] = glm::vec3(-x_near, -y_near, -z_near);
	corners[1] = glm::vec3( x_near, -y_near, -z_near);
	corners[2] = glm::vec3( x_near,  y_near, -z_near);
	corners[3] = glm::vec3(-x_near,  y_near, -z_near);

	corners[4] = glm::vec3(-x_far, -y_far, -z_far);
	corners[5] = glm::vec3( x_far, -y_far, -z_far);
	corners[6] = glm::vec3( x_far,  y_far, -z_far);
	corners[7] = glm::vec3(-x_far,  y_far, -z_far);

	glm::vec3 forward = glm::normalize(frustum.view_target_ - frustum.view_pos_);
	glm::vec3 right = glm::normalize(glm::cross(forward, frustum.view_up_));
	glm::vec3 up = glm::cross(right, forward);

#if 1
	glm::mat3 m(right, up, -forward);
#else
	// equal to

	glm::mat3 m;
	m[0].x = right.x;
	m[1].x = right.y;
	m[2].x = right.z;

	m[0].y = up.x;
	m[1].y = up.y;
	m[2].y = up.z;

	m[0].z = -forward.x;
	m[1].z = -forward.y;
	m[2].z = -forward.z;

	// invert
	m = glm::transpose(m);
#endif

	// convert from view space to world space
	for (int i = 0; i < 8; ++i) {
		corners[i] = m * corners[i];
		corners[i] += frustum.view_pos_;
	}
}

COMMON_API void	CalculateVec3ArrayCenter(const glm::vec3* arr, int array_sz, glm::vec3& center) {
	glm::vec3 sum = glm::vec3(0.0f);
	float inv = 1.0f / array_sz;

	for (int i = 0; i < array_sz; ++i) {
		sum += arr[i];
	}

	center = sum * inv;
}

COMMON_API float CalculateMaxDistToVec3Array(const glm::vec3* arr, int array_sz, const glm::vec3& pos) {
	float max_dist = 0.0f;
	
	for (int i = 0; i < 8; ++i) {
		glm::vec3 delta = pos - arr[i];
		float len = glm::length(delta);
		if (len > max_dist) {
			max_dist = len;
		}
	}

	return max_dist;
}

/*
================================================================================
vulkan helper
================================================================================
*/
COMMON_API const char* Vk_FormatToStr(VkFormat format) {
	switch (format) {
		RET_CASE_ID_TO_STR(VK_FORMAT_UNDEFINED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R4G4_UNORM_PACK8);
		RET_CASE_ID_TO_STR(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R5G6B5_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_B5G6R5_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R8G8B8A8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8A8_SRGB);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_UINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_SINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_UINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2R10G10B10_SINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_UINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_A2B10G10R10_SINT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_SNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_USCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_SSCALED);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R16G16B16A16_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32A32_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32A32_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R32G32B32A32_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64A64_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64A64_SINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_R64G64B64A64_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_D16_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_X8_D24_UNORM_PACK32);
		RET_CASE_ID_TO_STR(VK_FORMAT_D32_SFLOAT);
		RET_CASE_ID_TO_STR(VK_FORMAT_S8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_D16_UNORM_S8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_D24_UNORM_S8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC2_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC2_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC3_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC3_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC4_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC4_SNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC5_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC5_SNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC6H_UFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC6H_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC7_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_BC7_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_EAC_R11_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_EAC_R11_SNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8B8G8R8_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8G8_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_R10X6_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R10X6G10X6_UNORM_2PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R12X4_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R12X4G12X4_UNORM_2PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16B16G16R16_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_B16G16R16G16_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_A4R4G4B4_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_A4B4G4R4_UNORM_PACK16);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK);
		RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A1B5G5R5_UNORM_PACK16);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A8_UNORM);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
		RET_CASE_ID_TO_STR(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_SFIXED5_NV);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8B8G8R8_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_B8G8R8G8_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R10X6_UNORM_PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R12X4_UNORM_PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16B16G16R16_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_B16G16R16G16_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT);
		//RET_CASE_ID_TO_STR(VK_FORMAT_R16G16_S10_5_NV);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR);
		//RET_CASE_ID_TO_STR(VK_FORMAT_A8_UNORM_KHR);
	default:
		return "Unknow VkFormat";
	}
}

COMMON_API const char* Vk_PresentModelToStr(VkPresentModeKHR present_mode) {
	switch (present_mode) {
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_IMMEDIATE_KHR);
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_MAILBOX_KHR);
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_FIFO_KHR);
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR);
		RET_CASE_ID_TO_STR(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR);
		//RET_CASE_ID_TO_STR(VK_PRESENT_MODE_FIFO_LATEST_READY_EXT);
	default:
		return "Unknow VkPresentModeKHR";
	}
}

static void Vk_PushDescriptorSetLayoutBinding(std::vector<VkDescriptorSetLayoutBinding>& bindings,
	uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags) 
{
	bindings.push_back(
		{
			.binding = binding,	// the binding number of this entry and
			// corresponds to a resource of the same binding number in the shader stages.
			.descriptorType = descriptor_type,
			.descriptorCount = 1,	// non-array
			.stageFlags = stage_flags,	// specifying which pipeline shader stages can access a resource for this binding.
			.pImmutableSamplers = nullptr
		});
}

void Vk_PushDescriptorSetLayoutBinding_UBO(
	std::vector<VkDescriptorSetLayoutBinding>& bindings,
	uint32_t binding, VkShaderStageFlags stage_flags)
{
	Vk_PushDescriptorSetLayoutBinding(bindings, binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		stage_flags);
}

void Vk_PushDescriptorSetLayoutBinding_Tex(std::vector<VkDescriptorSetLayoutBinding>& bindings,
	uint32_t binding, VkShaderStageFlags stage_flags)
{
	Vk_PushDescriptorSetLayoutBinding(bindings, binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		stage_flags);
}

void Vk_PushDescriptorSetLayoutBinding_SBO(std::vector<VkDescriptorSetLayoutBinding>& bindings,
	uint32_t binding, VkShaderStageFlags stage_flags) 
{
	Vk_PushDescriptorSetLayoutBinding(bindings, binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		stage_flags);
}

void Vk_PushWriteDescriptorSet_UBO(
	update_desc_sets_buffer_s& buffer,
	VkDescriptorSet vk_desc_set, uint32_t binding,
	VkBuffer vk_buffer, VkDeviceSize vk_buffer_offset, VkDeviceSize vk_buffer_range)
{
	VkDescriptorBufferInfo* desc_buf_info = (VkDescriptorBufferInfo*)TEMP_ALLOC(sizeof(VkDescriptorBufferInfo));
	if (!desc_buf_info) {
		return;
	}

	desc_buf_info->buffer = vk_buffer;
	desc_buf_info->offset = vk_buffer_offset;
	desc_buf_info->range = vk_buffer_range;

	buffer.desc_buf_infos_.push_back(desc_buf_info);

	buffer.write_descriptor_sets_.push_back(
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = vk_desc_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = desc_buf_info,
			.pTexelBufferView = nullptr
		});
}

void Vk_PushWriteDescriptorSet_Tex(
	update_desc_sets_buffer_s& buffer,
	VkDescriptorSet vk_desc_set, uint32_t binding, const vk_image_s& texture)
{
	buffer.write_descriptor_sets_.push_back(
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = vk_desc_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &texture.desc_image_info_,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr
		});
}

COMMON_API void Vk_PushWriteDescriptorSet_SBO(
	update_desc_sets_buffer_s& buffer,
	VkDescriptorSet vk_desc_set, uint32_t binding,
	VkBuffer vk_buffer, VkDeviceSize vk_buffer_offset, VkDeviceSize vk_buffer_range)
{
	VkDescriptorBufferInfo* desc_buf_info = (VkDescriptorBufferInfo*)TEMP_ALLOC(sizeof(VkDescriptorBufferInfo));
	if (!desc_buf_info) {
		return;
	}

	desc_buf_info->buffer = vk_buffer;
	desc_buf_info->offset = vk_buffer_offset;
	desc_buf_info->range = vk_buffer_range;

	buffer.desc_buf_infos_.push_back(desc_buf_info);

	buffer.write_descriptor_sets_.push_back(
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = vk_desc_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = desc_buf_info,
			.pTexelBufferView = nullptr
		});
}
