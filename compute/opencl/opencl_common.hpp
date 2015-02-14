/*
 *  Flo's Open libRary (floor)
 *  Copyright (C) 2004 - 2015 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __FLOOR_OPENCL_COMMON_HPP__
#define __FLOOR_OPENCL_COMMON_HPP__

#include <floor/core/essentials.hpp>

#if !defined(FLOOR_NO_OPENCL)

#if defined(__APPLE__)
#if defined(FLOOR_IOS)
// don't let cl_gl_ext.h get included (it won't work anyways)
#define __OPENCL_CL_GL_EXT_H
#define __GCL_H
#endif
#include <OpenCL/OpenCL.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_platform.h>
#include <OpenCL/cl_ext.h>
#include <OpenCL/cl_gl.h>
#if !defined(FLOOR_IOS)
#include <OpenGL/CGLContext.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLDevice.h>
#endif
#else
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#endif

//! opencl version of the platform/driver/device
enum class OPENCL_VERSION : uint32_t {
	OPENCL_1_0,
	OPENCL_1_1,
	OPENCL_1_2,
	OPENCL_2_0,
};

constexpr const char* cl_error_to_string(const int& error_code) {
	// NOTE: don't use actual enums here so this doesn't have to rely on opencl version or vendor specific headers
	switch(error_code) {
		case 0: return "CL_SUCCESS";
		case -1: return "CL_DEVICE_NOT_FOUND";
		case -2: return "CL_DEVICE_NOT_AVAILABLE";
		case -3: return "CL_COMPILER_NOT_AVAILABLE";
		case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		case -5: return "CL_OUT_OF_RESOURCES";
		case -6: return "CL_OUT_OF_HOST_MEMORY";
		case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
		case -8: return "CL_MEM_COPY_OVERLAP";
		case -9: return "CL_IMAGE_FORMAT_MISMATCH";
		case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		case -11: return "CL_BUILD_PROGRAM_FAILURE";
		case -12: return "CL_MAP_FAILURE";
		case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		case -15: return "CL_COMPILE_PROGRAM_FAILURE";
		case -16: return "CL_LINKER_NOT_AVAILABLE";
		case -17: return "CL_LINK_PROGRAM_FAILURE";
		case -18: return "CL_DEVICE_PARTITION_FAILED";
		case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
		case -30: return "CL_INVALID_VALUE";
		case -31: return "CL_INVALID_DEVICE_TYPE";
		case -32: return "CL_INVALID_PLATFORM";
		case -33: return "CL_INVALID_DEVICE";
		case -34: return "CL_INVALID_CONTEXT";
		case -35: return "CL_INVALID_QUEUE_PROPERTIES";
		case -36: return "CL_INVALID_COMMAND_QUEUE";
		case -37: return "CL_INVALID_HOST_PTR";
		case -38: return "CL_INVALID_MEM_OBJECT";
		case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
		case -40: return "CL_INVALID_IMAGE_SIZE";
		case -41: return "CL_INVALID_SAMPLER";
		case -42: return "CL_INVALID_BINARY";
		case -43: return "CL_INVALID_BUILD_OPTIONS";
		case -44: return "CL_INVALID_PROGRAM";
		case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
		case -46: return "CL_INVALID_KERNEL_NAME";
		case -47: return "CL_INVALID_KERNEL_DEFINITION";
		case -48: return "CL_INVALID_KERNEL";
		case -49: return "CL_INVALID_ARG_INDEX";
		case -50: return "CL_INVALID_ARG_VALUE";
		case -51: return "CL_INVALID_ARG_SIZE";
		case -52: return "CL_INVALID_KERNEL_ARGS";
		case -53: return "CL_INVALID_WORK_DIMENSION";
		case -54: return "CL_INVALID_WORK_GROUP_SIZE";
		case -55: return "CL_INVALID_WORK_ITEM_SIZE";
		case -56: return "CL_INVALID_GLOBAL_OFFSET";
		case -57: return "CL_INVALID_EVENT_WAIT_LIST";
		case -58: return "CL_INVALID_EVENT";
		case -59: return "CL_INVALID_OPERATION";
		case -60: return "CL_INVALID_GL_OBJECT";
		case -61: return "CL_INVALID_BUFFER_SIZE";
		case -62: return "CL_INVALID_MIP_LEVEL";
		case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
		case -64: return "CL_INVALID_PROPERTY";
		case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
		case -66: return "CL_INVALID_COMPILER_OPTIONS";
		case -67: return "CL_INVALID_LINKER_OPTIONS";
		case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
		case -69: return "CL_INVALID_PIPE_SIZE";
		case -70: return "CL_INVALID_DEVICE_QUEUE";
		case -1000:
#if defined(__APPLE__)
			return "CL_INVALID_GL_CONTEXT_APPLE";
#else
			return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
#endif
		case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
		case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
		case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
		case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
		case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
		case -1006: return "CL_INVALID_D3D11_DEVICE_KHR";
		case -1007: return "CL_INVALID_D3D11_RESOURCE_KHR";
		case -1008: return "CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR";
		case -1009: return "CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR";
		case -1010: return "CL_INVALID_DX9_MEDIA_ADAPTER_KHR";
		case -1011: return "CL_INVALID_DX9_MEDIA_SURFACE_KHR";
		case -1012: return "CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR";
		case -1013: return "CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR";
		case -1057: return "CL_DEVICE_PARTITION_FAILED_EXT";
		case -1058: return "CL_INVALID_PARTITION_COUNT_EXT";
		case -1059: return "CL_INVALID_PARTITION_NAME_EXT";
		case -1060: return "CL_INVALID_ARG_NAME_APPLE";
		case -1092: return "CL_EGL_RESOURCE_NOT_ACQUIRED_KHR";
		case -1093: return "CL_INVALID_EGL_OBJECT_KHR";
		case -1094: return "CL_INVALID_ACCELERATOR_INTEL";
		case -1095: return "CL_INVALID_ACCELERATOR_TYPE_INTEL";
		case -1096: return "CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL";
		case -1097: return "CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL";
		case -1098: return "CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL";
		case -1099: return "CL_INVALID_VA_API_MEDIA_SURFACE_INTEL";
		case -1100: return "CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL";
		case -1101: return "CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL";
		case -6000: return "CL_INVALID_ACCELERATOR_INTEL_DEPRECATED";
		case -6001: return "CL_INVALID_ACCELERATOR_TYPE_INTEL_DEPRECATED";
		case -6002: return "CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL_DEPRECATED";
		case -6003: return "CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL_DEPRECATED";
		default: break;
	}
	return "<UNKNOWN_ERROR>";
}

#define CL_CALL_RET(call, error_msg, ...) { \
	const cl_int call_err_var = call; \
	if(call_err_var != CL_SUCCESS) { \
		log_error(error_msg ": %u: %s", call_err_var, cl_error_to_string(call_err_var)); \
		return __VA_ARGS__; \
	} \
}
#define CL_CALL_CONT(call, error_msg) { \
	const cl_int call_err_var = call; \
	if(call_err_var != CL_SUCCESS) { \
		log_error(error_msg ": %u: %s", call_err_var, cl_error_to_string(call_err_var)); \
		continue; \
	} \
}
#define CL_CALL_ERR_PARAM_RET(call, err_var_name, error_msg, ...) { \
	cl_int err_var_name = CL_SUCCESS; \
	call; \
	if(err_var_name != CL_SUCCESS) { \
		log_error(error_msg ": %u: %s", err_var_name, cl_error_to_string(err_var_name)); \
		return __VA_ARGS__; \
	} \
}
#define CL_CALL_ERR_PARAM_CONT(call, err_var_name, error_msg) { \
	cl_int err_var_name = CL_SUCCESS; \
	call; \
	if(err_var_name != CL_SUCCESS) { \
		log_error(error_msg ": %u: %s", err_var_name, cl_error_to_string(err_var_name)); \
		continue; \
	} \
}

// define this if you want the cl_get_info functions/voodoo
#if defined(FLOOR_OPENCL_INFO_FUNCS)
#include <floor/core/cpp_headers.hpp>
#include <floor/core/logger.hpp>
#include <floor/constexpr/const_string.hpp>

#define FLOOR_CL_INFO_RET_TYPES(F) \
/* cl_platform_info */ \
F(cl_platform_id, cl_platform_info, CL_PLATFORM_PROFILE, string) \
F(cl_platform_id, cl_platform_info, CL_PLATFORM_VERSION, string) \
F(cl_platform_id, cl_platform_info, CL_PLATFORM_NAME, string) \
F(cl_platform_id, cl_platform_info, CL_PLATFORM_VENDOR, string) \
F(cl_platform_id, cl_platform_info, CL_PLATFORM_EXTENSIONS, string) \
/* cl_device_info */ \
F(cl_device_id, cl_device_info, CL_DEVICE_TYPE, cl_device_type) \
F(cl_device_id, cl_device_info, CL_DEVICE_VENDOR_ID, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_WORK_ITEM_SIZES, vector<size_t>) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_ADDRESS_BITS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_READ_IMAGE_ARGS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE2D_MAX_WIDTH, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE2D_MAX_HEIGHT, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE3D_MAX_WIDTH, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE3D_MAX_DEPTH, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE_SUPPORT, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_PARAMETER_SIZE, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_SAMPLERS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MEM_BASE_ADDR_ALIGN, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_SINGLE_FP_CONFIG, cl_device_fp_config) \
F(cl_device_id, cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, cl_device_mem_cache_type) \
F(cl_device_id, cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, cl_ulong) \
F(cl_device_id, cl_device_info, CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong) \
F(cl_device_id, cl_device_info, CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_LOCAL_MEM_TYPE, cl_device_local_mem_type) \
F(cl_device_id, cl_device_info, CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong) \
F(cl_device_id, cl_device_info, CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_PROFILING_TIMER_RESOLUTION, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_ENDIAN_LITTLE, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_AVAILABLE, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_COMPILER_AVAILABLE, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_EXECUTION_CAPABILITIES, cl_device_exec_capabilities) \
F(cl_device_id, cl_device_info, CL_DEVICE_QUEUE_PROPERTIES, cl_command_queue_properties) \
F(cl_device_id, cl_device_info, CL_DEVICE_NAME, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_VENDOR, string) \
F(cl_device_id, cl_device_info, CL_DRIVER_VERSION, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_PROFILE, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_VERSION, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_EXTENSIONS, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_PLATFORM, cl_platform_id) \
F(cl_device_id, cl_device_info, CL_DEVICE_DOUBLE_FP_CONFIG, cl_device_fp_config) \
F(cl_device_id, cl_device_info, CL_DEVICE_HALF_FP_CONFIG, cl_device_fp_config) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_HOST_UNIFIED_MEMORY, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_OPENCL_C_VERSION, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_LINKER_AVAILABLE, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_BUILT_IN_KERNELS, string) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_PARENT_DEVICE, cl_device_id) \
F(cl_device_id, cl_device_info, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PARTITION_PROPERTIES, vector<cl_device_partition_property>) \
F(cl_device_id, cl_device_info, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, cl_device_affinity_domain) \
F(cl_device_id, cl_device_info, CL_DEVICE_PARTITION_TYPE, vector<cl_device_partition_property>) \
F(cl_device_id, cl_device_info, CL_DEVICE_REFERENCE_COUNT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, cl_bool) \
F(cl_device_id, cl_device_info, CL_DEVICE_PRINTF_BUFFER_SIZE, size_t) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE_PITCH_ALIGNMENT, cl_uint) \
F(cl_device_id, cl_device_info, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, cl_uint) \
/* cl_context_info */ \
F(cl_context, cl_context_info, CL_CONTEXT_REFERENCE_COUNT, cl_uint) \
F(cl_context, cl_context_info, CL_CONTEXT_DEVICES, vector<cl_device_id>) \
F(cl_context, cl_context_info, CL_CONTEXT_PROPERTIES, vector<cl_context_properties>) \
F(cl_context, cl_context_info, CL_CONTEXT_NUM_DEVICES, cl_uint) \
/* cl_program_info */ \
F(cl_program, cl_program_info, CL_PROGRAM_REFERENCE_COUNT, cl_uint) \
F(cl_program, cl_program_info, CL_PROGRAM_CONTEXT, cl_context) \
F(cl_program, cl_program_info, CL_PROGRAM_NUM_DEVICES, cl_uint) \
F(cl_program, cl_program_info, CL_PROGRAM_DEVICES, vector<cl_device_id>) \
F(cl_program, cl_program_info, CL_PROGRAM_SOURCE, string) \
F(cl_program, cl_program_info, CL_PROGRAM_BINARY_SIZES, vector<size_t>) \
F(cl_program, cl_program_info, CL_PROGRAM_NUM_KERNELS, size_t) \
F(cl_program, cl_program_info, CL_PROGRAM_KERNEL_NAMES, string) \
/* cl_program_build_info */ \
F(cl_program, cl_program_build_info, CL_PROGRAM_BUILD_STATUS, cl_build_status) \
F(cl_program, cl_program_build_info, CL_PROGRAM_BUILD_OPTIONS, string) \
F(cl_program, cl_program_build_info, CL_PROGRAM_BUILD_LOG, string) \
/* cl_kernel_info */ \
F(cl_kernel, cl_kernel_info, CL_KERNEL_FUNCTION_NAME, string) \
F(cl_kernel, cl_kernel_info, CL_KERNEL_NUM_ARGS, cl_uint) \
F(cl_kernel, cl_kernel_info, CL_KERNEL_REFERENCE_COUNT, cl_uint) \
F(cl_kernel, cl_kernel_info, CL_KERNEL_CONTEXT, cl_context) \
F(cl_kernel, cl_kernel_info, CL_KERNEL_PROGRAM, cl_program) \
F(cl_kernel, cl_kernel_info, CL_KERNEL_ATTRIBUTES, string)

