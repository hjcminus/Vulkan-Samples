/******************************************************************************
 DirectDraw Surface file format
 *****************************************************************************/

#include "inc.h"
#include <dds.h>

static void PrintDDSInfo(const DDS_HEADER* head) {

	auto AddFlagStr = [](char s[1024], const char* flag_str) {
		if (s[0]) {
			Str_Cat(s, 1024, " | ");
		}

		Str_Cat(s, 1024, flag_str);
	};

	auto HeaderFlagsStr = [AddFlagStr](DWORD flags, char s[1024]) {
		s[0] = 0;

		if (flags & DDS_HEADER_FLAGS_TEXTURE) {
			AddFlagStr(s, "DDS_HEADER_FLAGS_TEXTURE");
		}

		if (flags & DDS_HEADER_FLAGS_MIPMAP) {
			AddFlagStr(s, "DDS_HEADER_FLAGS_MIPMAP");
		}

		if (flags & DDS_HEADER_FLAGS_VOLUME) {
			AddFlagStr(s, "DDS_HEADER_FLAGS_VOLUME");
		}

		if (flags & DDS_HEADER_FLAGS_PITCH) {
			AddFlagStr(s, "DDS_HEADER_FLAGS_PITCH");
		}

		if (flags & DDS_HEADER_FLAGS_LINEARSIZE) {
			AddFlagStr(s, "DDS_HEADER_FLAGS_LINEARSIZE");
		}
	};

	auto SurfaceFlagsStr = [AddFlagStr](DWORD flags, char s[1024]) {
		s[0] = 0;

		if (flags & DDS_SURFACE_FLAGS_TEXTURE) {
			AddFlagStr(s, "DDS_SURFACE_FLAGS_TEXTURE");
		}

		if (flags & DDS_SURFACE_FLAGS_MIPMAP) {
			AddFlagStr(s, "DDS_SURFACE_FLAGS_MIPMAP");
		}

		if (flags & DDS_SURFACE_FLAGS_CUBEMAP) {
			AddFlagStr(s, "DDS_SURFACE_FLAGS_CUBEMAP");
		}
	};

	auto CubemapFlagsStr = [](DWORD flags) -> const char* {
		switch (flags) {
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_POSITIVEX);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_NEGATIVEX);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_POSITIVEY);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_NEGATIVEY);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_POSITIVEZ);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_NEGATIVEZ);
			RET_CASE_ID_TO_STR(DDS_CUBEMAP_ALLFACES);
		default:
			return "N/A";
		}
	};

	auto PixelFormatFlagsStr = [AddFlagStr](DWORD flags, char s[1024]) {
		s[0] = 0;

		switch (flags) {
		case DDS_FOURCC:
			AddFlagStr(s, "DDS_FOURCC");
			break;
		case DDS_RGB:
			AddFlagStr(s, "DDS_RGB");
			break;
		case DDS_RGBA:
			AddFlagStr(s, "DDS_RGBA");
			break;
		case DDS_LUMINANCE:
			AddFlagStr(s, "DDS_LUMINANCE");
			break;
		case DDS_ALPHA:
			AddFlagStr(s, "DDS_ALPHA");
			break;
		default:

			if (flags & DDPF_ALPHAPIXELS) {
				AddFlagStr(s, "DDPF_ALPHAPIXELS");
			}

			if (flags & DDPF_ALPHA) {
				AddFlagStr(s, "DDPF_ALPHA");
			}

			if (flags & DDPF_FOURCC) {
				AddFlagStr(s, "DDPF_FOURCC");
			}

			if (flags & DDPF_RGB) {
				AddFlagStr(s, "DDPF_RGB");
			}

			if (flags & DDPF_YUV) {
				AddFlagStr(s, "DDPF_YUV");
			}

			if (flags & DDPF_LUMINANCE) {
				AddFlagStr(s, "DDPF_LUMINANCE");
			}

			break;
		}
	};

	auto PixelFormatStr = [](const DDS_PIXELFORMAT& fmt) -> const char* {
		if (!memcmp(&fmt, &DDSPF_DXT1, sizeof(DDS_PIXELFORMAT))) {
			return "DXT1 (DXGI_FORMAT_BC1_UNORM)";
		}
		else if (!memcmp(&fmt, &DDSPF_DXT2, sizeof(DDS_PIXELFORMAT))) {
			return "DXT2 (D3DFMT_DXT2)";
		}
		else if (!memcmp(&fmt, &DDSPF_DXT3, sizeof(DDS_PIXELFORMAT))) {
			return "DXT3 (DXGI_FORMAT_BC2_UNORM)";
		}
		else if (!memcmp(&fmt, &DDSPF_DXT4, sizeof(DDS_PIXELFORMAT))) {
			return "DXT4 (D3DFMT_DXT4)";
		}
		else if (!memcmp(&fmt, &DDSPF_DXT5, sizeof(DDS_PIXELFORMAT))) {
			return "DXT5 (DXGI_FORMAT_BC3_UNORM)";
		}
		else if (!memcmp(&fmt, &DDSPF_A8R8G8B8, sizeof(DDS_PIXELFORMAT))) {
			return "A8R8G8B8";
		}
		else if (!memcmp(&fmt, &DDSPF_A1R5G5B5, sizeof(DDS_PIXELFORMAT))) {
			return "A1R5G5B5";
		}
		else if (!memcmp(&fmt, &DDSPF_A4R4G4B4, sizeof(DDS_PIXELFORMAT))) {
			return "A4R4G4B4";
		}
		else if (!memcmp(&fmt, &DDSPF_R8G8B8, sizeof(DDS_PIXELFORMAT))) {
			return "R8G8B8";
		}
		else if (!memcmp(&fmt, &DDSPF_R5G6B5, sizeof(DDS_PIXELFORMAT))) {
			return "R5G6B5";
		}
		else if (!memcmp(&fmt, &DDSPF_DX10, sizeof(DDS_PIXELFORMAT))) {
			return "DX10 (Any DXGI format)";
		}
		else {
			if (fmt.dwFlags == DDS_FOURCC) {
				if (fmt.dwFourCC == MAKEFOURCC('B', 'C', '4', 'U')) {
					return "DXGI_FORMAT_BC4_UNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('B', 'C', '4', 'S')) {
					return "DXGI_FORMAT_BC4_SNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('A', 'T', 'I', '2')) {
					return "DXGI_FORMAT_BC5_UNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('B', 'C', '5', 'S')) {
					return "DXGI_FORMAT_BC5_SNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('R', 'G', 'B', 'G')) {
					return "DXGI_FORMAT_R8G8_B8G8_UNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('G', 'R', 'G', 'B')) {
					return "DXGI_FORMAT_G8R8_G8B8_UNORM";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y')) {
					return "D3DFMT_UYVY";
				}
				else if (fmt.dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2')) {
					return "D3DFMT_YUY2";
				}
				else if (fmt.dwFourCC == 36) {
					return "DXGI_FORMAT_R16G16B16A16_UNORM";
				}
				else if (fmt.dwFourCC == 110) {
					return "DXGI_FORMAT_R16G16B16A16_SNORM";
				}
				else if (fmt.dwFourCC == 111) {
					return "DXGI_FORMAT_R16_FLOAT";
				}
				else if (fmt.dwFourCC == 112) {
					return "DXGI_FORMAT_R16G16_FLOAT";
				}
				else if (fmt.dwFourCC == 113) {
					return "DXGI_FORMAT_R16G16B16A16_FLOAT";
				}
				else if (fmt.dwFourCC == 114) {
					return "DXGI_FORMAT_R32_FLOAT";
				}
				else if (fmt.dwFourCC == 115) {
					return "DXGI_FORMAT_R32G32_FLOAT";
				}
				else if (fmt.dwFourCC == 116) {
					return "DXGI_FORMAT_R32G32B32A32_FLOAT";
				}
				else if (fmt.dwFourCC == 117) {
					return "D3DFMT_CxV8U8";
				}
				else {
					return "<unknown>";
				}
			}
			else {
				return "<unknown>";
			}
		}
	};

	auto RGBBitCountValid = [](DWORD flags) {
		return flags & (DDPF_RGB | DDPF_LUMINANCE | DDPF_YUV);
	};

	char s[1024] = {};

	printf("dwSize = %u\n", head->dwSize);

	HeaderFlagsStr(head->dwHeaderFlags, s);
	printf("dwHeaderFlags = %s\n", s);
	printf("dwHeight = %u\n", head->dwHeight);
	printf("dwWidth = %u\n", head->dwWidth);
	printf("dwPitchOrLinearSize = %u\n", head->dwPitchOrLinearSize);
	printf("dwDepth = %u\n", head->dwDepth);
	printf("dwMipMapCount = %u\n", head->dwMipMapCount);

	printf("ddspf.dwSize = %u\n", head->ddspf.dwSize);

	PixelFormatFlagsStr(head->ddspf.dwFlags, s);

	printf("ddspf.dwFlags = %s\n", s);

	if (head->ddspf.dwFlags == DDS_FOURCC) {
		if (head->ddspf.dwFourCC <= 117) {	// special format
			printf("ddspf.dwFourCC = %u\n", head->ddspf.dwFourCC);
		}
		else {
			// four characters
			char four_cc[16] = {};
			memcpy(four_cc, &head->ddspf.dwFourCC, sizeof(head->ddspf.dwFourCC));
			printf("ddspf.dwFourCC = %s\n", four_cc);
		}
	}
	else {
		printf("ddspf.dwFourCC = N/A\n");
	}

	if (RGBBitCountValid(head->ddspf.dwFlags)) {
		printf("ddspf.dwRGBBitCount = %u\n", head->ddspf.dwRGBBitCount);
		printf("ddspf.dwRBitMask = 0x%08x\n", head->ddspf.dwRBitMask);
		printf("ddspf.dwGBitMask = 0x%08x\n", head->ddspf.dwGBitMask);
		printf("ddspf.dwBBitMask = 0x%08x\n", head->ddspf.dwBBitMask);
		printf("ddspf.dwABitMask = 0x%08x\n", head->ddspf.dwABitMask);
	}
	else {
		printf("ddspf.dwRGBBitCount = N/A\n");
		printf("ddspf.dwRBitMask = N/A\n");
		printf("ddspf.dwGBitMask = N/A\n");
		printf("ddspf.dwBBitMask = N/A\n");
		printf("ddspf.dwABitMask = N/A\n");
	}

	printf("ddspf = %s\n", PixelFormatStr(head->ddspf));

	SurfaceFlagsStr(head->dwSurfaceFlags, s);
	printf("dwSurfaceFlags = %s\n", s);

	printf("dwCubemapFlags = %s\n", CubemapFlagsStr(head->dwCubemapFlags));
}

