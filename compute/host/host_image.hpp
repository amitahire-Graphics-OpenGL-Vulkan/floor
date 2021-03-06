/*
 *  Flo's Open libRary (floor)
 *  Copyright (C) 2004 - 2019 Florian Ziesche
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

#ifndef __FLOOR_HOST_IMAGE_HPP__
#define __FLOOR_HOST_IMAGE_HPP__

#include <floor/compute/host/host_common.hpp>

#if !defined(FLOOR_NO_HOST_COMPUTE)

#include <floor/compute/compute_image.hpp>
#include <floor/compute/device/host_limits.hpp>

class host_device;
class host_image final : public compute_image {
public:
	host_image(const compute_queue& cqueue,
			   const uint4 image_dim,
			   const COMPUTE_IMAGE_TYPE image_type,
			   void* host_ptr = nullptr,
			   const COMPUTE_MEMORY_FLAG flags_ = (COMPUTE_MEMORY_FLAG::HOST_READ_WRITE),
			   const uint32_t opengl_type = 0,
			   const uint32_t external_gl_object_ = 0,
			   const opengl_image_info* gl_image_info = nullptr);
	
	~host_image() override;
	
	bool acquire_opengl_object(const compute_queue* cqueue) override;
	bool release_opengl_object(const compute_queue* cqueue) override;
	
	void zero(const compute_queue& cqueue) override;
	
	void* __attribute__((aligned(128))) map(const compute_queue& cqueue,
											const COMPUTE_MEMORY_MAP_FLAG flags = (COMPUTE_MEMORY_MAP_FLAG::READ_WRITE | COMPUTE_MEMORY_MAP_FLAG::BLOCK)) override;
	
	void unmap(const compute_queue& cqueue, void* __attribute__((aligned(128))) mapped_ptr) override;
	
	//! returns a direct pointer to the internal host image buffer
	uint8_t* __attribute__((aligned(128))) get_host_image_buffer_ptr() const {
		return image;
	}
	
	//! returns the internal structure necessary to run a function/program with this image
	void* get_host_image_program_info() const {
		return (void*)&program_info;
	}
	
protected:
	uint8_t* __attribute__((aligned(1024))) image { nullptr };
	
	struct image_program_info {
		uint8_t* __attribute__((aligned(128))) buffer;
		COMPUTE_IMAGE_TYPE runtime_image_type;
		alignas(16) struct {
			uint4 dim;
			int4 clamp_dim_int;
			float4 clamp_dim_float;
			uint32_t offset;
			const uint32_t _unused[3] { 0, 0, 0 };
		} level_info[host_limits::max_mip_levels];
		static_assert(sizeof(level_info) == (16 * 4) * host_limits::max_mip_levels,
					  "invalid level_info size");
	} program_info;
	
	//! separate create buffer function, b/c it's called by the constructor and resize
	bool create_internal(const bool copy_host_data, const compute_queue& cqueue);
	
};

#endif

#endif
