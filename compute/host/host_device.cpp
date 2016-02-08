/*
 *  Flo's Open libRary (floor)
 *  Copyright (C) 2004 - 2016 Florian Ziesche
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

#include <floor/compute/host/host_device.hpp>

host_device::host_device() : compute_device() {
	// init statically known info
	type = type = compute_device::TYPE::CPU0;
	platform_vendor = COMPUTE_VENDOR::HOST;
	version_str = FLOOR_BUILD_VERSION_STR;
	driver_version_str = FLOOR_BUILD_VERSION_STR;
	
	local_mem_size = host_compute_local_memory_size;
	local_mem_dedicated = false;
	
	// always at least 4 (SSE, newer NEON), 8-wide if avx/avx, 16-wide if avx-512
	simd_width = (core::cpu_has_avx() ? (core::cpu_has_avx512() ? 16 : 8) : 4);
	simd_range = { 1, simd_width };
	
	max_work_item_sizes = { 0xFFFFFFFFu };
	
	// can technically use any dim as long as it fits into memory
	max_image_1d_dim = { 65536 };
	max_image_2d_dim = { 65536, 65536 };
	max_image_3d_dim = { 65536, 65536, 65536 };
	
	double_support = true;
	unified_memory = true;
	basic_64_bit_atomics_support = true;
	extended_64_bit_atomics_support = true;
	
	image_support = true;
	image_depth_support = true;
	image_depth_write_support = true;
	image_msaa_support = false; // not implementable with opengl
	image_msaa_write_support = false;
	image_cube_support = true;
	image_cube_write_support = true;
	image_mipmap_support = false; // for now
	image_mipmap_write_support = false;
	
#if defined(PLATFORM_X32)
	bitness = 32;
#elif defined(PLATFORM_X64)
	bitness = 64;
#endif
}
