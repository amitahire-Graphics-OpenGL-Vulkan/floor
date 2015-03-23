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

#ifndef __FLOOR_METAL_KERNEL_HPP__
#define __FLOOR_METAL_KERNEL_HPP__

#include <floor/compute/metal/metal_common.hpp>

#if !defined(FLOOR_NO_METAL)

#include <floor/core/logger.hpp>
#include <floor/threading/atomic_spin_lock.hpp>
#include <floor/compute/compute_buffer.hpp>
#include <floor/compute/llvm_compute.hpp>

// the amount of macro voodoo is too damn high ...
#define FLOOR_METAL_KERNEL_IMPL 1
#include <floor/compute/compute_kernel.hpp>
#undef FLOOR_METAL_KERNEL_IMPL

class metal_buffer;
class metal_device;
class metal_kernel final : public compute_kernel {
protected:
	// encapsulates MTLComputeCommandEncoder and MTLCommandBuffer, so we don't need obj-c++ here
	struct metal_encoder;
	
public:
	metal_kernel(const void* kernel,
				 const void* kernel_state,
				 const metal_device* device,
				 const llvm_compute::kernel_info& info);
	~metal_kernel() override;
	
	template <typename... Args> void execute(compute_queue* queue,
											 const uint32_t work_dim floor_unused,
											 const size3 global_work_size,
											 const size3 local_work_size_,
											 Args&&... args) REQUIRES(!args_lock) {
		// need to make sure that only one thread is setting kernel arguments at a time
		GUARD(args_lock);
		
		//
		auto encoder = create_encoder(queue);
		
		// set and handle kernel arguments
		set_kernel_arguments<0>(queue, encoder.get(), forward<Args>(args)...);
		
		// run
		const uint3 block_dim { local_work_size_.maxed(1u) }; // prevent % or / by 0, also: needs at least 1
		const uint3 grid_dim_overflow {
			global_work_size.x > 0 ? std::min(uint32_t(global_work_size.x % block_dim.x), 1u) : 0u,
			global_work_size.y > 0 ? std::min(uint32_t(global_work_size.y % block_dim.y), 1u) : 0u,
			global_work_size.z > 0 ? std::min(uint32_t(global_work_size.z % block_dim.z), 1u) : 0u
		};
		uint3 grid_dim { (global_work_size / block_dim) + grid_dim_overflow };
		grid_dim.max(1u);
		
		execute_internal(queue, encoder, grid_dim, block_dim);
	}
	
protected:
	const void* kernel;
	const void* kernel_state;
	const string func_name;
	
	struct param {
		shared_ptr<metal_buffer> buffer;
		vector<uint8_t> cur_value;
	};
	vector<param> param_buffers;
	
	atomic_spin_lock args_lock;
	
	COMPUTE_TYPE get_compute_type() const override { return COMPUTE_TYPE::METAL; }
	
	// must use shared_ptr, b/c metal_encoder is incomplete at this point
	shared_ptr<metal_encoder> create_encoder(compute_queue* queue);
	
	void execute_internal(compute_queue* queue,
						  shared_ptr<metal_encoder> encoder,
						  const uint3& grid_dim,
						  const uint3& block_dim);
	
	//! handle kernel call terminator
	template <uint32_t num>
	floor_inline_always void set_kernel_arguments(compute_queue*, metal_encoder*) {}
	
	//! set kernel argument and recurse
	template <uint32_t num, typename T, typename... Args>
	floor_inline_always void set_kernel_arguments(compute_queue* queue, metal_encoder* encoder,
												  T&& arg, Args&&... args) {
		set_kernel_argument(num, queue, encoder, forward<T>(arg));
		set_kernel_arguments<num + 1>(queue, encoder, forward<Args>(args)...);
	}
	
	//! actual kernel argument setter
	//! TODO: specialize for types (raw/pod types, images, ...)
	//! TODO: check for invalid args (e.g. raw pointers)
	template <typename T>
	void set_kernel_argument(const uint32_t num, compute_queue* queue, metal_encoder* encoder, T&& arg) {
		set_const_parameter(queue, encoder, num, &arg, sizeof(T));
	}
	void set_const_parameter(compute_queue* queue, metal_encoder* encoder, const uint32_t& num,
							 const void* ptr, const size_t& size);
	
	void set_kernel_argument(const uint32_t num,
							 compute_queue* queue,
							 metal_encoder* encoder,
							 shared_ptr<compute_buffer> arg);
	
};

#endif

#endif