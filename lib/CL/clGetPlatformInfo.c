/* OpenCL runtime library: clGetPlatformInfo()

   Copyright (c) 2011 Kalle Raiskila

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_util.h"

static const char *pocl_version
    = "OpenCL " POCL_CL_VERSION " pocl " POCL_VERSION_FULL

#if defined(_WIN32)
   #if defined(_WIN64)
      "  Windows x86-64"
   #else
      "  Windows x86"
   #endif
#elif defined(__ANDROID__)
      "  Android"
#elif defined(__APPLE__)
      "  Apple"
#elif defined(__linux__)
      "  Linux"
#elif defined(__unix__) // all unices not caught above
      "  Unix"
#else
      "  (Unknown OS)"
#endif
      ", " CMAKE_BUILD_TYPE
#ifdef POCL_ASSERTS_BUILD
      "+Asserts"
#endif

#ifdef ENABLE_RELOCATION
      ", RELOC"
#endif

#ifdef ENABLE_SPIR
      ", SPIR"
#endif

#ifdef ENABLE_SPIRV
      ", SPIR-V"
#endif

#ifdef ENABLE_LLVM
      ", LLVM " LLVM_VERSION
#ifdef LLVM_BUILD_MODE_DEBUG
      " - debug"
#endif

#ifdef ENABLE_SLEEF
      ", SLEEF"
#endif

#ifndef _CL_DISABLE_HALF
      ", FP16"
#endif

#ifdef KERNELLIB_HOST_DISTRO_VARIANTS
      ", DISTRO"
#endif

#else
      ", without LLVM"
#endif

#ifdef ENABLE_ASAN
      ", ASAN"
#endif
#ifdef ENABLE_TSAN
      ", TSAN"
#endif
#ifdef ENABLE_LSAN
      ", LSAN"
#endif
#ifdef ENABLE_UBSAN
      ", UBSAN"
#endif

#ifdef BUILD_PROXY
      ", PROXY"
#endif

#ifdef BUILD_CUDA
      ", CUDA"
#endif

#ifdef BUILD_HSA
      ", HSA"
#endif

#ifdef TCE_AVAILABLE
      ", TCE"
#endif

#ifdef HAVE_LTTNG_UST
      ", LTTNG"
#endif

#ifdef POCL_DEBUG_MESSAGES
      ", POCL_DEBUG"
#endif
    ;

CL_API_ENTRY cl_int CL_API_CALL
POname(clGetPlatformInfo)(cl_platform_id   platform,
                  cl_platform_info param_name,
                  size_t           param_value_size,
                  void *           param_value,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_platform_id tmp_platform;

  POCL_RETURN_ERROR_COND ((platform == NULL), CL_INVALID_PLATFORM);

  POname (clGetPlatformIDs) (1, &tmp_platform, NULL);
  POCL_RETURN_ERROR_ON ((platform != tmp_platform), CL_INVALID_PLATFORM,
                        "Can only return info about the POCL platform\n");

  switch (param_name)
    {
    case CL_PLATFORM_PROFILE:
      // TODO: figure this out depending on the native execution host.
      // assume FULL_PROFILE for now.
      POCL_RETURN_GETINFO_STR ("FULL_PROFILE");

    case CL_PLATFORM_VERSION:

      POCL_RETURN_GETINFO_STR (pocl_version);

    case CL_PLATFORM_NAME:
      POCL_RETURN_GETINFO_STR("Portable Computing Language");

    case CL_PLATFORM_VENDOR:
      POCL_RETURN_GETINFO_STR("The pocl project");

    case CL_PLATFORM_EXTENSIONS:
      POCL_RETURN_GETINFO_STR (
#ifdef BUILD_ICD
          "cl_khr_icd"
#endif
          " cl_pocl_content_size");

    case CL_PLATFORM_ICD_SUFFIX_KHR:
      POCL_RETURN_GETINFO_STR("POCL");

    case CL_PLATFORM_HOST_TIMER_RESOLUTION:
      POCL_RETURN_GETINFO(cl_ulong, 0);

    default:
      return CL_INVALID_VALUE;
  }
}
POsym(clGetPlatformInfo)
