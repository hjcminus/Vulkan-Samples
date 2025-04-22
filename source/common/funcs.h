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

// half float
using float16_t = uint16_t;

static_assert(sizeof(byte_t) == 1, "Bad size of byte_t");
static_assert(sizeof(word_t) == 2, "Bad size of word_t");
static_assert(sizeof(dword_t) == 4, "Bad size of dword_t");


#if defined(_WIN32)
# define KEY_F1				VK_F1
# define KEY_F2				VK_F2
# define KEY_F3				VK_F3
# define KEY_F4				VK_F4
# define KEY_F5				VK_F5
# define KEY_F6				VK_F6
# define KEY_F7				VK_F7
# define KEY_F8				VK_F8
# define KEY_F9				VK_F9
# define KEY_F10			VK_F10
# define KEY_F11			VK_F11
# define KEY_F12			VK_F12

# define KEY_PAGEUP			VK_PRIOR
# define KEY_PAGEDOWN		VK_NEXT
#endif

#if defined(__linux__)	// TODO: https://www.oreilly.com/library/view/xlib-reference-manual/9780937175262/16_appendix-h.html
# define KEY_F1				XK_F1
# define KEY_F2				XK_F2
# define KEY_F3				XK_F3	
# define KEY_F4				XK_F4	
# define KEY_F5				XK_F5	
# define KEY_F6				XK_F6	
# define KEY_F7				XK_F7	
# define KEY_F8				XK_F8	
# define KEY_F9				XK_F9	
# define KEY_F10			XK_F10	
# define KEY_F11			XK_F11	
# define KEY_F12			XK_F12

# define KEY_PAGEUP			XK_Prior
# define KEY_PAGEDOWN		XK_Next
#endif

/*
================================================================================
common
================================================================================
*/

// #define SQUARE(x)			(x) * (x)

#define	TEMP_ALLOC(sz)		malloc(sz)
#define TEMP_FREE(ptr)		free(ptr)

COMMON_API void				Common_Init();

COMMON_API const char *		GetDataFolder();
COMMON_API const char *		GetShadersFolder();

// s: struct
// f: field
#define GET_FIELD_OFFSET(s, f) (uint32_t)(&(((s*)0)->f))

#define COUNT_OF(a)		(sizeof(a) / sizeof(a[0]))

#define SAFE_FREE(ptr) do { if (ptr) { TEMP_FREE(ptr); ptr = nullptr; } } while (0)

// helper
#define RET_CASE_ID_TO_STR(ID)    case ID: return #ID

COMMON_API bool				IsBigEndian();

COMMON_API float			HalfFloatToFloat(float16_t h);
COMMON_API float16_t		FloatToHalfFloat(float f);

/*
================================================================================
random number generator
================================================================================
*/
// https://www.man7.org/linux/man-pages/man3/srand.3.html
COMMON_API void				SRand(uint32_t seed);
COMMON_API uint32_t			Rand();

// return [0, 1]
COMMON_API float			Rand01();

// return [-1, 1]
COMMON_API float			RandNeg1Pos1();

/*
================================================================================
file
================================================================================
*/

COMMON_API FILE *			File_Open(const char* filename, const char* mod);
COMMON_API bool				File_LoadBinary32(const char* filename, void*& data, int32_t& len);
COMMON_API void				File_FreeBinary(void* data);
COMMON_API bool				File_LoadText(const char* filename, char*& data, int32_t& len);
COMMON_API void				File_FreeText(char* data);

/*
================================================================================
string
================================================================================
*/
COMMON_API void				Str_Copy(char* dst, int dst_cap, const char* src);
COMMON_API void				Str_Cat(char* dst, int dst_cap, const char* src);
COMMON_API int				Str_ICmp(const char* s1, const char* s2);
COMMON_API void				Str_VSPrintf(char* dst, int dst_cap, const char* fmt, va_list argptr);
COMMON_API void				Str_SPrintf(char* dst, int dst_cap, const char* fmt, ...);
COMMON_API int				Str_UTF8ToUTF16(const char* utf8, char16_t* utf16, int utf16_chars);
COMMON_API int				Str_UTF8ToUTF32(const char* utf8, char32_t* utf32, int utf32_chars);
COMMON_API int				Str_UTF16ToUTF8(const char16_t* utf16, char* utf8, int utf8_bytes);
COMMON_API int				Str_UTF32ToUTF8(const char32_t* utf32, char* utf8, int utf8_bytes);

// return number tokens
COMMON_API char*			Str_SkipWhiteSpace(char* pc);
COMMON_API char*			Str_SkipCharactor(char* pc);
COMMON_API bool				Str_ExtractFileDir(const char* path, char* dir, int dir_cap);
COMMON_API bool				Str_ExtractFileName(const char* path, char* filename, int filename_cap);
COMMON_API bool				Str_ReplaceFileNameExt(char * path, int path_cap, const char * new_ext);
COMMON_API int				Str_Tokenize(const char* s, char** buffers, int each_buffer_size, int buffers_count);

// note: s will be changed
// return: item count
COMMON_API int				Str_Split(char* s, char delimiter, char** items, int max_item);

/*
================================================================================
Lexer
================================================================================
*/
class COMMON_API Lexer {
public:
	Lexer();
	~Lexer();

	void					SetPtr(const char * s);
	const char *			GetToken();
	void					SkipLine();
	const char *			ReadLine();

public:

	static const int		MAX_TOKEN_LEN = 4096;
	static const int		MAX_LINE_LEN = 4096;

	const char *			s_;	// string
	const char *			pc_;

	char					buffer_[MAX_TOKEN_LEN];
};

/*
================================================================================
color
================================================================================
*/
struct rgb_s {
	double					r_;	// [0-1]
	double					g_;	// [0-1]
	double					b_;	// [0-1]
};

