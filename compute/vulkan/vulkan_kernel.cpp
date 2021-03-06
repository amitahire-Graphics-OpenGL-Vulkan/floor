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

#include <floor/compute/vulkan/vulkan_kernel.hpp>

#if !defined(FLOOR_NO_VULKAN)
#include <floor/compute/vulkan/vulkan_common.hpp>
#include <floor/compute/vulkan/vulkan_queue.hpp>
#include <floor/compute/vulkan/vulkan_device.hpp>

struct vulkan_kernel::vulkan_encoder {
	vulkan_queue::command_buffer cmd_buffer;
	const vulkan_queue& cqueue;
	const vulkan_device& device;
	vector<VkWriteDescriptorSet> write_descs;
	vector<shared_ptr<compute_buffer>> constant_buffers;
	vector<uint32_t> dyn_offsets;
	vector<shared_ptr<vector<VkDescriptorImageInfo>>> image_array_info;
	const VkPipeline pipeline { nullptr };
	const VkPipelineLayout pipeline_layout { nullptr };
};

uint64_t vulkan_kernel::vulkan_kernel_entry::make_spec_key(const uint3& work_group_size) {
#if defined(FLOOR_DEBUG)
	if((work_group_size.yz >= 65536u).any()) {
		log_error("work-group size is too big: %v", work_group_size);
		return 0;
	}
#endif
	return (uint64_t(work_group_size.x) << 32ull |
			uint64_t(work_group_size.y) << 16ull |
			uint64_t(work_group_size.z));
}

vulkan_kernel::vulkan_kernel_entry::spec_entry* vulkan_kernel::vulkan_kernel_entry::specialize(const vulkan_device& device,
																							   const uint3& work_group_size) {
	const auto spec_key = vulkan_kernel_entry::make_spec_key(work_group_size);
	const auto iter = specializations.find(spec_key);
	if(iter != specializations.end()) {
		// already built this
		return &iter->second;
	}
	
	// work-group size specialization
	static constexpr const uint32_t spec_entry_count = 3;
	
	vulkan_kernel::vulkan_kernel_entry::spec_entry spec_entry;
	spec_entry.data.resize(spec_entry_count);
	spec_entry.data[0] = work_group_size.x;
	spec_entry.data[1] = work_group_size.y;
	spec_entry.data[2] = work_group_size.z;
	
	spec_entry.map_entries = {
		{ .constantID = 1, .offset = sizeof(uint32_t) * 0, .size = sizeof(uint32_t) },
		{ .constantID = 2, .offset = sizeof(uint32_t) * 1, .size = sizeof(uint32_t) },
		{ .constantID = 3, .offset = sizeof(uint32_t) * 2, .size = sizeof(uint32_t) },
	};
	
	spec_entry.info = VkSpecializationInfo {
		.mapEntryCount = uint32_t(spec_entry.map_entries.size()),
		.pMapEntries = spec_entry.map_entries.data(),
		.dataSize = spec_entry.data.size() * sizeof(decltype(spec_entry.data)::value_type),
		.pData = spec_entry.data.data(),
	};
	stage_info.pSpecializationInfo = &spec_entry.info;
	
	// create the compute pipeline for this kernel + device + work-group size
	const VkComputePipelineCreateInfo pipeline_info {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = stage_info,
		.layout = pipeline_layout,
		.basePipelineHandle = nullptr,
		.basePipelineIndex = 0,
	};
	log_debug("specializing %s for %v ...", info->name, work_group_size); logger::flush();
	VK_CALL_RET(vkCreateComputePipelines(device.device, nullptr, 1, &pipeline_info, nullptr,
										 &spec_entry.pipeline),
				"failed to create compute pipeline (" + info->name + ", " + work_group_size.to_string() + ")",
				nullptr)
	
	auto spec_iter = specializations.insert(spec_key, spec_entry);
	if(!spec_iter.first) return nullptr;
	return &spec_iter.second->second;
}

vulkan_kernel::vulkan_kernel(kernel_map_type&& kernels_) : kernels(move(kernels_)) {
}

typename vulkan_kernel::kernel_map_type::iterator vulkan_kernel::get_kernel(const compute_queue& cqueue) const {
	return kernels.find((const vulkan_device&)cqueue.get_device());
}

