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

#ifndef __FLOOR_VULKAN_KERNEL_HPP__
#define __FLOOR_VULKAN_KERNEL_HPP__

#include <floor/compute/vulkan/vulkan_common.hpp>

#if !defined(FLOOR_NO_VULKAN)

#include <floor/core/logger.hpp>
#include <floor/threading/atomic_spin_lock.hpp>
#include <floor/compute/vulkan/vulkan_buffer.hpp>
#include <floor/compute/vulkan/vulkan_image.hpp>
#include <floor/compute/compute_kernel.hpp>

class vulkan_device;

class vulkan_kernel final : public compute_kernel {
public:
	// don't want to include vulkan_queue here
	struct vulkan_encoder;
	
	struct vulkan_kernel_entry : kernel_entry {
		VkPipelineLayout pipeline_layout { nullptr };
		VkPipelineShaderStageCreateInfo stage_info;
		VkDescriptorSetLayout desc_set_layout { nullptr };
		VkDescriptorPool desc_pool { nullptr };
		VkDescriptorSet desc_set { nullptr };
		vector<VkDescriptorType> desc_types;
		
		struct spec_entry {
			VkPipeline pipeline { nullptr };
			VkSpecializationInfo info;
			vector<VkSpecializationMapEntry> map_entries;
			vector<uint32_t> data;
		};
		// work-group size -> spec entry
		flat_map<uint64_t, spec_entry> specializations;
		
		//! creates a 64-bit key out of the specified uint3 work-group size
		//! NOTE: components of the work-group size must fit into 16-bit
		static uint64_t make_spec_key(const uint3& work_group_size);
		
		//! specializes/builds a compute pipeline for the specified work-group size
		vulkan_kernel_entry::spec_entry* specialize(const vulkan_device& device,
													const uint3& work_group_size);
	};
	typedef flat_map<const vulkan_device&, vulkan_kernel_entry> kernel_map_type;
	
	struct idx_handler {
		// actual argument index (directly corresponding to the c++ source code)
		uint32_t arg { 0 };
		// index into the descriptor set that will be updated/written
		// NOTE: starts out at 1, because 0 is the fixed sampler set
		uint32_t write_desc { 1 };
		// binding index in the resp. descriptor set
		uint32_t binding { 0 };
		// current kernel/shader entry
		uint32_t entry { 0 };
		
		// advances all indices by one
		void next() {
			++arg;
			++write_desc;
			++binding;
		}
	};
	
	vulkan_kernel(kernel_map_type&& kernels);
	~vulkan_kernel() override = default;
	
	void execute(const compute_queue& cqueue,
				 const bool& is_cooperative,
				 const uint32_t& dim,
				 const uint3& global_work_size,
				 const uint3& local_work_size,
				 const vector<compute_kernel_arg>& args) const override;
	
	//! NOTE: very wip/temporary
	struct multi_draw_entry {
		uint32_t vertex_count;
		uint32_t instance_count { 1u };
		uint32_t first_vertex { 0u };
		uint32_t first_instance { 0u };
	};
	struct multi_draw_indexed_entry {
		compute_buffer* index_buffer;
		uint32_t index_count;
		uint32_t instance_count { 1u };
		uint32_t first_index { 0u };
		int32_t vertex_offset { 0u };
		uint32_t first_instance { 0u };
	};
	
	//! NOTE: very wip/temporary, need to specifically set vs/fs entries here, b/c we only store one in here
	//! NOTE: vertex shader arguments are specified first, fragment shader arguments after
	//! NOTE: 'fragment_shader' can be nullptr when only running a vertex shader
	template <typename... Args> void multi_draw(const compute_queue& cqueue,
												// NOTE: this is a vulkan_queue::command_buffer*
												void* cmd_buffer,
												const VkPipeline pipeline,
												const VkPipelineLayout pipeline_layout,
												const vulkan_kernel_entry* vertex_shader,
												const vulkan_kernel_entry* fragment_shader,
												// NOTE: current workaround for not directly submitting cmd buffers
												vector<shared_ptr<compute_buffer>>& retained_buffers,
												const vector<multi_draw_entry>& draw_entries,
												Args&&... args) const {
		if(vertex_shader == nullptr) {
			log_error("must specify a vertex shader!");
			return;
		}
		
		// create command buffer ("encoder") for this kernel execution
		bool encoder_success = false;
		const vector<const vulkan_kernel_entry*> shader_entries {
			vertex_shader, fragment_shader
		};
		auto encoder = create_encoder(cqueue, cmd_buffer, pipeline, pipeline_layout,
									  shader_entries, encoder_success);
		if(!encoder_success) {
			log_error("failed to create vulkan encoder / command buffer for shader \"%s\"",
					  vertex_shader->info->name);
			return;
		}
		
		// set and handle arguments
		idx_handler idx;
		set_arguments(encoder.get(), shader_entries, idx, forward<Args>(args)...);
		
		// run
		draw_internal(encoder, cqueue, vertex_shader, fragment_shader, retained_buffers,
					  &draw_entries, nullptr);
	}
	