struct hsv_s {
	double					h_;	// Hue: [0-360]
	double					s_;	// Saturation: [0-1]
	double					v_;	// Value: [0-1]
};

COMMON_API void				RGB2HSV(const rgb_s & in, hsv_s & out);
COMMON_API void				HSV2RGB(const hsv_s & in, rgb_s & out);

// t: [0-1], 0: start, 1 stop
COMMON_API void				RGBInterpolate(const rgb_s & start, const rgb_s & stop, double t, rgb_s& out);

/*
================================================================================
image
================================================================================
*/

enum class image_format_t : int32_t {
	R8G8B8A8,
	R16G16B16A16_FLOAT
	// ...
};

struct image_s {
	int						width_;
	int						height_;
	image_format_t			format_;
	byte_t*					pixels_;
};

COMMON_API bool				Img_Create(int width, int height, image_format_t fmt, image_s& image);
COMMON_API bool				Img_Load(const char * filename, image_s & image);
COMMON_API bool				Img_Save(const char* filename, const image_s& image);
COMMON_API void				Img_Free(image_s & image);

/*
================================================================================
terrain
================================================================================
*/

enum class terrain_gen_algorithm_t {
	FAULT_FORMATION,
	MID_POINT
};

enum class terrain_size_t : int {	// edge length of the squared terrain
	TS_32,
	TS_64,
	TS_128,
	TS_256,
	TS_512,
	TS_1K
};

struct terrain_gen_params_s {
	terrain_gen_algorithm_t algo_;
	terrain_size_t			sz_;
	float					min_z_;
	float					max_z_;
	int						iterations_;	// for fault-formation
	float					filter_;
	float					roughness_;		// for mid-point
};

struct terrain_s {
	int						vertex_count_per_edge_;
	float*					heights_;
};

COMMON_API uint32_t			Terrain_GetVertexCountPerEdge(terrain_size_t sz);
COMMON_API bool				Terrain_Generate(const terrain_gen_params_s & params, terrain_s& terrain);
COMMON_API void				Terrain_Free(terrain_s& terrain);

struct terrain_texture_tiles_s {
	char					lowest_[MAX_PATH];
	char					low_[MAX_PATH];
	char					high_[MAX_PATH];
	char					hightest_[MAX_PATH];
};

COMMON_API bool				Terrain_Texture(const terrain_texture_tiles_s & tile_images,
								const terrain_s& terrain, 
								int width, int height, image_s & texture);

/*
================================================================================
model
================================================================================
*/

// vertex

enum class vertex_format_t : int32_t {
	VF_POS,
	VF_POS_COLOR,
	VF_POS_NORMAL,
	VF_POS_UV,
	VF_POS_NORMAL_COLOR,
	VF_POS_NORMAL_UV,
	VF_POS_NORMAL_UV_TANGENT
};

struct vertex_pos_s {
	glm::vec3				pos_;
};

struct vertex_pos_color_s {
	glm::vec3				pos_;
	glm::vec4				color_;
};

struct vertex_pos_normal_s {
	glm::vec3				pos_;
	glm::vec3				normal_;
};

struct vertex_pos_uv_s {
	glm::vec3				pos_;
	glm::vec2				uv_;
};

struct vertex_pos_normal_color_s {
	glm::vec3				pos_;
	glm::vec3				normal_;
	glm::vec4				color_;
};

struct vertex_pos_normal_uv_s {
	glm::vec3				pos_;
	glm::vec3				normal_;
	glm::vec2				uv_;
};

struct vertex_pos_normal_uv_tangent_s {
	glm::vec3				pos_;
	glm::vec3				normal_;
	glm::vec2				uv_;
	glm::vec3				tangent_;
};

// instance

enum class instance_format_t : int32_t {
	INST_NONE,
	INST_POS_VEC3,
	INST_POS_VEC4
};

struct instance_pos_vec3_s {
	glm::vec3				pos_;
	glm::vec3				vec3_;
};

struct instance_pos_vec4_s {
	glm::vec3				pos_;
	glm::vec4				vec4_;
};

// model

struct model_material_s {
	char					tex_file_[MAX_PATH];	// full path
	glm::vec3				ambient_;
	glm::vec3				diffuse_;
	glm::vec3				specular_;
	float					shiness_;
	float					alpha_;
};

struct model_part_s {
	uint32_t				material_idx_;
	uint32_t				index_offset_;
	uint32_t				index_count_;
};

struct model_s {
	vertex_format_t			vertex_format_;
	void *					vertices_;
	uint32_t *				indices_;
	model_material_s *		materials_;
	model_part_s *			parts_;
	uint32_t				num_vertex_;
	uint32_t				num_index_;
	uint32_t				num_material_;
	uint32_t				num_parts_;
	float					min_[3];
	float					max_[3];
};

COMMON_API bool				Model_Load(const char* filename, bool move_to_origin, model_s & model, const glm::mat4 * transform = nullptr);
COMMON_API void				Model_Free(model_s& model);

/*
================================================================================
helper
================================================================================
*/
struct frustum_s {
	float					z_near_;
	float					z_far_;
	float					fovy_;
	float					ratio_;	// w / h
	glm::vec3				view_pos_;
	glm::vec3				view_target_;
	glm::vec3				view_up_;
};

COMMON_API void				CalculateFrustumCorners(const frustum_s & frustum, glm::vec3 corners[8]);
COMMON_API void				CalculateVec3ArrayCenter(const glm::vec3 * arr, int array_sz, glm::vec3 & center);
COMMON_API float			CalculateMaxDistToVec3Array(const glm::vec3* arr, int array_sz, const glm::vec3 & pos);

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