shared_ptr<vulkan_kernel::vulkan_encoder> vulkan_kernel::create_encoder(const compute_queue& cqueue,
																		void* cmd_buffer_,
																		const VkPipeline pipeline,
																		const VkPipelineLayout pipeline_layout,
																		const vector<const vulkan_kernel_entry*>& entries,
																		bool& success) const {
	success = false;
	if(entries.empty()) return {};
	
	// create a command buffer if none was specified
	vulkan_queue::command_buffer cmd_buffer;
	if(cmd_buffer_ == nullptr) {
		cmd_buffer = ((const vulkan_queue&)cqueue).make_command_buffer("encoder");
		if(cmd_buffer.cmd_buffer == nullptr) return {}; // just abort
		
		// begin recording
		const VkCommandBufferBeginInfo begin_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};
		VK_CALL_RET(vkBeginCommandBuffer(cmd_buffer.cmd_buffer, &begin_info),
					"failed to begin command buffer", {})
	}
	else {
		cmd_buffer = *(vulkan_queue::command_buffer*)cmd_buffer_;
	}
	
	vkCmdBindPipeline(cmd_buffer.cmd_buffer,
					  entries[0]->stage_info.stage == VK_SHADER_STAGE_COMPUTE_BIT ?
					  VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS,
					  pipeline);
	
	const auto& vk_dev = (const vulkan_device&)cqueue.get_device();
	auto encoder = make_shared<vulkan_encoder>(vulkan_encoder {
		.cmd_buffer = cmd_buffer,
		.cqueue = (const vulkan_queue&)cqueue,
		.device = vk_dev,
		.pipeline = pipeline,
		.pipeline_layout = pipeline_layout,
	});
	
	// allocate #args write descriptor sets + 1 for the fixed sampler set
	// NOTE: any stage_input arguments have to be ignored
	size_t arg_count = 1;
	for(const auto& entry : entries) {
		if(entry == nullptr) continue;
		for(const auto& arg : entry->info->args) {
			if(arg.special_type != llvm_toolchain::function_info::SPECIAL_TYPE::STAGE_INPUT) {
				++arg_count;
				
				// +1 for read/write images
				if(arg.image_type != llvm_toolchain::function_info::ARG_IMAGE_TYPE::NONE &&
				   arg.image_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
					++arg_count;
				}
			}
		}
	}
	encoder->write_descs.resize(arg_count);
	
	// fixed sampler set
	{
		auto& write_desc = encoder->write_descs[0];
		write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc.pNext = nullptr;
		write_desc.dstSet = vk_dev.fixed_sampler_desc_set;
		write_desc.dstBinding = 0;
		write_desc.dstArrayElement = 0;
		write_desc.descriptorCount = (uint32_t)vk_dev.fixed_sampler_set.size();
		write_desc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write_desc.pImageInfo = vk_dev.fixed_sampler_image_info.data();
		write_desc.pBufferInfo = nullptr;
		write_desc.pTexelBufferView = nullptr;
	}
	
	success = true;
	return encoder;
}

VkPipeline vulkan_kernel::get_pipeline_spec(const vulkan_device& device,
											vulkan_kernel_entry& entry,
											const uint3& work_group_size) const {
	// try to find a pipeline that has already been built/specialized for this work-group size
	const auto spec_key = vulkan_kernel_entry::make_spec_key(work_group_size);
	const auto iter = entry.specializations.find(spec_key);
	if(iter != entry.specializations.end()) {
		return iter->second.pipeline;
	}
	
	// not built/specialized yet, do so now
	const auto spec_entry = entry.specialize(device, work_group_size);
	if(spec_entry == nullptr) {
		log_error("run-time specialization of kernel %s with work-group size %v failed",
				  entry.info->name, work_group_size);
		return entry.specializations.begin()->second.pipeline;
	}
	return spec_entry->pipeline;
}

