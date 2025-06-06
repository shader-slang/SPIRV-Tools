LOCAL_PATH := $(call my-dir)
SPVTOOLS_OUT_PATH=$(if $(call host-path-is-absolute,$(TARGET_OUT)),$(TARGET_OUT),$(abspath $(TARGET_OUT)))

ifeq ($(SPVHEADERS_LOCAL_PATH),)
	SPVHEADERS_LOCAL_PATH := $(LOCAL_PATH)/external/spirv-headers
endif

SPVTOOLS_SRC_FILES := \
		source/assembly_grammar.cpp \
		source/binary.cpp \
		source/diagnostic.cpp \
		source/disassemble.cpp \
		source/ext_inst.cpp \
		source/enum_string_mapping.cpp \
		source/extensions.cpp \
		source/libspirv.cpp \
		source/name_mapper.cpp \
		source/opcode.cpp \
		source/operand.cpp \
		source/parsed_operand.cpp \
		source/print.cpp \
		source/software_version.cpp \
		source/spirv_endian.cpp \
		source/spirv_optimizer_options.cpp \
		source/spirv_target_env.cpp \
		source/spirv_validator_options.cpp \
		source/table.cpp \
		source/text.cpp \
		source/text_handler.cpp \
		source/to_string.cpp \
		source/util/bit_vector.cpp \
		source/util/parse_number.cpp \
		source/util/string_utils.cpp \
		source/util/timer.cpp \
		source/val/basic_block.cpp \
		source/val/construct.cpp \
		source/val/function.cpp \
		source/val/instruction.cpp \
		source/val/validation_state.cpp \
		source/val/validate.cpp \
		source/val/validate_adjacency.cpp \
		source/val/validate_annotation.cpp \
		source/val/validate_arithmetics.cpp \
		source/val/validate_atomics.cpp \
		source/val/validate_barriers.cpp \
		source/val/validate_bitwise.cpp \
		source/val/validate_builtins.cpp \
		source/val/validate_capability.cpp \
		source/val/validate_cfg.cpp \
		source/val/validate_composites.cpp \
		source/val/validate_constants.cpp \
		source/val/validate_conversion.cpp \
		source/val/validate_debug.cpp \
		source/val/validate_decorations.cpp \
		source/val/validate_derivatives.cpp \
		source/val/validate_extensions.cpp \
		source/val/validate_execution_limitations.cpp \
		source/val/validate_function.cpp \
		source/val/validate_id.cpp \
		source/val/validate_image.cpp \
		source/val/validate_interfaces.cpp \
		source/val/validate_instruction.cpp \
		source/val/validate_memory.cpp \
		source/val/validate_memory_semantics.cpp \
		source/val/validate_mesh_shading.cpp \
		source/val/validate_misc.cpp \
		source/val/validate_mode_setting.cpp \
		source/val/validate_layout.cpp \
		source/val/validate_literals.cpp \
		source/val/validate_logicals.cpp \
		source/val/validate_non_uniform.cpp \
		source/val/validate_primitives.cpp \
		source/val/validate_ray_query.cpp \
		source/val/validate_ray_tracing.cpp \
		source/val/validate_ray_tracing_reorder.cpp \
		source/val/validate_scopes.cpp \
		source/val/validate_small_type_uses.cpp \
		source/val/validate_tensor_layout.cpp \
		source/val/validate_type.cpp\
		source/val/validate_invalid_type.cpp

