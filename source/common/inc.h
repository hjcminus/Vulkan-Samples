/******************************************************************************
 precompiled head
 *****************************************************************************/

#pragma once

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

#if defined(PLATFORM_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# define VK_USE_PLATFORM_WIN32_KHR		// to use VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

// stdlib
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>

// stl
#include <vector>
#include <array>

 // vulkan
#include <vulkan/vulkan.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// commonlib export
#include "funcs.h"
#include "ply.h"
#include "vk_demo.h"
#include "vk_funcs.h"