void vulkan_kernel::execute(const compute_queue& cqueue,
							const bool& is_cooperative,
							const uint32_t& dim floor_unused,
							const uint3& global_work_size,
							const uint3& local_work_size_,
							const vector<compute_kernel_arg>& args) const {
	// no cooperative support yet
	if (is_cooperative) {
		log_error("cooperative kernel execution is not supported for Vulkan");
		return;
	}
	
	// find entry for queue device
	const auto kernel_iter = get_kernel(cqueue);
	if(kernel_iter == kernels.cend()) {
		log_error("no kernel for this compute queue/device exists!");
		return;
	}
	
	// check work size
	const uint3 block_dim = check_local_work_size(kernel_iter->second, local_work_size_);
	
	const uint3 grid_dim_overflow {
		global_work_size.x > 0 ? std::min(uint32_t(global_work_size.x % block_dim.x), 1u) : 0u,
		global_work_size.y > 0 ? std::min(uint32_t(global_work_size.y % block_dim.y), 1u) : 0u,
		global_work_size.z > 0 ? std::min(uint32_t(global_work_size.z % block_dim.z), 1u) : 0u
	};
	uint3 grid_dim { (global_work_size / block_dim) + grid_dim_overflow };
	grid_dim.max(1u);
	
	// create command buffer ("encoder") for this kernel execution
	const vector<const vulkan_kernel_entry*> shader_entries {
		&kernel_iter->second
	};
	bool encoder_success = false;
	auto encoder = create_encoder(cqueue, nullptr,
								  get_pipeline_spec(kernel_iter->first, kernel_iter->second, block_dim),
								  kernel_iter->second.pipeline_layout,
								  shader_entries, encoder_success);
	if(!encoder_success) {
		log_error("failed to create vulkan encoder / command buffer for kernel \"%s\"", kernel_iter->second.info->name);
		return;
	}
	
	// set and handle arguments
	idx_handler idx;
	for (const auto& arg : args) {
		auto entry = arg_pre_handler(shader_entries, idx);
		if (auto buf_ptr = get_if<const compute_buffer*>(&arg.var)) {
			set_argument(encoder.get(), *entry, idx, *buf_ptr);
		} else if (auto img_ptr = get_if<const compute_image*>(&arg.var)) {
			set_argument(encoder.get(), *entry, idx, *img_ptr);
		} else if (auto vec_img_ptrs = get_if<const vector<compute_image*>*>(&arg.var)) {
			set_argument(encoder.get(), *entry, idx, **vec_img_ptrs);
		} else if (auto vec_img_sptrs = get_if<const vector<shared_ptr<compute_image>>*>(&arg.var)) {
			set_argument(encoder.get(), *entry, idx, **vec_img_sptrs);
		} else if (auto generic_arg_ptr = get_if<const void*>(&arg.var)) {
			set_argument(encoder.get(), *entry, idx, *generic_arg_ptr, arg.size);
		} else {
			log_error("encountered invalid arg");
			return;
		}
	}
	
	// run
	const auto& entry = kernel_iter->second;
	const auto& vk_dev = (const vulkan_device&)cqueue.get_device();
	
	// set/write/update descriptors
	vkUpdateDescriptorSets(vk_dev.device,
						   (uint32_t)encoder->write_descs.size(), encoder->write_descs.data(),
						   // never copy (bad for performance)
						   0, nullptr);
	
	// final desc set binding after all parameters have been updated/set
	const VkDescriptorSet desc_sets[2] {
		vk_dev.fixed_sampler_desc_set,
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
	VK_CALL_RET(vkEndCommandBuffer(encoder->cmd_buffer.cmd_buffer), "failed to end command buffer")
	((const vulkan_queue&)cqueue).submit_command_buffer(encoder->cmd_buffer,
														[encoder](const vulkan_queue::command_buffer&) {
															// -> completion handler
															
															// kill constant buffers after the kernel has finished execution
															encoder->constant_buffers.clear();
														});
}

void vulkan_kernel::draw_internal(shared_ptr<vulkan_encoder> encoder,
								  const compute_queue& cqueue,
								  const vulkan_kernel_entry* vs_entry,
								  const vulkan_kernel_entry* fs_entry,
								  vector<shared_ptr<compute_buffer>>& retained_buffers,
								  const vector<multi_draw_entry>* draw_entries,
								  const vector<multi_draw_indexed_entry>* draw_indexed_entries) const {
	const auto& vk_dev = (const vulkan_device&)cqueue.get_device();
	
	// set/write/update descriptors
	vkUpdateDescriptorSets(vk_dev.device,
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
		vk_dev.fixed_sampler_desc_set,
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

const vulkan_kernel::vulkan_kernel_entry* vulkan_kernel::arg_pre_handler(const vector<const vulkan_kernel_entry*>& entries,
																		 idx_handler& idx) const {
	// make sure we have a usable entry
	const vulkan_kernel_entry* entry = nullptr;
	for(;;) {
		// get the next non-nullptr entry or use the current one if it's valid
		while(entries[idx.entry] == nullptr) {
			++idx.entry;
#if defined(FLOOR_DEBUG)
			if(idx.entry >= entries.size()) {
				log_error("shader/kernel entry out of bounds");
				return nullptr;
			}
#endif
		}
		entry = entries[idx.entry];
		
		// ignore any stage input args
		while(idx.arg < entry->info->args.size() &&
			  entry->info->args[idx.arg].special_type == llvm_toolchain::function_info::SPECIAL_TYPE::STAGE_INPUT) {
			++idx.arg;
		}
		
		// have all args been specified for this entry?
		if(idx.arg >= entry->info->args.size()) {
			// get the next entry
			++idx.entry;
			// reset
			idx.arg = 0;
			idx.binding = 0;
			continue;
		}
		break;
	}
	return entry;
}

void vulkan_kernel::set_argument(vulkan_encoder* encoder,
								 const vulkan_kernel_entry& entry,
								 idx_handler& idx,
								 const void* ptr, const size_t& size) const {
	// TODO: it would probably be better to allocate just one buffer, then use an offset/range for each argument
	// TODO: current limitation of this is that size must be a multiple of 4
	shared_ptr<compute_buffer> constant_buffer = make_shared<vulkan_buffer>(encoder->cqueue, size, ptr,
																			COMPUTE_MEMORY_FLAG::READ |
																			COMPUTE_MEMORY_FLAG::HOST_WRITE);
	encoder->constant_buffers.emplace_back(constant_buffer);
	set_argument(encoder, entry, idx, constant_buffer.get());
}

void vulkan_kernel::set_argument(vulkan_encoder* encoder,
								 const vulkan_kernel_entry& entry,
								 idx_handler& idx,
								 const compute_buffer* arg) const {
	auto& write_desc = encoder->write_descs[idx.write_desc];
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.pNext = nullptr;
	write_desc.dstSet = entry.desc_set;
	write_desc.dstBinding = idx.binding;
	write_desc.dstArrayElement = 0;
	write_desc.descriptorCount = 1;
	write_desc.descriptorType = entry.desc_types[idx.binding];
	write_desc.pImageInfo = nullptr;
	write_desc.pBufferInfo = ((const vulkan_buffer*)arg)->get_vulkan_buffer_info();
	write_desc.pTexelBufferView = nullptr;
	
	// always offset 0 for now
	encoder->dyn_offsets.emplace_back(0);
	
	idx.next();
}

void vulkan_kernel::set_argument(vulkan_encoder* encoder,
								 const vulkan_kernel_entry& entry,
								 idx_handler& idx,
								 const compute_image* arg) const {
	auto vk_img = static_cast<vulkan_image*>(const_cast<compute_image*>(arg));
	
	// transition image to appropriate layout
	const auto img_access = entry.info->args[idx.arg].image_access;
	if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::WRITE ||
	   img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
		vk_img->transition_write(encoder->cqueue, encoder->cmd_buffer.cmd_buffer,
								 // also readable?
								 img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE);
	}
	else { // READ
		vk_img->transition_read(encoder->cqueue, encoder->cmd_buffer.cmd_buffer);
	}
	
	// read image desc/obj
	if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ ||
	   img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
		auto& write_desc = encoder->write_descs[idx.write_desc];
		write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc.pNext = nullptr;
		write_desc.dstSet = entry.desc_set;
		write_desc.dstBinding = idx.binding;
		write_desc.dstArrayElement = 0;
		write_desc.descriptorCount = 1;
		write_desc.descriptorType = entry.desc_types[idx.binding];
		write_desc.pImageInfo = vk_img->get_vulkan_image_info();
		write_desc.pBufferInfo = nullptr;
		write_desc.pTexelBufferView = nullptr;
	}
	
	// write image descs/objs
	if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::WRITE ||
	   img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
		if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
			// next, but still the same arg
			++idx.write_desc;
			++idx.binding;
		}
		
		const auto& mip_info = vk_img->get_vulkan_mip_map_image_info();
		
		auto& write_desc = encoder->write_descs[idx.write_desc];
		write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc.pNext = nullptr;
		write_desc.dstSet = entry.desc_set;
		write_desc.dstBinding = idx.binding;
		write_desc.dstArrayElement = 0;
		write_desc.descriptorCount = uint32_t(mip_info.size());
		write_desc.descriptorType = entry.desc_types[idx.binding];
		write_desc.pImageInfo = mip_info.data();
		write_desc.pBufferInfo = nullptr;
		write_desc.pTexelBufferView = nullptr;
	}
	
	idx.next();
}