SPVTOOLS_OPT_SRC_FILES := \
		source/opt/aggressive_dead_code_elim_pass.cpp \
		source/opt/amd_ext_to_khr.cpp \
		source/opt/analyze_live_input_pass.cpp \
		source/opt/basic_block.cpp \
		source/opt/block_merge_pass.cpp \
		source/opt/block_merge_util.cpp \
		source/opt/build_module.cpp \
		source/opt/cfg.cpp \
		source/opt/cfg_cleanup_pass.cpp \
		source/opt/ccp_pass.cpp \
		source/opt/code_sink.cpp \
		source/opt/combine_access_chains.cpp \
		source/opt/compact_ids_pass.cpp \
		source/opt/composite.cpp \
		source/opt/const_folding_rules.cpp \
		source/opt/constants.cpp \
		source/opt/control_dependence.cpp \
		source/opt/convert_to_sampled_image_pass.cpp \
		source/opt/convert_to_half_pass.cpp \
		source/opt/copy_prop_arrays.cpp \
		source/opt/dataflow.cpp \
		source/opt/dead_branch_elim_pass.cpp \
		source/opt/dead_insert_elim_pass.cpp \
		source/opt/dead_variable_elimination.cpp \
		source/opt/decoration_manager.cpp \
		source/opt/debug_info_manager.cpp \
		source/opt/def_use_manager.cpp \
		source/opt/desc_sroa.cpp \
		source/opt/desc_sroa_util.cpp \
		source/opt/dominator_analysis.cpp \
		source/opt/dominator_tree.cpp \
		source/opt/eliminate_dead_constant_pass.cpp \
		source/opt/eliminate_dead_functions_pass.cpp \
		source/opt/eliminate_dead_functions_util.cpp \
		source/opt/eliminate_dead_io_components_pass.cpp \
		source/opt/eliminate_dead_members_pass.cpp \
		source/opt/eliminate_dead_output_stores_pass.cpp \
		source/opt/feature_manager.cpp \
		source/opt/fix_func_call_arguments.cpp \
		source/opt/fix_storage_class.cpp \
		source/opt/flatten_decoration_pass.cpp \
		source/opt/fold.cpp \
		source/opt/folding_rules.cpp \
		source/opt/fold_spec_constant_op_and_composite_pass.cpp \
		source/opt/freeze_spec_constant_value_pass.cpp \
		source/opt/function.cpp \
		source/opt/graphics_robust_access_pass.cpp \
		source/opt/if_conversion.cpp \
		source/opt/inline_pass.cpp \
		source/opt/inline_exhaustive_pass.cpp \
		source/opt/inline_opaque_pass.cpp \
		source/opt/instruction.cpp \
		source/opt/instruction_list.cpp \
		source/opt/interface_var_sroa.cpp \
		source/opt/interp_fixup_pass.cpp \
		source/opt/invocation_interlock_placement_pass.cpp \
		source/opt/ir_context.cpp \
		source/opt/ir_loader.cpp \
		source/opt/licm_pass.cpp \
		source/opt/liveness.cpp \
		source/opt/local_access_chain_convert_pass.cpp \
		source/opt/local_redundancy_elimination.cpp \
		source/opt/local_single_block_elim_pass.cpp \
		source/opt/local_single_store_elim_pass.cpp \
		source/opt/loop_dependence.cpp \
		source/opt/loop_dependence_helpers.cpp \
		source/opt/loop_descriptor.cpp \
		source/opt/loop_fission.cpp \
		source/opt/loop_fusion.cpp \
		source/opt/loop_fusion_pass.cpp \
		source/opt/loop_peeling.cpp \
		source/opt/loop_unroller.cpp \
		source/opt/loop_unswitch_pass.cpp \
		source/opt/loop_utils.cpp \
		source/opt/mem_pass.cpp \
		source/opt/merge_return_pass.cpp \
		source/opt/modify_maximal_reconvergence.cpp \
		source/opt/module.cpp \
		source/opt/opextinst_forward_ref_fixup_pass.cpp \
		source/opt/optimizer.cpp \
		source/opt/pass.cpp \
		source/opt/pass_manager.cpp \
		source/opt/private_to_local_pass.cpp \
		source/opt/propagator.cpp \
		source/opt/reduce_load_size.cpp \
		source/opt/redundancy_elimination.cpp \
		source/opt/register_pressure.cpp \
		source/opt/relax_float_ops_pass.cpp \
		source/opt/remove_dontinline_pass.cpp \
		source/opt/remove_duplicates_pass.cpp \
		source/opt/remove_unused_interface_variables_pass.cpp \
		source/opt/replace_desc_array_access_using_var_index.cpp \
		source/opt/replace_invalid_opc.cpp \
		source/opt/resolve_binding_conflicts_pass.cpp \
		source/opt/scalar_analysis.cpp \
		source/opt/scalar_analysis_simplification.cpp \
		source/opt/scalar_replacement_pass.cpp \
		source/opt/set_spec_constant_default_value_pass.cpp \
		source/opt/simplification_pass.cpp \
		source/opt/split_combined_image_sampler_pass.cpp \
		source/opt/spread_volatile_semantics.cpp \
		source/opt/ssa_rewrite_pass.cpp \
		source/opt/strength_reduction_pass.cpp \
		source/opt/strip_debug_info_pass.cpp \
		source/opt/strip_nonsemantic_info_pass.cpp \
		source/opt/struct_cfg_analysis.cpp \
		source/opt/struct_packing_pass.cpp \
		source/opt/switch_descriptorset_pass.cpp \
		source/opt/trim_capabilities_pass.cpp \
		source/opt/type_manager.cpp \
		source/opt/types.cpp \
		source/opt/unify_const_pass.cpp \
		source/opt/upgrade_memory_model.cpp \
		source/opt/value_number_table.cpp \
		source/opt/vector_dce.cpp \
		source/opt/workaround1209.cpp \
		source/opt/wrap_opkill.cpp