/*

	cube map

	Y
	|
	|_______________________________
	|       |       |               |
	|       | pic 2 |               |
	|_______|_______|_______________|
	|       |       |       |       |
	| pic 1 | pic 4 | pic 0 | pic 5 |
	|_______|_______|_______|_______|
	|       |       |               |
	|       | pic 3 |               |
	|_______|_______|_______________|___X


 */
static const uint32_t CUBEMAP_OFFSET_X[] = { 2, 0, 1, 1, 1, 3 };
static const uint32_t CUBEMAP_OFFSET_Y[] = { 1, 1, 2, 0, 1, 1 };

static const size_t SIZEOF_HEAD = sizeof(DDS_HEADER);

bool Image_LoadDDS(const char* filename, image_s& image) {
#pragma pack(push, 1)
	union bc1_clr_s {
		uint16_t		word_;
		struct {
			uint16_t	b_ : 5;
			uint16_t	g_ : 6;
			uint16_t	r_ : 5;
		};
	};

	struct bc1_clr_idx_s {
		uint32_t	a_ : 2;
		uint32_t	b_ : 2;
		uint32_t	c_ : 2;
		uint32_t	d_ : 2;
		uint32_t	e_ : 2;
		uint32_t	f_ : 2;
		uint32_t	g_ : 2;
		uint32_t	h_ : 2;
		uint32_t	i_ : 2;
		uint32_t	j_ : 2;
		uint32_t	k_ : 2;
		uint32_t	l_ : 2;
		uint32_t	m_ : 2;
		uint32_t	n_ : 2;
		uint32_t	o_ : 2;
		uint32_t	p_ : 2;
	};

	struct bc1_s {
		bc1_clr_s	colors_[2];
		bc1_clr_idx_s	idx_;
	};
#pragma pack(pop)

	struct rgba32_s {
		byte_t		r_;
		byte_t		g_;
		byte_t		b_;
		byte_t		a_;
	};

	// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression?source=recommendations
	// https://learn.microsoft.com/en-us/windows/win32/direct3d9/opaque-and-1-bit-alpha-textures
	/*
	
	The 1-bit alpha format is distinguished from the opaque format by comparing the two 16-bit color 
	values stored in the block. They are treated as unsigned integers. If the first color is greater 
	than the second, it implies that only opaque texels are defined. This means four colors are used 
	to represent the texels. In four-color encoding, there are two derived colors and all four colors 
	are equally distributed in RGB color space. This format is analogous to RGB 5:6:5 format. Otherwise, 
	for 1-bit alpha transparency, three colors are used and the fourth is reserved to represent transparent 
	texels.

	 */
	
	auto Load_DDSPF_DXT1 = [](DDS_HEADER* head, image_s& image) -> bool {

		auto CopyPixels = [head](const byte_t * src_pixels, byte_t * dst_pixels, uint32_t dst_pic_w, uint32_t dst_pic_h) {
			if (head->dwWidth >= 4 && head->dwHeight >= 4) {

				const bc1_s* src_block = (const bc1_s*)src_pixels;

				uint32_t x_blocks = head->dwWidth >> 2;
				uint32_t y_blocks = head->dwHeight >> 2;

				for (uint32_t by = 0; by < y_blocks; ++by) {
					for (uint32_t bx = 0; bx < x_blocks; ++bx) {
						int src_x = bx * 4;
						int src_y = by * 4;
						int dst_y = head->dwHeight - src_y - 1;
						byte_t* dst_pixels_block = dst_pixels + dst_y * dst_pic_w * 4 + src_x * 4;

						// https://www.fsdeveloper.com/wiki/index.php?title=DXT_compression_explained
						rgba32_s color_table[4];

						//bool _1_bit_alpha_transparency = src_block->colors_[0].word_ <= src_block->colors_[1].word_;

						color_table[0].r_ = (byte_t)((src_block->colors_[0].r_ / 31.0f) * 255.0f);
						color_table[0].g_ = (byte_t)((src_block->colors_[0].g_ / 63.0f) * 255.0f);
						color_table[0].b_ = (byte_t)((src_block->colors_[0].b_ / 31.0f) * 255.0f);
						color_table[0].a_ = 255;

						color_table[1].r_ = (byte_t)((src_block->colors_[1].r_ / 31.0f) * 255.0f);
						color_table[1].g_ = (byte_t)((src_block->colors_[1].g_ / 63.0f) * 255.0f);
						color_table[1].b_ = (byte_t)((src_block->colors_[1].b_ / 31.0f) * 255.0f);
						color_table[1].a_ = 255;

						// interpolate
						float delta_r = (float)color_table[3].r_ - (float)color_table[0].r_;
						float delta_g = (float)color_table[3].g_ - (float)color_table[0].g_;
						float delta_b = (float)color_table[3].b_ - (float)color_table[0].b_;

						color_table[2].r_ = (byte_t)(color_table[0].r_ * 0.666667f + color_table[1].r_ * 0.333333f);
						color_table[2].g_ = (byte_t)(color_table[0].g_ * 0.666667f + color_table[1].g_ * 0.333333f);
						color_table[2].b_ = (byte_t)(color_table[0].b_ * 0.666667f + color_table[1].b_ * 0.333333f);
						color_table[2].a_ = 255;

						/*
						if (_1_bit_alpha_transparency) {
							// transparency texels
							color_table[3].r_ = 0;
							color_table[3].g_ = 0;
							color_table[3].b_ = 0;
							color_table[3].a_ = 0;
						}
						else {
							color_table[3].r_ = (byte_t)(color_table[0].r_ * 0.333333f + color_table[1].r_ * 0.666667f);
							color_table[3].g_ = (byte_t)(color_table[0].g_ * 0.333333f + color_table[1].g_ * 0.666667f);
							color_table[3].b_ = (byte_t)(color_table[0].b_ * 0.333333f + color_table[1].b_ * 0.666667f);
							color_table[3].a_ = 255;
						}
						*/

						color_table[3].r_ = (byte_t)(color_table[0].r_ * 0.333333f + color_table[1].r_ * 0.666667f);
						color_table[3].g_ = (byte_t)(color_table[0].g_ * 0.333333f + color_table[1].g_ * 0.666667f);
						color_table[3].b_ = (byte_t)(color_table[0].b_ * 0.333333f + color_table[1].b_ * 0.666667f);
						color_table[3].a_ = 255;

						/*

						 block:

						 | d c b a |
						 | h g f e |
						 | l k j i |
						 | p o n m |

						*/

						uint32_t idx[16] = {
							src_block->idx_.a_,
							src_block->idx_.b_,
							src_block->idx_.c_,
							src_block->idx_.d_,
							src_block->idx_.e_,
							src_block->idx_.f_,
							src_block->idx_.g_,
							src_block->idx_.h_,
							src_block->idx_.i_,
							src_block->idx_.j_,
							src_block->idx_.k_,
							src_block->idx_.l_,
							src_block->idx_.m_,
							src_block->idx_.n_,
							src_block->idx_.o_,
							src_block->idx_.p_,
						};

						for (int i = 0; i < 16; ++i) {
							int x = 4 - i % 4 - 1;
							int y = i / 4;

							byte_t* dst_pixel = dst_pixels_block - y * dst_pic_w * 4 + x * 4;

							int colr_idx = idx[i];
							dst_pixel[0] = color_table[colr_idx].r_;
							dst_pixel[1] = color_table[colr_idx].g_;
							dst_pixel[2] = color_table[colr_idx].b_;
							dst_pixel[3] = color_table[colr_idx].a_;
						}

						src_block++;
					}
				}
			}
			else {
				for (uint32_t y = 0; y < head->dwHeight; ++y) {
					const byte_t* src_pixel = (const byte_t*)(src_pixels + y * head->dwWidth * 4);
					byte_t* dst_pixel = dst_pixels + y * dst_pic_w * 4;

					// TO FIX
					for (uint32_t x = 0; x < head->dwWidth; ++x) {
						dst_pixel[0] = src_pixel[2];
						dst_pixel[1] = src_pixel[1];
						dst_pixel[2] = src_pixel[0];
						dst_pixel[3] = src_pixel[3];

						src_pixel += 4;
						dst_pixel += 4;
					}
				}
			}
		};

		bool ok = false;

		// (head->dwWidth * head->dwHeight) / 16 * 8
		//   16 pixels per block, 8 bytes per block
		uint32_t total_bytes_per_pic = (head->dwWidth * head->dwHeight) >> 1;
		if (head->dwWidth < 4 || head->dwHeight < 4) {
			total_bytes_per_pic = head->dwWidth * head->dwHeight * 4;
		}

		bool mipmap_error = false;
		if (head->dwSurfaceFlags & DDS_SURFACE_FLAGS_MIPMAP) {
			uint32_t mip_map_count = head->dwMipMapCount;
			uint32_t w = head->dwWidth;
			uint32_t h = head->dwHeight;

			for (uint32_t i = 1; i < mip_map_count; ++i) {
				w >>= 1;
				h >>= 1;

				if (!w || !h) {
					mipmap_error = true;
					break;
				}

				if (w < 4 || h < 4) {
					if (w >= 2 && h >= 2) {
						total_bytes_per_pic += w * h * 4;
					}
				}
				else {
					total_bytes_per_pic += w * h / 2;
				}
			}
		}

		if (mipmap_error) {
			printf("Bad mipmap\n");
			return false;
		}

		const byte_t* src_pixels = (const byte_t*)(head + 1);

		// use mipmap level 0 only
		if (head->dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) {
			if (head->dwWidth != head->dwHeight) {
				printf("cubemap width not equal to height\n");
			}
			else {
				if (head->dwCubemapFlags == DDS_CUBEMAP_ALLFACES) {
					uint32_t dst_w = head->dwWidth * 4;
					uint32_t dst_h = head->dwHeight * 3;

					uint32_t num_dst_pixels = dst_w * dst_h;

					byte_t* dst_pixels = (byte_t*)TEMP_ALLOC(4 * num_dst_pixels);
					if (!dst_pixels) {
						printf("Memory overflow\n");
					}
					else {
						// fill zero
						memset(dst_pixels, 0, num_dst_pixels * 4);

						image.width_ = (int)dst_w;
						image.height_ = (int)dst_h;
						image.format_ = image_format_t::R8G8B8A8;
						image.pixels_ = dst_pixels;

						auto CopyPicture = [head, dst_w, dst_h, dst_pixels, CopyPixels](const byte_t* src,
							uint32_t dst_offset_x, uint32_t dst_offset_y)
							{
								byte_t* dst = dst_pixels + (dst_offset_y * dst_w + dst_offset_x) * 4;
								CopyPixels(src, dst, dst_w, dst_h);
							};

						for (int i = 0; i < 6; ++i) {
							CopyPicture(
								src_pixels + i * total_bytes_per_pic,
								CUBEMAP_OFFSET_X[i] * head->dwWidth,
								CUBEMAP_OFFSET_Y[i] * head->dwHeight);
						}

						ok = true;
					}

				}
				else {
					printf("Only support DDS_CUBEMAP_ALLFACES cubemap mode\n");
				}
			}
		}
		else {
			// not a cubemap
			uint32_t num_dst_pixels = head->dwWidth * head->dwHeight;

			byte_t* dst_pixels = (byte_t*)TEMP_ALLOC(num_dst_pixels * 4);
			if (!dst_pixels) {
				printf("Memory overflow\n");
			}
			else {
				image.width_ = (int)head->dwWidth;
				image.height_ = (int)head->dwHeight;
				image.format_ = image_format_t::R8G8B8A8;
				image.pixels_ = dst_pixels;

				CopyPixels(src_pixels, dst_pixels, head->dwWidth, head->dwHeight);

				ok = true;
			}
		}

		return ok;
	};

	auto Load_DDSPF_A8R8G8B8 = [](DDS_HEADER* head, image_s& image) -> bool {
		bool ok = false;

		uint32_t total_pixels_per_pic = head->dwWidth * head->dwHeight;

		bool mipmap_error = false;
		if (head->dwSurfaceFlags & DDS_SURFACE_FLAGS_MIPMAP) {
			uint32_t mip_map_count = head->dwMipMapCount;
			uint32_t w = head->dwWidth;
			uint32_t h = head->dwHeight;

			for (uint32_t i = 1; i < mip_map_count; ++i) {
				w >>= 1;
				h >>= 1;

				if (!w || !h) {
					mipmap_error = true;
					break;
				}

				total_pixels_per_pic += (w * h);
			}
		}

		if (mipmap_error) {
			printf("Bad mipmap\n");
			return false;
		}

		const byte_t* src_pixels = (const byte_t*)(head + 1);

		// use mipmap level 0 only
		if (head->dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) {
			if (head->dwWidth != head->dwHeight) {
				printf("cubemap width not equal to height\n");
			}
			else {
				if (head->dwCubemapFlags == DDS_CUBEMAP_ALLFACES) {
					uint32_t dst_w = head->dwWidth * 4;
					uint32_t dst_h = head->dwHeight * 3;

					uint32_t num_dst_pixels = dst_w * dst_h;

					byte_t* dst_pixels = (byte_t*)TEMP_ALLOC(num_dst_pixels * 4);
					if (!dst_pixels) {
						printf("Memory overflow\n");
					}
					else {
						// fill zero
						memset(dst_pixels, 0, num_dst_pixels * 4);

						image.width_ = (int)dst_w;
						image.height_ = (int)dst_h;
						image.format_ = image_format_t::R8G8B8A8;
						image.pixels_ = dst_pixels;

						auto CopyPicture = [head, dst_w, dst_pixels](const byte_t* src,
							uint32_t dst_offset_x, uint32_t dst_offset_y)
						{
							byte_t* dst = dst_pixels + (dst_offset_y * dst_w + dst_offset_x) * 4;

							for (uint32_t row = 0; row < head->dwHeight; ++row) {
								const byte_t* src_row = src + row * head->dwWidth * 4;
								byte_t* dst_row = dst + (head->dwHeight - row - 1) * dst_w * 4;	// revert

								for (uint32_t col = 0; col < head->dwWidth; ++col) {
									const byte_t* src_p = src_row + col * 4;
									byte_t* dst_p = dst_row + col * 4;

									dst_p[0] = src_p[2];	// R
									dst_p[1] = src_p[1];	// G
									dst_p[2] = src_p[0];	// B
									dst_p[3] = src_p[3];	// A
								}
							}

						};

						for (int i = 0; i < 6; ++i) {
							CopyPicture(
								src_pixels + 4 * i * total_pixels_per_pic,
								CUBEMAP_OFFSET_X[i] * head->dwWidth,
								CUBEMAP_OFFSET_Y[i] * head->dwHeight);
						}

						ok = true;
					}

				}
				else {
					printf("Only support DDS_CUBEMAP_ALLFACES cubemap mode\n");
				}
			}
		}
		else {
			// not a cubemap
			uint32_t num_dst_pixels = head->dwWidth * head->dwHeight;

			byte_t* dst_pixels = (byte_t*)TEMP_ALLOC(num_dst_pixels * 4);
			if (!dst_pixels) {
				printf("Memory overflow\n");
			}
			else {
				image.width_ = (int)head->dwWidth;
				image.height_ = (int)head->dwHeight;
				image.format_ = image_format_t::R8G8B8A8;
				image.pixels_ = dst_pixels;

				const byte_t* src_pixel = src_pixels;
				byte_t* dst_pixel = dst_pixels;

				for (uint32_t i = 0; i < num_dst_pixels; ++i) {
					dst_pixel[0] = src_pixel[2];	// R
					dst_pixel[1] = src_pixel[1];	// G
					dst_pixel[2] = src_pixel[0];	// B
					dst_pixel[3] = src_pixel[3];	// A

					src_pixel += 4;
					dst_pixel += 4;
				}

				ok = true;
			}
		}

		return ok;
	};

	auto Load_DDS_FOURCC_113 = [](DDS_HEADER* head, image_s& image) -> bool {
		// DXGI_FORMAT_R16G16B16A16_FLOAT

		bool ok = false;

		const float16_t * src_pixels = (const float16_t*)(head + 1);

		uint32_t num_dst_pixels = head->dwWidth * head->dwHeight;

		float16_t* dst_pixels = (float16_t*)TEMP_ALLOC(num_dst_pixels * 8);
		if (!dst_pixels) {
			printf("Memory overflow\n");
		}
		else {
			image.width_ = (int)head->dwWidth;
			image.height_ = (int)head->dwHeight;
			image.format_ = image_format_t::R16G16B16A16_FLOAT;
			image.pixels_ = (byte_t*)dst_pixels;

			for (uint32_t y = 0; y < head->dwHeight; ++y) {
				const float16_t* src_line = src_pixels + y * head->dwWidth * 4;
				float16_t* dst_line = dst_pixels + (head->dwHeight - y - 1) * head->dwWidth * 4;

				for (uint32_t x = 0; x < head->dwWidth; ++x) {
					const float16_t* src_p = src_line + x * 4;
					float16_t* dst_p = dst_line + x * 4;

					dst_p[0] = src_p[0];	// R
					dst_p[1] = src_p[1];	// G
					dst_p[2] = src_p[2];	// B
					dst_p[3] = src_p[3];	// A
				}
			}

			ok = true;
		}

		return ok;
	};

	memset(&image, 0, sizeof(image));

	void* buffer = nullptr;
	int32_t file_size = 0;
	if (!File_LoadBinary32(filename, buffer, file_size)) {
		printf("Failed to load file \"%s\".\n", filename);
		return false;
	}

	DWORD* magic = (DWORD*)buffer;
	if (*magic != DDS_MAGIC) {
		printf("Bad dds file\n");
		File_FreeBinary(buffer);
		return false;
	}

	DDS_HEADER * head = (DDS_HEADER*)(magic + 1);

	bool ok = false;

	if (head->dwSurfaceFlags & DDS_SURFACE_FLAGS_TEXTURE) {
		if (memcmp(&head->ddspf, &DDSPF_DXT1, sizeof(head->ddspf)) == 0) {
			ok = Load_DDSPF_DXT1(head, image);
		}
		else if (memcmp(&head->ddspf, &DDSPF_A8R8G8B8, sizeof(head->ddspf)) == 0) {
			ok = Load_DDSPF_A8R8G8B8(head, image);
		}
		else if (head->ddspf.dwFlags == DDS_FOURCC && head->ddspf.dwFourCC == 113) {
			ok = Load_DDS_FOURCC_113(head, image);
		}
		else {
			printf("-- Not supported dds file format --\n");
			PrintDDSInfo(head);
		}
	}
	else {
		printf("DDS_SURFACE_FLAGS_TEXTURE not present\n");
	}

	File_FreeBinary(buffer);

	return ok;
}
