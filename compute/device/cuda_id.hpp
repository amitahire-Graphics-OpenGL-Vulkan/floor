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

#ifndef __FLOOR_COMPUTE_DEVICE_CUDA_ID_HPP__
#define __FLOOR_COMPUTE_DEVICE_CUDA_ID_HPP__

#if defined(FLOOR_COMPUTE_CUDA) || defined(FLOOR_COMPUTE_METAL)

// this is a workaround for missing "air.get_*_id.i32" functions on metal on os x
// -> nvvm/ptx builtin functions aren't defined there, llvm intrinsics are piped through correctly however
#if defined(FLOOR_COMPUTE_METAL)
metal_func uint32_t __builtin_ptx_read_tid_x() asm("llvm.nvvm.read.ptx.sreg.tid.x");
metal_func uint32_t __builtin_ptx_read_tid_y() asm("llvm.nvvm.read.ptx.sreg.tid.y");
metal_func uint32_t __builtin_ptx_read_tid_z() asm("llvm.nvvm.read.ptx.sreg.tid.z");
metal_func uint32_t __builtin_ptx_read_ntid_x() asm("llvm.nvvm.read.ptx.sreg.ntid.x");
metal_func uint32_t __builtin_ptx_read_ntid_y() asm("llvm.nvvm.read.ptx.sreg.ntid.y");
metal_func uint32_t __builtin_ptx_read_ntid_z() asm("llvm.nvvm.read.ptx.sreg.ntid.z");
metal_func uint32_t __builtin_ptx_read_ctaid_x() asm("llvm.nvvm.read.ptx.sreg.ctaid.x");
metal_func uint32_t __builtin_ptx_read_ctaid_y() asm("llvm.nvvm.read.ptx.sreg.ctaid.y");
metal_func uint32_t __builtin_ptx_read_ctaid_z() asm("llvm.nvvm.read.ptx.sreg.ctaid.z");
metal_func uint32_t __builtin_ptx_read_nctaid_x() asm("llvm.nvvm.read.ptx.sreg.nctaid.x");
metal_func uint32_t __builtin_ptx_read_nctaid_y() asm("llvm.nvvm.read.ptx.sreg.nctaid.y");
metal_func uint32_t __builtin_ptx_read_nctaid_z() asm("llvm.nvvm.read.ptx.sreg.nctaid.z");
#endif

// make this a little easier to use
#define FLOOR_CUDA_DIM0 __attribute__((always_inline, flatten, pure, enable_if(dim == 0, "const 0")))
#define FLOOR_CUDA_DIM1 __attribute__((always_inline, flatten, pure, enable_if(dim == 1, "const 1")))
#define FLOOR_CUDA_DIM2 __attribute__((always_inline, flatten, pure, enable_if(dim == 2, "const 2")))
#define FLOOR_CUDA_INVALID __attribute__((always_inline, flatten, pure, enable_if(dim > 2, "invalid dim"), unavailable("invalid dim")))
#define FLOOR_CUDA_DIM_RT __attribute__((always_inline, flatten, pure))

#define global_id size3 { \
	__builtin_ptx_read_ctaid_x() * __builtin_ptx_read_ntid_x() + __builtin_ptx_read_tid_x(), \
	__builtin_ptx_read_ctaid_y() * __builtin_ptx_read_ntid_y() + __builtin_ptx_read_tid_y(), \
	__builtin_ptx_read_ctaid_z() * __builtin_ptx_read_ntid_z() + __builtin_ptx_read_tid_z() \
}
#define global_size size3 { \
	__builtin_ptx_read_nctaid_x() * __builtin_ptx_read_ntid_x(), \
	__builtin_ptx_read_nctaid_y() * __builtin_ptx_read_ntid_y(), \
	__builtin_ptx_read_nctaid_z() * __builtin_ptx_read_ntid_z() \
}
#define local_id size3 { \
	__builtin_ptx_read_tid_x(), \
	__builtin_ptx_read_tid_y(), \
	__builtin_ptx_read_tid_z() \
}
#define local_size size3 { \
	__builtin_ptx_read_ntid_x(), \
	__builtin_ptx_read_ntid_y(), \
	__builtin_ptx_read_ntid_z() \
}
#define group_id size3 { \
	__builtin_ptx_read_ctaid_x(), \
	__builtin_ptx_read_ctaid_y(), \
	__builtin_ptx_read_ctaid_z() \
}
#define group_size size3 { \
	__builtin_ptx_read_nctaid_x(), \
	__builtin_ptx_read_nctaid_y(), \
	__builtin_ptx_read_nctaid_z() \
}