# Locations of grammar files.
#
SPV_COREUNIFIED1_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/spirv.core.grammar.json
SPV_GLSL_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.glsl.std.450.grammar.json
SPV_OPENCL_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.opencl.std.100.grammar.json
SPV_DEBUGINFO_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.debuginfo.grammar.json
SPV_CLDEBUGINFO100_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json
SPV_VKDEBUGINFO100_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.nonsemantic.shader.debuginfo.100.grammar.json

define gen_spvtools_grammar_tables
$(call generate-file-dir,$(1)/core.insts-unified1.inc)
$(1)/core.insts-unified1.inc $(1)/operand.kinds-unified1.inc \
$(1)/glsl.std.450.insts.inc \
$(1)/opencl.std.insts.inc \
: \
        $(LOCAL_PATH)/utils/generate_grammar_tables.py \
        $(SPV_COREUNIFIED1_GRAMMAR) \
        $(SPV_GLSL_GRAMMAR) \
        $(SPV_OpenCL_GRAMMAR) \
        $(SPV_DEBUGINFO_GRAMMAR) \
        $(SPV_CLDEBUGINFO100_GRAMMAR)
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/generate_grammar_tables.py \
		                --spirv-core-grammar=$(SPV_COREUNIFIED1_GRAMMAR) \
		                --extinst-glsl-grammar=$(SPV_GLSL_GRAMMAR) \
		                --extinst-opencl-grammar=$(SPV_OPENCL_GRAMMAR) \
		                --extinst-debuginfo-grammar=$(SPV_DEBUGINFO_GRAMMAR) \
		                --extinst-cldebuginfo100-grammar=$(SPV_CLDEBUGINFO100_GRAMMAR) \
		                --core-insts-output=$(1)/core.insts-unified1.inc \
		                --glsl-insts-output=$(1)/glsl.std.450.insts.inc \
		                --opencl-insts-output=$(1)/opencl.std.insts.inc \
		                --operand-kinds-output=$(1)/operand.kinds-unified1.inc \
										--output-language=c++
		@echo "[$(TARGET_ARCH_ABI)] Grammar (from unified1)  : instructions & operands <= grammar JSON files"
