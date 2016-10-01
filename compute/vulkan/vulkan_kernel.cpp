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

#include <floor/compute/vulkan/vulkan_kernel.hpp>

#if !defined(FLOOR_NO_VULKAN)
#include <floor/compute/vulkan/vulkan_common.hpp>
#include <floor/compute/vulkan/vulkan_queue.hpp>
#include <floor/compute/vulkan/vulkan_device.hpp>

struct vulkan_kernel::vulkan_encoder {
	vulkan_queue::command_buffer cmd_buffer;
	vulkan_device* device;
	vector<VkWriteDescriptorSet> write_descs;
	vector<shared_ptr<compute_buffer>> constant_buffers;
	vector<uint32_t> dyn_offsets;
	const VkPipeline pipeline { nullptr };
	const VkPipelineLayout pipeline_layout { nullptr };
};

vulkan_kernel::vulkan_kernel(kernel_map_type&& kernels_) : kernels(move(kernels_)) {
}

vulkan_kernel::~vulkan_kernel() {}

typename vulkan_kernel::kernel_map_type::const_iterator vulkan_kernel::get_kernel(const compute_queue* queue) const {
	return kernels.find((vulkan_device*)queue->get_device().get());
}

shared_ptr<vulkan_kernel::vulkan_encoder> vulkan_kernel::create_encoder(compute_queue* queue,
																		const vulkan_kernel_entry& entry,
																		bool& success) {
	success = false;
	auto vk_dev = (vulkan_device*)queue->get_device().get();
	auto encoder = make_shared<vulkan_encoder>(vulkan_encoder {
		.cmd_buffer = ((vulkan_queue*)queue)->make_command_buffer("kernel encoder"),
		.device = vk_dev,
	});
	
	if(encoder->cmd_buffer.cmd_buffer == nullptr) {
		return {};
	}

	// begin recording
	const VkCommandBufferBeginInfo begin_info {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	VK_CALL_RET(vkBeginCommandBuffer(encoder->cmd_buffer.cmd_buffer, &begin_info),
				"failed to begin command buffer", {});
	
	vkCmdBindPipeline(encoder->cmd_buffer.cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, entry.pipeline);
	
	// allocate #args write descriptor sets + 1 for the fixed sampler set
	encoder->write_descs.resize(entry.info->args.size() + 1);
	
	// fixed sampler set
	{
		auto& write_desc = encoder->write_descs[0];
		write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc.pNext = nullptr;
		write_desc.dstSet = vk_dev->fixed_sampler_desc_set;
		write_desc.dstBinding = 0;
		write_desc.dstArrayElement = 0;
		write_desc.descriptorCount = (uint32_t)vk_dev->fixed_sampler_set.size();
		write_desc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write_desc.pImageInfo = vk_dev->fixed_sampler_image_info.data();
		write_desc.pBufferInfo = nullptr;
		write_desc.pTexelBufferView = nullptr;
	}

	success = true;
	return encoder;
}

void vulkan_kernel::execute_internal(shared_ptr<vulkan_encoder> encoder,
									 compute_queue* queue,
									 const vulkan_kernel_entry& entry,
									 const uint32_t&,
									 const uint3& grid_dim,
									 const uint3& block_dim floor_unused /* unused for now, until dyn local size is possible */) const {
	auto vk_dev = (vulkan_device*)queue->get_device().get();
	
	// set/write/update descriptors
	vkUpdateDescriptorSets(vk_dev->device,
						   (uint32_t)encoder->write_descs.size(), encoder->write_descs.data(),
						   // never copy (bad for performance)
						   0, nullptr);
	
	// final desc set binding after all parameters have been updated/set
	const VkDescriptorSet desc_sets[2] {
		vk_dev->fixed_sampler_desc_set,
		entry.desc_set,
	};
	vkCmdBindDescriptorSets(encoder->cmd_buffer.cmd_buffer,
							VK_PIPELINE_BIND_POINT_COMPUTE,
							entry.pipeline_layout,
							0,
							(entry.desc_set != nullptr ? 2 : 1),
							desc_sets,
							(uint32_t)encoder->dyn_offsets.size(),
							encoder->dyn_offsets.data());
	
	// set dims + pipeline
	// TODO: check if grid_dim matches compute shader defintion
	vkCmdDispatch(encoder->cmd_buffer.cmd_buffer, grid_dim.x, grid_dim.y, grid_dim.z);
	
	// all done here, end + submit
	VK_CALL_RET(vkEndCommandBuffer(encoder->cmd_buffer.cmd_buffer), "failed to end command buffer");
	((vulkan_queue*)queue)->submit_command_buffer(encoder->cmd_buffer,
												  [encoder](const vulkan_queue::command_buffer&) {
		// -> completion handler
		
		// kill constant buffers after the kernel has finished execution
		encoder->constant_buffers.clear();
	});
}

shared_ptr<vulkan_kernel::vulkan_encoder> vulkan_kernel::create_encoder(compute_queue* queue,
																		void* cmd_buffer,
																		const VkPipeline pipeline,
																		const VkPipelineLayout pipeline_layout,
																		const vulkan_kernel_entry* vs_entry,
																		const vulkan_kernel_entry* fs_entry,
																		bool& success) {
	success = false;
	auto vk_dev = (vulkan_device*)queue->get_device().get();
	auto encoder = make_shared<vulkan_encoder>(vulkan_encoder {
		.cmd_buffer = *(vulkan_queue::command_buffer*)cmd_buffer,
		.device = vk_dev,
		.pipeline = pipeline,
		.pipeline_layout = pipeline_layout,
	});
	
	vkCmdBindPipeline(encoder->cmd_buffer.cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	
	// allocate #args write descriptor sets + 1 for the fixed sampler set
	// NOTE: any stage_input arguments have to be ignored
	size_t arg_count = 1;
	for(const auto& arg : vs_entry->info->args) {
		if(arg.special_type != llvm_toolchain::function_info::SPECIAL_TYPE::STAGE_INPUT) {
			++arg_count;
		}
	}
	if(fs_entry != nullptr) {
		for(const auto& arg : fs_entry->info->args) {
			if(arg.special_type != llvm_toolchain::function_info::SPECIAL_TYPE::STAGE_INPUT) {
				++arg_count;
			}
		}
	}
	encoder->write_descs.resize(arg_count);
	
	// fixed sampler set
	{
		auto& write_desc = encoder->write_descs[0];
		write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc.pNext = nullptr;
		write_desc.dstSet = vk_dev->fixed_sampler_desc_set;
		write_desc.dstBinding = 0;
		write_desc.dstArrayElement = 0;
		write_desc.descriptorCount = (uint32_t)vk_dev->fixed_sampler_set.size();
		write_desc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write_desc.pImageInfo = vk_dev->fixed_sampler_image_info.data();
		write_desc.pBufferInfo = nullptr;
		write_desc.pTexelBufferView = nullptr;
	}
	
	success = true;
	return encoder;
}

void vulkan_kernel::draw_internal(shared_ptr<vulkan_encoder> encoder,
								  compute_queue* queue,
								  const vulkan_kernel_entry* vs_entry,
								  const vulkan_kernel_entry* fs_entry,
								  vector<shared_ptr<compute_buffer>>& retained_buffers,
								  const vector<multi_draw_entry>* draw_entries,
								  const vector<multi_draw_indexed_entry>* draw_indexed_entries) const {
	auto vk_dev = (vulkan_device*)queue->get_device().get();
	
	// set/write/update descriptors
	vkUpdateDescriptorSets(vk_dev->device,
						   (uint32_t)encoder->write_descs.size(), encoder->write_descs.data(),
						   // never copy (bad for performance)
						   0, nullptr);
	
	// final desc set binding after all parameters have been updated/set
	// note that we need to take care of the situation where the vertex shader doesn't have a desc set,
	// but the fragment shader does -> binding discontiguous sets is not directly possible
	const bool has_vs_desc = (vs_entry->desc_set != nullptr);
	const bool has_fs_desc = (fs_entry != nullptr && fs_entry->desc_set != nullptr);
	const bool discontiguous = (!has_vs_desc && has_fs_desc);
	
	const array<VkDescriptorSet, 3> desc_sets {{
		vk_dev->fixed_sampler_desc_set,
		vs_entry->desc_set,
		has_fs_desc ? fs_entry->desc_set : nullptr,
	}};
	// either binds everything or just the fixed sampler set
	vkCmdBindDescriptorSets(encoder->cmd_buffer.cmd_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							encoder->pipeline_layout,
							0,
							(discontiguous || !has_vs_desc) ? 1 : (!has_fs_desc ? 2 : 3),
							desc_sets.data(),
							// don't want to set dyn offsets when only binding the fixed sampler set
							discontiguous ? 0 : uint32_t(encoder->dyn_offsets.size()),
							discontiguous ? nullptr : encoder->dyn_offsets.data());
	
	// bind fs set
	if(discontiguous) {
		vkCmdBindDescriptorSets(encoder->cmd_buffer.cmd_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								encoder->pipeline_layout,
								2,
								1,
								&fs_entry->desc_set,
								(uint32_t)encoder->dyn_offsets.size(),
								encoder->dyn_offsets.data());
	}
	
	if(draw_entries != nullptr) {
		for(const auto& entry : *draw_entries) {
			vkCmdDraw(encoder->cmd_buffer.cmd_buffer, entry.vertex_count, entry.instance_count,
					  entry.first_vertex, entry.first_instance);
		}
	}
	if(draw_indexed_entries != nullptr) {
		for(const auto& entry : *draw_indexed_entries) {
			vkCmdBindIndexBuffer(encoder->cmd_buffer.cmd_buffer, ((vulkan_buffer*)entry.index_buffer)->get_vulkan_buffer(),
								 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(encoder->cmd_buffer.cmd_buffer, entry.index_count, entry.instance_count, entry.first_index,
							 entry.vertex_offset, entry.first_instance);
		}
	}
	
	// NOTE: caller will end command buffer
	
	// TODO: properly kill constant_buffers !!!
	retained_buffers.insert(retained_buffers.end(), encoder->constant_buffers.begin(), encoder->constant_buffers.end());
}

const vulkan_kernel::vulkan_kernel_entry* vulkan_kernel::handle_shader_arg(const vulkan_kernel_entry* vs_entry,
																		   const vulkan_kernel_entry* fs_entry,
																		   const uint32_t num,
																		   uint32_t& shader_arg_idx,
																		   uint32_t& binding_idx) const {
	const auto vs_arg_count = vs_entry->info->args.size();
	const auto is_fs = (num >= vs_arg_count && fs_entry != nullptr);
	
	// TODO: ideally, we want to skip stage_input for vertex shaders as well -> handle the vs to fs arg transition better
	if(num == vs_arg_count) {
		shader_arg_idx = 0;
		binding_idx = 0;
	}
	
	// skip stage_input arguments in fragment shaders
	if(is_fs) {
		for(;;) {
			if(fs_entry->info->args[shader_arg_idx].special_type == llvm_toolchain::function_info::SPECIAL_TYPE::STAGE_INPUT) {
				++shader_arg_idx;
				continue;
			}
			break;
		}
	}
	
	return (!is_fs ? vs_entry : fs_entry);
}

void vulkan_kernel::set_kernel_argument(vulkan_encoder* encoder, const vulkan_kernel_entry& entry,
										const uint32_t num, const uint32_t arg_idx, const uint32_t binding_idx,
										const void* ptr, const size_t& size) const {
	// TODO: it would probably be better to allocate just one buffer, then use an offset/range for each argument
	// TODO: current limitation of this is that size must be a multiple of 4
	shared_ptr<compute_buffer> constant_buffer = make_shared<vulkan_buffer>(encoder->device, size, (void*)ptr,
																			COMPUTE_MEMORY_FLAG::READ |
																			COMPUTE_MEMORY_FLAG::HOST_WRITE);
	encoder->constant_buffers.push_back(constant_buffer);
	set_kernel_argument(encoder, entry, num, arg_idx, binding_idx, constant_buffer.get());
}

void vulkan_kernel::set_kernel_argument(vulkan_encoder* encoder,
										const vulkan_kernel_entry& entry,
										const uint32_t num, const uint32_t arg_idx floor_unused, const uint32_t binding_idx,
										const compute_buffer* arg) const {
	auto& write_desc = encoder->write_descs[num + 1];
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.pNext = nullptr;
	write_desc.dstSet = entry.desc_set;
	write_desc.dstBinding = binding_idx;
	write_desc.dstArrayElement = 0;
	write_desc.descriptorCount = 1;
	write_desc.descriptorType = entry.desc_types[binding_idx];
	write_desc.pImageInfo = nullptr;
	write_desc.pBufferInfo = ((vulkan_buffer*)arg)->get_vulkan_buffer_info();
	write_desc.pTexelBufferView = nullptr;
	
	// always offset 0 for now
	encoder->dyn_offsets.emplace_back(0);
}

void vulkan_kernel::set_kernel_argument(vulkan_encoder* encoder,
										const vulkan_kernel_entry& entry,
										const uint32_t num, const uint32_t arg_idx, const uint32_t binding_idx,
										const compute_image* arg) const {
	auto vk_img = (vulkan_image*)arg;
	
	// transition image to appropriate layout
	const auto img_access = entry.info->args[arg_idx].image_access;
	if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::WRITE ||
	   img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
		// TODO: might need to handle read/write differently
		vk_img->transition_write(encoder->cmd_buffer.cmd_buffer);
	}
	else { // READ
		vk_img->transition_read(encoder->cmd_buffer.cmd_buffer);
	}
	
	auto& write_desc = encoder->write_descs[num + 1];
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.pNext = nullptr;
	write_desc.dstSet = entry.desc_set;
	write_desc.dstBinding = binding_idx;
	write_desc.dstArrayElement = 0;
	write_desc.descriptorCount = 1;
	write_desc.descriptorType = entry.desc_types[binding_idx];
	write_desc.pImageInfo = vk_img->get_vulkan_image_info();
	write_desc.pBufferInfo = nullptr;
	write_desc.pTexelBufferView = nullptr;
}

#endif