	//! NOTE: very wip/temporary, need to specifically set vs/fs entries here, b/c we only store one in here
	//! NOTE: vertex shader arguments are specified first, fragment shader arguments after
	//! NOTE: 'fragment_shader' can be nullptr when only running a vertex shader
	template <typename... Args> void multi_draw_indexed(const compute_queue& cqueue,
														// NOTE: this is a vulkan_queue::command_buffer*
														void* cmd_buffer,
														const VkPipeline pipeline,
														const VkPipelineLayout pipeline_layout,
														const vulkan_kernel_entry* vertex_shader,
														const vulkan_kernel_entry* fragment_shader,
														// NOTE: current workaround for not directly submitting cmd buffers
														vector<shared_ptr<compute_buffer>>& retained_buffers,
														const vector<multi_draw_indexed_entry>& draw_entries,
														Args&&... args) const {
		if(vertex_shader == nullptr) {
			log_error("must specify a vertex shader!");
			return;
		}
		
		// create command buffer ("encoder") for this kernel execution
		bool encoder_success = false;
		const vector<const vulkan_kernel_entry*> shader_entries {
			vertex_shader, fragment_shader
		};
		auto encoder = create_encoder(cqueue, cmd_buffer, pipeline, pipeline_layout,
									  shader_entries, encoder_success);
		if(!encoder_success) {
			log_error("failed to create vulkan encoder / command buffer for shader \"%s\"",
					  vertex_shader->info->name);
			return;
		}
		
		// set and handle arguments
		idx_handler idx;
		set_arguments(encoder.get(), shader_entries, idx, forward<Args>(args)...);
		
		// run
		draw_internal(encoder, cqueue, vertex_shader, fragment_shader, retained_buffers,
					  nullptr, &draw_entries);
	}
	
	const kernel_entry* get_kernel_entry(const compute_device& dev) const override;
	
protected:
	mutable kernel_map_type kernels;
	
	typename kernel_map_type::iterator get_kernel(const compute_queue& queue) const;
	
	COMPUTE_TYPE get_compute_type() const override { return COMPUTE_TYPE::VULKAN; }
	
	shared_ptr<vulkan_encoder> create_encoder(const compute_queue& queue,
											  void* cmd_buffer,
											  const VkPipeline pipeline,
											  const VkPipelineLayout pipeline_layout,
											  const vector<const vulkan_kernel_entry*>& entries,
											  bool& success) const;
	VkPipeline get_pipeline_spec(const vulkan_device& device,
								 vulkan_kernel_entry& entry,
								 const uint3& work_group_size) const;
	
	void draw_internal(shared_ptr<vulkan_encoder> encoder,
					   const compute_queue& cqueue,
					   const vulkan_kernel_entry* vs_entry,
					   const vulkan_kernel_entry* fs_entry,
					   vector<shared_ptr<compute_buffer>>& retained_buffers,
					   const vector<multi_draw_entry>* draw_entries,
					   const vector<multi_draw_indexed_entry>* draw_indexed_entries) const;
	
	//! returns the entry for the current indices and makes sure that stage_input args are ignored
	const vulkan_kernel_entry* arg_pre_handler(const vector<const vulkan_kernel_entry*>& entries,
											   idx_handler& idx) const;
	
	//! terminator
	floor_inline_always void set_arguments(vulkan_encoder*,
										   const vector<const vulkan_kernel_entry*>&,
										   idx_handler&) const {}
	
	//! set argument and recurse
	template <typename T, typename... Args>
	floor_inline_always void set_arguments(vulkan_encoder* encoder,
										   const vector<const vulkan_kernel_entry*>& entries,
										   idx_handler& idx,
										   T&& arg, Args&&... args) const {
		auto entry = arg_pre_handler(entries, idx);
		set_argument(encoder, *entry, idx, forward<T>(arg));
		set_arguments(encoder, entries, idx, forward<Args>(args)...);
	}
	
	// actual argument setters
	template <typename T, enable_if_t<!is_pointer<decay_t<T>>::value>* = nullptr>
	floor_inline_always void set_argument(vulkan_encoder* encoder,
										  const vulkan_kernel_entry& entry,
										  idx_handler& idx,
										  T&& arg) const {
		set_argument(encoder, entry, idx, &arg, sizeof(T));
	}
	
	void set_argument(vulkan_encoder* encoder,
					  const vulkan_kernel_entry& entry,
					  idx_handler& idx,
					  const void* ptr, const size_t& size) const;
	
	floor_inline_always void set_argument(vulkan_encoder* encoder,
										  const vulkan_kernel_entry& entry,
										  idx_handler& idx,
										  shared_ptr<compute_buffer> arg) const {
		set_argument(encoder, entry, idx, arg.get());
	}
	
	floor_inline_always void set_argument(vulkan_encoder* encoder,
										  const vulkan_kernel_entry& entry,
										  idx_handler& idx,
										  shared_ptr<compute_image> arg) const {
		set_argument(encoder, entry, idx, arg.get());
	}
	
	void set_argument(vulkan_encoder* encoder,
					  const vulkan_kernel_entry& entry,
					  idx_handler& idx,
					  const compute_buffer* arg) const;
	
	void set_argument(vulkan_encoder* encoder,
					  const vulkan_kernel_entry& entry,
					  idx_handler& idx,
					  const compute_image* arg) const;
	
	void set_argument(vulkan_encoder* encoder,
					  const vulkan_kernel_entry& entry,
					  idx_handler& idx,
					  vector<shared_ptr<compute_image>>& arg) const;
	void set_argument(vulkan_encoder* encoder,
					  const vulkan_kernel_entry& entry,
					  idx_handler& idx,
					  vector<const compute_image*>& arg) const;
	
};

#endif

#endif