$(LOCAL_PATH)/source/opcode.cpp: $(1)/core.insts-unified1.inc
$(LOCAL_PATH)/source/operand.cpp: $(1)/operand.kinds-unified1.inc
$(LOCAL_PATH)/source/ext_inst.cpp: \
	$(1)/glsl.std.450.insts.inc \
	$(1)/opencl.std.insts.inc \
	$(1)/debuginfo.insts.inc \
	$(1)/opencl.debuginfo.100.insts.inc \
	$(1)/nonsemantic.shader.debuginfo.100.insts.inc \
	$(1)/spv-amd-gcn-shader.insts.inc \
	$(1)/spv-amd-shader-ballot.insts.inc \
	$(1)/spv-amd-shader-explicit-vertex-parameter.insts.inc \
	$(1)/spv-amd-shader-trinary-minmax.insts.inc
$(LOCAL_PATH)/source/opt/amd_ext_to_khr.cpp: \
	$(1)/spv-amd-shader-ballot.insts.inc
endef
$(eval $(call gen_spvtools_grammar_tables,$(SPVTOOLS_OUT_PATH)))


define gen_spvtools_lang_headers
# Generate language-specific headers.  So far we only generate C headers
# $1 is the output directory.
# $2 is the base name of the header file, e.g. "DebugInfo".
# $3 is the grammar file containing token definitions.
$(call generate-file-dir,$(1)/$(2).h)
$(1)/$(2).h : \
        $(LOCAL_PATH)/utils/generate_language_headers.py \
        $(3)
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/generate_language_headers.py \
		    --extinst-grammar=$(3) \
		    --extinst-output-path=$(1)/$(2).h
		@echo "[$(TARGET_ARCH_ABI)] Generate language specific header for $(2): headers <= grammar"
$(foreach F,$(SPVTOOLS_SRC_FILES) $(SPVTOOLS_OPT_SRC_FILES),$(LOCAL_PATH)/$F ) \
	: $(1)/$(2).h
endef
# We generate language-specific headers for DebugInfo and OpenCL.DebugInfo.100
$(eval $(call gen_spvtools_lang_headers,$(SPVTOOLS_OUT_PATH),DebugInfo,$(SPV_DEBUGINFO_GRAMMAR)))
$(eval $(call gen_spvtools_lang_headers,$(SPVTOOLS_OUT_PATH),OpenCLDebugInfo100,$(SPV_CLDEBUGINFO100_GRAMMAR)))
$(eval $(call gen_spvtools_lang_headers,$(SPVTOOLS_OUT_PATH),NonSemanticShaderDebugInfo100,$(SPV_VKDEBUGINFO100_GRAMMAR)))


define gen_spvtools_vendor_tables
$(call generate-file-dir,$(1)/$(2).insts.inc)
$(1)/$(2).insts.inc : \
        $(LOCAL_PATH)/utils/generate_grammar_tables.py \
        $(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.$(2).grammar.json
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/generate_grammar_tables.py \
		    --extinst-vendor-grammar=$(SPVHEADERS_LOCAL_PATH)/include/spirv/unified1/extinst.$(2).grammar.json \
		    --vendor-insts-output=$(1)/$(2).insts.inc \
		    --vendor-operand-kind-prefix=$(3)
		@echo "[$(TARGET_ARCH_ABI)] Vendor extended instruction set: $(2) tables <= grammar"
$(LOCAL_PATH)/source/ext_inst.cpp: $(1)/$(2).insts.inc
endef
# Vendor and debug extended instruction sets, with grammars from SPIRV-Tools source tree.
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),debuginfo,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),opencl.debuginfo.100,"CLDEBUG100_"))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),nonsemantic.shader.debuginfo.100,"SHDEBUG100_"))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),spv-amd-gcn-shader,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),spv-amd-shader-ballot,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),spv-amd-shader-explicit-vertex-parameter,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),spv-amd-shader-trinary-minmax,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),nonsemantic.clspvreflection,""))
$(eval $(call gen_spvtools_vendor_tables,$(SPVTOOLS_OUT_PATH),nonsemantic.vkspreflection,""))

