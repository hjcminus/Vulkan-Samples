/******************************************************************************
 helper
 *****************************************************************************/

#include "funcs.h"

#if defined(PLATFORM_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

static wchar_t	g_data_folder[1024];

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
	GetModuleFileName(GetModuleHandle(NULL), g_data_folder, 1024);

	Str_ExtractFileDirSelf(g_data_folder);
	wcscat_s(g_data_folder, L"\\..\\..\\..\\data");

	Str_EraseDoubleDotsInPath(g_data_folder);
#endif
}

COMMON_API const wchar_t * GetDataFolder() {
	return g_data_folder;
}

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

// string
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