template <cl_uint info_type> struct cl_info_type;
template <typename cl_info_object, cl_uint info_type> struct cl_is_valid_info_type : public false_type {};

#define FLOOR_CL_INFO_RET_TYPE_SPEC(object_type, object_info_type, info_type, ret_type) \
template <> struct cl_info_type<info_type> { \
	typedef ret_type type; \
	static constexpr const uint32_t info_hash { make_const_string(#object_info_type).hash() }; \
}; \
template <> struct cl_is_valid_info_type<object_type, info_type> : public true_type {};

FLOOR_CL_INFO_RET_TYPES(FLOOR_CL_INFO_RET_TYPE_SPEC)

// handle additional arguments for certain clGet*Info functions
#define FLOOR_CI_NO_ADD()
#define FLOOR_CI_PROGRAM_BUILD_INFO_ARGS() , const cl_device_id& device
#define FLOOR_CI_PROGRAM_BUILD_INFO_ARG_NAMES() , device

#define FLOOR_CL_INFO_TYPES(F) \
F(cl_platform_id, cl_platform_info, clGetPlatformInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_device_id, cl_device_info, clGetDeviceInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_context, cl_context_info, clGetContextInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_command_queue, cl_command_queue_info, clGetCommandQueueInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_mem, cl_mem_info, clGetMemObjectInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_mem, cl_image_info, clGetImageInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_sampler, cl_sampler_info, clGetSamplerInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_program, cl_program_info, clGetProgramInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_program, cl_program_build_info, clGetProgramBuildInfo, FLOOR_CI_PROGRAM_BUILD_INFO_ARGS, FLOOR_CI_PROGRAM_BUILD_INFO_ARG_NAMES) \
F(cl_kernel, cl_kernel_info, clGetKernelInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_event, cl_event_info, clGetEventInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD) \
F(cl_event, cl_profiling_info, clGetEventProfilingInfo, FLOOR_CI_NO_ADD, FLOOR_CI_NO_ADD)

#define FLOOR_CL_INFO_FUNC(obj_type, cl_info_typename, cl_info_func, additional_args, additional_arg_names) \
template <cl_uint info_type, \
		  enable_if_t<(cl_is_valid_info_type<obj_type, info_type>::value && \
					   !is_same<typename cl_info_type<info_type>::type, string>::value && \
					   !is_vector<typename cl_info_type<info_type>::type>::value && \
					   make_const_string(#cl_info_typename).hash() == cl_info_type<info_type>::info_hash), int> = 0> \
typename cl_info_type<info_type>::type cl_get_info(const obj_type& obj additional_args() ) { \
	typedef typename cl_info_type<info_type>::type ret_type; \
	ret_type ret {}; \
	cl_info_func(obj additional_arg_names() , info_type, sizeof(ret_type), &ret, nullptr); \
	return ret; \
} \
template <cl_uint info_type, \
		  enable_if_t<(cl_is_valid_info_type<obj_type, info_type>::value && \
					   is_same<typename cl_info_type<info_type>::type, string>::value && \
					   !is_vector<typename cl_info_type<info_type>::type>::value && \
					   make_const_string(#cl_info_typename).hash() == cl_info_type<info_type>::info_hash), int> = 0> \
typename cl_info_type<info_type>::type cl_get_info(const obj_type& obj additional_args() ) { \
	size_t buf_size = 0; \
	cl_info_func(obj additional_arg_names() , info_type, 0, nullptr, &buf_size); \
	vector<char> info(buf_size); \
	cl_info_func(obj additional_arg_names() , info_type, buf_size, info.data(), nullptr); \
	return (buf_size > 0 ? string(info.data(), buf_size - 1 /* trim \0 */) : ""); \
} \
template <cl_uint info_type, \
		  enable_if_t<(cl_is_valid_info_type<obj_type, info_type>::value && \
					   !is_same<typename cl_info_type<info_type>::type, string>::value && \
					   is_vector<typename cl_info_type<info_type>::type>::value && \
					   make_const_string(#cl_info_typename).hash() == cl_info_type<info_type>::info_hash), int> = 0> \
typename cl_info_type<info_type>::type cl_get_info(const obj_type& obj additional_args() ) { \
	typedef typename cl_info_type<info_type>::type ret_type; \
	typedef typename ret_type::value_type value_type; \
	size_t params_size = 0; \
	cl_info_func(obj additional_arg_names() , info_type, 0, nullptr, &params_size); \
	ret_type ret(params_size / sizeof(value_type)); \
	cl_info_func(obj additional_arg_names() , info_type, params_size, ret.data(), nullptr); \
	return ret; \
}

FLOOR_CL_INFO_TYPES(FLOOR_CL_INFO_FUNC)

// CL_PROGRAM_BINARIES is rather complicated/different than the other clGet*Info calls, need to add special handling
template <cl_uint info_type, enable_if_t<info_type == CL_PROGRAM_BINARIES, int> = 0>
vector<string> cl_get_info(const cl_program& program) {
	// need to get the binary size for each device first
	const auto sizes = cl_get_info<CL_PROGRAM_BINARY_SIZES>(program);
	const auto binary_count = sizes.size();
	
	// then allocate enough memory for each binary (+init pointers to our strings, opencl api will write uint8_t data)
	vector<string> ret(binary_count);
	for(size_t i = 0; i < binary_count; ++i) {
		ret[i].resize(sizes[i]);
	}
	vector<uint8_t*> binary_ptrs(binary_count);
	for(size_t i = 0; i < binary_count; ++i) {
		binary_ptrs[i] = (uint8_t*)ret[i].data();
	}
	
	// finally: retrieve the binaries
	clGetProgramInfo(program, CL_PROGRAM_BINARIES, binary_count * sizeof(unsigned char*), &binary_ptrs[0], nullptr);
	
	// note: clGetProgramInfo should have directly written into the strings in the return vector
	return ret;
}

#endif // FLOOR_OPENCL_INFO_FUNCS

#endif // FLOOR_NO_OPENCL

#endif