template <typename T, typename F>
floor_inline_always static void set_image_array_argument(vulkan_kernel::vulkan_encoder* encoder,
														 const vulkan_kernel::vulkan_kernel_entry& entry,
														 vulkan_kernel::idx_handler& idx,
														 vector<T>& image_array, F&& image_accessor) {
	// TODO: write/read-write array support
	
	// transition images to appropriate layout
	const auto img_access = entry.info->args[idx.arg].image_access;
	if(img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::WRITE ||
	   img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE) {
		for(auto& img : image_array) {
			image_accessor(img)->transition_write(encoder->cqueue, encoder->cmd_buffer.cmd_buffer,
												  // also readable?
												  img_access == llvm_toolchain::function_info::ARG_IMAGE_ACCESS::READ_WRITE);
		}
	}
	else { // READ
		for(auto& img : image_array) {
			image_accessor(img)->transition_read(encoder->cqueue, encoder->cmd_buffer.cmd_buffer);
		}
	}
	
	//
	const auto elem_count = entry.info->args[idx.arg].size;
#if defined(FLOOR_DEBUG)
	if(elem_count != image_array.size()) {
		log_error("invalid image array: expected %u elements, got %u elements", elem_count, image_array.size());
		return;
	}
#endif
	
	// need to heap allocate this, because the actual write/update will happen later
	auto image_info = make_shared<vector<VkDescriptorImageInfo>>(elem_count);
	for(uint32_t i = 0; i < elem_count; ++i) {
		memcpy(&(*image_info)[i], image_accessor(image_array[i])->get_vulkan_image_info(), sizeof(VkDescriptorImageInfo));
	}
	encoder->image_array_info.emplace_back(image_info);
	
	auto& write_desc = encoder->write_descs[idx.write_desc];
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.pNext = nullptr;
	write_desc.dstSet = entry.desc_set;
	write_desc.dstBinding = idx.binding;
	write_desc.dstArrayElement = 0;
	write_desc.descriptorCount = elem_count;
	write_desc.descriptorType = entry.desc_types[idx.binding];
	write_desc.pImageInfo = image_info->data();
	write_desc.pBufferInfo = nullptr;
	write_desc.pTexelBufferView = nullptr;
	
	idx.next();
}

void vulkan_kernel::set_argument(vulkan_encoder* encoder,
								 const vulkan_kernel_entry& entry,
								 idx_handler& idx,
								 vector<shared_ptr<compute_image>>& arg) const {
	set_image_array_argument(encoder, entry, idx, arg, [](shared_ptr<compute_image>& img) { return (vulkan_image*)img.get(); });
}

void vulkan_kernel::set_argument(vulkan_encoder* encoder,
								 const vulkan_kernel_entry& entry,
								 idx_handler& idx,
								 vector<const compute_image*>& arg) const {
	set_image_array_argument(encoder, entry, idx, arg, [](const compute_image* img) {
		return static_cast<vulkan_image*>(const_cast<compute_image*>(img));
	});
}

const compute_kernel::kernel_entry* vulkan_kernel::get_kernel_entry(const compute_device& dev) const {
	const auto ret = kernels.get((const vulkan_device&)dev);
	return !ret.first ? nullptr : &ret.second->second;
}

#endif
