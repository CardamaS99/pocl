/* OpenCL runtime library: clCreateProgramWithIL()

   Copyright (c) 2019 pocl developers

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.
*/

#include "pocl_cache.h"
#include "pocl_cl.h"
#include "pocl_file_util.h"
#include "pocl_llvm.h"
#include "pocl_util.h"
#include "pocl_shared.h"

CL_API_ENTRY cl_program CL_API_CALL
POname(clCreateProgramWithIL)(cl_context context,
                              const void *il,
                              size_t length,
                              cl_int *errcode_ret)
CL_API_SUFFIX__VERSION_2_1
{
  /* SPIR is disabled when building with conformance */
#ifdef ENABLE_CONFORMANCE
  if (errcode_ret)
    *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
#endif
  cl_program program = NULL;
  int errcode = CL_SUCCESS;
#ifdef ENABLE_SPIRV
  uint64_t converted_spir_file_size = 0;
  char *converted_spir_file = NULL;
#endif

  POCL_GOTO_ERROR_COND ((context == NULL), CL_INVALID_CONTEXT);

  POCL_GOTO_ERROR_COND ((il == NULL), CL_INVALID_VALUE);

  POCL_GOTO_ERROR_COND ((length == 0), CL_INVALID_VALUE);

  int is_spirv = 0;
#ifdef ENABLE_SPIRV
  int is_spirv_kernel
      = pocl_bitcode_is_spirv_execmodel_kernel ((const char *)il, length);
  is_spirv += is_spirv_kernel;
#endif
#ifdef ENABLE_VULKAN
  int is_spirv_shader
      = pocl_bitcode_is_spirv_execmodel_shader ((const char *)il, length);
  is_spirv += is_spirv_shader;
#endif

  POCL_GOTO_ERROR_ON (
      (!is_spirv), CL_INVALID_VALUE,
      "The IL provided to clCreateProgramWithIL "
      "is not recognized as SPIR-V!\n");

#ifdef ENABLE_SPIRV
  if (is_spirv_kernel)
    {
      /* convert and the SPIR-V to LLVM IR with spir triple */
      POCL_MSG_PRINT_LLVM (
          "SPIR-V binary detected, converting to LLVM SPIR\n");

      char program_bc_spirv[POCL_FILENAME_LENGTH];
      char program_bc_temp[POCL_FILENAME_LENGTH];
      pocl_cache_write_spirv (program_bc_spirv, (const char *)il,
                              (uint64_t)length);
      pocl_cache_tempname (program_bc_temp, ".bc", NULL);

      char *args[] = { LLVM_SPIRV,       "-r", "-o", program_bc_temp,
                       program_bc_spirv, NULL };

      errcode = pocl_run_command (args);
      POCL_GOTO_ERROR_ON (
          (errcode != 0), CL_INVALID_VALUE,
          "External command (llvm-spirv translator) failed!\n");

      /* load LLVM SPIR binary. */
      pocl_read_file (program_bc_temp, &converted_spir_file,
                      &converted_spir_file_size);
      POCL_GOTO_ERROR_ON ((converted_spir_file == NULL), CL_INVALID_VALUE,
                          "Can't read converted bitcode file\n");
      pocl_remove (program_bc_temp);
    }
#endif

  /* TODO should we create a program for all devices ?
   * should we fail if we can't create for all devices ?
   * this seems to be unspecified.
   * right now, we create only for devices
   * which have supports_binary() callback
   * and fail if there's no such device in context. */
  unsigned num = context->num_devices;
  unsigned num_devices_with_spir = 0;

  cl_device_id *devices_with_spir
      = (cl_device_id *)alloca (num * sizeof (cl_device_id));
  size_t *devices_with_spir_lens = (size_t *)alloca (num * sizeof (size_t));
  const unsigned char **devices_with_spir_bins
      = (const unsigned char **)alloca (num * sizeof (unsigned char *));

  for (unsigned i = 0; i < num; ++i)
    {
      cl_device_id dev = context->devices[i];
      if (dev->ops->supports_binary == NULL)
        continue;
      if (dev->ops->supports_binary (dev, length, il))
        {
          devices_with_spir[num_devices_with_spir] = dev;
          devices_with_spir_lens[num_devices_with_spir] = length;
          devices_with_spir_bins[num_devices_with_spir] = il;
          ++num_devices_with_spir;
        }
#ifdef ENABLE_SPIRV
      else if (dev->ops->supports_binary (dev, converted_spir_file_size,
                                          converted_spir_file))
        {
          devices_with_spir[num_devices_with_spir] = dev;
          devices_with_spir_lens[num_devices_with_spir]
              = (size_t)converted_spir_file_size;
          devices_with_spir_bins[num_devices_with_spir]
              = (const unsigned char *)converted_spir_file;
          ++num_devices_with_spir;
        }
#endif
    }

  POCL_GOTO_ERROR_ON ((num_devices_with_spir == 0), CL_INVALID_OPERATION,
                      "No device in context supports SPIR\n");
  POCL_MSG_PRINT_GENERAL ("Creating context from IL for %u devices\n",
                          num_devices_with_spir);

  program = POname (clCreateProgramWithBinary) (
      context, num_devices_with_spir, devices_with_spir,
      devices_with_spir_lens, devices_with_spir_bins, NULL, &errcode);
  if (errcode == CL_SUCCESS)
    {
      POCL_LOCK_OBJ (program);
      program->program_il = (char *)malloc (length);
      memcpy (program->program_il, il, length);
      program->program_il_size = length;
      POCL_UNLOCK_OBJ (program);
    }

ERROR:
  if (errcode_ret)
    *errcode_ret = errcode;
#ifdef ENABLE_SPIRV
  POCL_MEM_FREE (converted_spir_file);
#endif
  return program;
}
POsym(clCreateProgramWithIL)