define gen_spvtools_enum_string_mapping
$(call generate-file-dir,$(1)/extension_enum.inc.inc)
$(1)/extension_enum.inc $(1)/enum_string_mapping.inc: \
        $(LOCAL_PATH)/utils/generate_grammar_tables.py \
        $(SPV_COREUNIFIED1_GRAMMAR)
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/generate_grammar_tables.py \
		                --spirv-core-grammar=$(SPV_COREUNIFIED1_GRAMMAR) \
		                --extinst-debuginfo-grammar=$(SPV_DEBUGINFO_GRAMMAR) \
		                --extinst-cldebuginfo100-grammar=$(SPV_CLDEBUGINFO100_GRAMMAR) \
		                --extension-enum-output=$(1)/extension_enum.inc \
		                --enum-string-mapping-output=$(1)/enum_string_mapping.inc \
										--output-language=c++
		@echo "[$(TARGET_ARCH_ABI)] Generate enum<->string mapping <= grammar JSON files"
# Generated header extension_enum.inc is transitively included by table.h, which is
# used pervasively.  Capture the pervasive dependency.
$(foreach F,$(SPVTOOLS_SRC_FILES) $(SPVTOOLS_OPT_SRC_FILES),$(LOCAL_PATH)/$F ) \
  : $(1)/extension_enum.inc
$(LOCAL_PATH)/source/enum_string_mapping.cpp: $(1)/enum_string_mapping.inc
endef
$(eval $(call gen_spvtools_enum_string_mapping,$(SPVTOOLS_OUT_PATH)))

define gen_spvtools_build_version_inc
$(call generate-file-dir,$(1)/dummy_filename)
$(1)/build-version.inc: \
        $(LOCAL_PATH)/utils/update_build_version.py \
        $(LOCAL_PATH)/CHANGES
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/update_build_version.py \
		                $(LOCAL_PATH)/CHANGES $(1)/build-version.inc
		@echo "[$(TARGET_ARCH_ABI)] Generate       : build-version.inc <= CHANGES"
$(LOCAL_PATH)/source/software_version.cpp: $(1)/build-version.inc
endef
$(eval $(call gen_spvtools_build_version_inc,$(SPVTOOLS_OUT_PATH)))

define gen_spvtools_generators_inc
$(call generate-file-dir,$(1)/dummy_filename)
$(1)/generators.inc: \
        $(LOCAL_PATH)/utils/generate_registry_tables.py \
        $(SPVHEADERS_LOCAL_PATH)/include/spirv/spir-v.xml
		@$(HOST_PYTHON) $(LOCAL_PATH)/utils/generate_registry_tables.py \
		                --xml=$(SPVHEADERS_LOCAL_PATH)/include/spirv/spir-v.xml \
				--generator-output=$(1)/generators.inc
		@echo "[$(TARGET_ARCH_ABI)] Generate       : generators.inc <= spir-v.xml"
$(LOCAL_PATH)/source/opcode.cpp: $(1)/generators.inc
endef
$(eval $(call gen_spvtools_generators_inc,$(SPVTOOLS_OUT_PATH)))

include $(CLEAR_VARS)
LOCAL_MODULE := SPIRV-Tools
LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/include \
		$(SPVHEADERS_LOCAL_PATH)/include \
		$(SPVTOOLS_OUT_PATH)
LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/include
LOCAL_CXXFLAGS:=-std=c++17 -fno-exceptions -fno-rtti -Werror
LOCAL_SRC_FILES:= $(SPVTOOLS_SRC_FILES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SPIRV-Tools-opt
LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/include \
		$(LOCAL_PATH)/source \
		$(SPVHEADERS_LOCAL_PATH)/include \
		$(SPVTOOLS_OUT_PATH)
LOCAL_CXXFLAGS:=-std=c++17 -fno-exceptions -fno-rtti -Werror
LOCAL_STATIC_LIBRARIES:=SPIRV-Tools
LOCAL_SRC_FILES:= $(SPVTOOLS_OPT_SRC_FILES)
include $(BUILD_STATIC_LIBRARY)