static uint64_t get_global_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_ctaid_x() * __builtin_ptx_read_ntid_x() + __builtin_ptx_read_tid_x();
}
static uint64_t get_global_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_ctaid_y() * __builtin_ptx_read_ntid_y() + __builtin_ptx_read_tid_y();
}
static uint64_t get_global_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_ctaid_z() * __builtin_ptx_read_ntid_z() + __builtin_ptx_read_tid_z();
}
static uint64_t get_global_id(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_global_id(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_ctaid_x() * __builtin_ptx_read_ntid_x() + __builtin_ptx_read_tid_x();
		case 1: return __builtin_ptx_read_ctaid_y() * __builtin_ptx_read_ntid_y() + __builtin_ptx_read_tid_y();
		case 2: return __builtin_ptx_read_ctaid_z() * __builtin_ptx_read_ntid_z() + __builtin_ptx_read_tid_z();
		default: return 0;
	}
}
static uint64_t get_global_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_nctaid_x() * __builtin_ptx_read_ntid_x();
}
static uint64_t get_global_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_nctaid_y() * __builtin_ptx_read_ntid_y();
}
static uint64_t get_global_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_nctaid_z() * __builtin_ptx_read_ntid_z();
}
static uint64_t get_global_size(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_global_size(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_nctaid_x() * __builtin_ptx_read_ntid_x();
		case 1: return __builtin_ptx_read_nctaid_y() * __builtin_ptx_read_ntid_y();
		case 2: return __builtin_ptx_read_nctaid_z() * __builtin_ptx_read_ntid_z();
		default: return 1;
	}
}

static uint64_t get_local_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_tid_x();
}
static uint64_t get_local_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_tid_y();
}
static uint64_t get_local_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_tid_z();
}
static uint64_t get_local_id(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_local_id(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_tid_x();
		case 1: return __builtin_ptx_read_tid_y();
		case 2: return __builtin_ptx_read_tid_z();
		default: return 0;
	}
}

static uint64_t get_local_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_ntid_x();
}
static uint64_t get_local_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_ntid_y();
}
static uint64_t get_local_size(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_ntid_z();
}
static uint64_t get_local_size(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_local_size(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_ntid_x();
		case 1: return __builtin_ptx_read_ntid_y();
		case 2: return __builtin_ptx_read_ntid_z();
		default: return 1;
	}
}

static uint64_t get_group_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_ctaid_x();
}
static uint64_t get_group_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_ctaid_y();
}
static uint64_t get_group_id(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_ctaid_z();
}
static uint64_t get_group_id(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_group_id(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_ctaid_x();
		case 1: return __builtin_ptx_read_ctaid_y();
		case 2: return __builtin_ptx_read_ctaid_z();
		default: return 0;
	}
}

static uint64_t get_num_groups(uint32_t dim floor_unused) FLOOR_CUDA_DIM0 {
	return __builtin_ptx_read_nctaid_x();
}
static uint64_t get_num_groups(uint32_t dim floor_unused) FLOOR_CUDA_DIM1 {
	return __builtin_ptx_read_nctaid_y();
}
static uint64_t get_num_groups(uint32_t dim floor_unused) FLOOR_CUDA_DIM2 {
	return __builtin_ptx_read_nctaid_z();
}
static uint64_t get_num_groups(uint32_t dim floor_unused) FLOOR_CUDA_INVALID {
	return 42;
}
static uint64_t get_num_groups(uint32_t dim) FLOOR_CUDA_DIM_RT {
	switch(dim) {
		case 0: return __builtin_ptx_read_nctaid_x();
		case 1: return __builtin_ptx_read_nctaid_y();
		case 2: return __builtin_ptx_read_nctaid_z();
		default: return 1;
	}
}

static uint32_t get_work_dim() {
	// grid dim (X, Y, Z)
	// -> if Z is 1, must either be 1D or 2D
	if(__builtin_ptx_read_nctaid_z() == 1) {
		// -> if Y is 1 as well, it is 1D, else 2D
		return (__builtin_ptx_read_nctaid_y() == 1 ? 1 : 2);
	}
	// else: -> Z is not 1, must always be 3D
	return 3;
}

//! currently not supported by any compute implementation
static constexpr uint64_t get_global_offset(uint32_t dim floor_unused) {
	return 0;
}

#endif

#endif
