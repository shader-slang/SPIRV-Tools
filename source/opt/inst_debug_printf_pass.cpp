// Copyright (c) 2020 The Khronos Group Inc.
// Copyright (c) 2020 Valve Corporation
// Copyright (c) 2020 LunarG Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "inst_debug_printf_pass.h"

#include "source/spirv_constant.h"
#include "source/to_string.h"
#include "source/util/string_utils.h"
#include "spirv/unified1/NonSemanticDebugPrintf.h"

namespace spvtools {
namespace opt {

void InstDebugPrintfPass::GenOutputValues(Instruction* val_inst,
                                          std::vector<uint32_t>* val_ids,
                                          InstructionBuilder* builder) {
  uint32_t val_ty_id = val_inst->type_id();
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  analysis::Type* val_ty = type_mgr->GetType(val_ty_id);
  switch (val_ty->kind()) {
    case analysis::Type::kVector: {
      analysis::Vector* v_ty = val_ty->AsVector();
      const analysis::Type* c_ty = v_ty->element_type();
      uint32_t c_ty_id = type_mgr->GetId(c_ty);
      for (uint32_t c = 0; c < v_ty->element_count(); ++c) {
        Instruction* c_inst =
            builder->AddCompositeExtract(c_ty_id, val_inst->result_id(), {c});
        GenOutputValues(c_inst, val_ids, builder);
      }
      return;
    }
    case analysis::Type::kBool: {
      // Select between uint32 zero or one
      uint32_t zero_id = builder->GetUintConstantId(0);
      uint32_t one_id = builder->GetUintConstantId(1);
      Instruction* sel_inst = builder->AddSelect(
          GetUintId(), val_inst->result_id(), one_id, zero_id);
      val_ids->push_back(sel_inst->result_id());
      return;
    }
    case analysis::Type::kFloat: {
      analysis::Float* f_ty = val_ty->AsFloat();
      switch (f_ty->width()) {
        case 16: {
          // Convert float16 to float32 and recurse
          Instruction* f32_inst = builder->AddUnaryOp(
              GetFloatId(), spv::Op::OpFConvert, val_inst->result_id());
          GenOutputValues(f32_inst, val_ids, builder);
          return;
        }
        case 64: {
          // Bitcast float64 to uint64 and recurse
          Instruction* ui64_inst = builder->AddUnaryOp(
              GetUint64Id(), spv::Op::OpBitcast, val_inst->result_id());
          GenOutputValues(ui64_inst, val_ids, builder);
          return;
        }
        case 32: {
          // Bitcase float32 to uint32
          Instruction* bc_inst = builder->AddUnaryOp(
              GetUintId(), spv::Op::OpBitcast, val_inst->result_id());
          val_ids->push_back(bc_inst->result_id());
          return;
        }
        default:
          assert(false && "unsupported float width");
          return;
      }
    }
    case analysis::Type::kInteger: {
      analysis::Integer* i_ty = val_ty->AsInteger();
      switch (i_ty->width()) {
        case 64: {
          Instruction* ui64_inst = val_inst;
          if (i_ty->IsSigned()) {
            // Bitcast sint64 to uint64
            ui64_inst = builder->AddUnaryOp(GetUint64Id(), spv::Op::OpBitcast,
                                            val_inst->result_id());
          }
          // Break uint64 into 2x uint32
          Instruction* lo_ui64_inst = builder->AddUnaryOp(
              GetUintId(), spv::Op::OpUConvert, ui64_inst->result_id());
          Instruction* rshift_ui64_inst = builder->AddBinaryOp(
              GetUint64Id(), spv::Op::OpShiftRightLogical,
              ui64_inst->result_id(), builder->GetUintConstantId(32));
          Instruction* hi_ui64_inst = builder->AddUnaryOp(
              GetUintId(), spv::Op::OpUConvert, rshift_ui64_inst->result_id());
          val_ids->push_back(lo_ui64_inst->result_id());
          val_ids->push_back(hi_ui64_inst->result_id());
          return;
        }
        case 8: {
          Instruction* ui8_inst = val_inst;
          if (i_ty->IsSigned()) {
            // Bitcast sint8 to uint8
            ui8_inst = builder->AddUnaryOp(GetUint8Id(), spv::Op::OpBitcast,
                                           val_inst->result_id());
          }
          // Convert uint8 to uint32
          Instruction* ui32_inst = builder->AddUnaryOp(
              GetUintId(), spv::Op::OpUConvert, ui8_inst->result_id());
          val_ids->push_back(ui32_inst->result_id());
          return;
        }
        case 32: {
          Instruction* ui32_inst = val_inst;
          if (i_ty->IsSigned()) {
            // Bitcast sint32 to uint32
            ui32_inst = builder->AddUnaryOp(GetUintId(), spv::Op::OpBitcast,
                                            val_inst->result_id());
          }
          // uint32 needs no further processing
          val_ids->push_back(ui32_inst->result_id());
          return;
        }
        default:
          // TODO(greg-lunarg): Support non-32-bit int
          assert(false && "unsupported int width");
          return;
      }
    }
    default:
      assert(false && "unsupported type");
      return;
  }
}

void InstDebugPrintfPass::GenOutputCode(
    Instruction* printf_inst,
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks) {
  BasicBlock* back_blk_ptr = &*new_blocks->back();
  InstructionBuilder builder(
      context(), back_blk_ptr,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  // Gen debug printf record validation-specific values. The format string
  // will have its id written. Vectors will need to be broken down into
  // component values. float16 will need to be converted to float32. Pointer
  // and uint64 will need to be converted to two uint32 values. float32 will
  // need to be bitcast to uint32. int32 will need to be bitcast to uint32.
  std::vector<uint32_t> val_ids;
  bool is_first_operand = false;
  printf_inst->ForEachInId(
      [&is_first_operand, &val_ids, &builder, this](const uint32_t* iid) {
        // skip set operand
        if (!is_first_operand) {
          is_first_operand = true;
          return;
        }
        Instruction* opnd_inst = get_def_use_mgr()->GetDef(*iid);
        if (opnd_inst->opcode() == spv::Op::OpString) {
          uint32_t string_id_id = builder.GetUintConstantId(*iid);
          val_ids.push_back(string_id_id);
        } else {
          GenOutputValues(opnd_inst, &val_ids, &builder);
        }
      });
  GenDebugStreamWrite(
      builder.GetUintConstantId(shader_id_),
      builder.GetUintConstantId(uid2offset_[printf_inst->unique_id()]), val_ids,
      &builder);
  context()->KillInst(printf_inst);
}

void InstDebugPrintfPass::GenDebugPrintfCode(
    BasicBlock::iterator ref_inst_itr,
    UptrVectorIterator<BasicBlock> ref_block_itr,
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks) {
  // If not DebugPrintf OpExtInst, return.
  Instruction* printf_inst = &*ref_inst_itr;
  if (printf_inst->opcode() != spv::Op::OpExtInst) return;
  if (printf_inst->GetSingleWordInOperand(0) != ext_inst_printf_id_) return;
  if (printf_inst->GetSingleWordInOperand(1) !=
      NonSemanticDebugPrintfDebugPrintf)
    return;
  // Initialize DefUse manager before dismantling module
  (void)get_def_use_mgr();
  // Move original block's preceding instructions into first new block
  std::unique_ptr<BasicBlock> new_blk_ptr;
  MovePreludeCode(ref_inst_itr, ref_block_itr, &new_blk_ptr);
  new_blocks->push_back(std::move(new_blk_ptr));
  // Generate instructions to output printf args to printf buffer
  GenOutputCode(printf_inst, new_blocks);
  // Caller expects at least two blocks with last block containing remaining
  // code, so end block after instrumentation, create remainder block, and
  // branch to it
  uint32_t rem_blk_id = TakeNextId();
  std::unique_ptr<Instruction> rem_label(NewLabel(rem_blk_id));
  BasicBlock* back_blk_ptr = &*new_blocks->back();
  InstructionBuilder builder(
      context(), back_blk_ptr,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  (void)builder.AddBranch(rem_blk_id);
  // Gen remainder block
  new_blk_ptr.reset(new BasicBlock(std::move(rem_label)));
  builder.SetInsertPoint(&*new_blk_ptr);
  // Move original block's remaining code into remainder block and add
  // to new blocks
  MovePostludeCode(ref_block_itr, &*new_blk_ptr);
  new_blocks->push_back(std::move(new_blk_ptr));
}

// Return id for output buffer
uint32_t InstDebugPrintfPass::GetOutputBufferId() {
  if (output_buffer_id_ == 0) {
    // If not created yet, create one
    analysis::DecorationManager* deco_mgr = get_decoration_mgr();
    analysis::TypeManager* type_mgr = context()->get_type_mgr();
    analysis::RuntimeArray* reg_uint_rarr_ty = GetUintRuntimeArrayType(32);
    analysis::Integer* reg_uint_ty = GetInteger(32, false);
    analysis::Type* reg_buf_ty =
        GetStruct({reg_uint_ty, reg_uint_ty, reg_uint_rarr_ty});
    uint32_t obufTyId = type_mgr->GetTypeInstruction(reg_buf_ty);
    // By the Vulkan spec, a pre-existing struct containing a RuntimeArray
    // must be a block, and will therefore be decorated with Block. Therefore
    // the undecorated type returned here will not be pre-existing and can
    // safely be decorated. Since this type is now decorated, it is out of
    // sync with the TypeManager and therefore the TypeManager must be
    // invalidated after this pass.
    assert(context()->get_def_use_mgr()->NumUses(obufTyId) == 0 &&
           "used struct type returned");
    deco_mgr->AddDecoration(obufTyId, uint32_t(spv::Decoration::Block));
    deco_mgr->AddMemberDecoration(obufTyId, kDebugOutputFlagsOffset,
                                  uint32_t(spv::Decoration::Offset), 0);
    deco_mgr->AddMemberDecoration(obufTyId, kDebugOutputSizeOffset,
                                  uint32_t(spv::Decoration::Offset), 4);
    deco_mgr->AddMemberDecoration(obufTyId, kDebugOutputDataOffset,
                                  uint32_t(spv::Decoration::Offset), 8);
    uint32_t obufTyPtrId_ =
        type_mgr->FindPointerToType(obufTyId, spv::StorageClass::StorageBuffer);
    output_buffer_id_ = TakeNextId();
    std::unique_ptr<Instruction> newVarOp(new Instruction(
        context(), spv::Op::OpVariable, obufTyPtrId_, output_buffer_id_,
        {{spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER,
          {uint32_t(spv::StorageClass::StorageBuffer)}}}));
    context()->AddGlobalValue(std::move(newVarOp));
    context()->AddDebug2Inst(NewGlobalName(obufTyId, "OutputBuffer"));
    context()->AddDebug2Inst(NewMemberName(obufTyId, 0, "flags"));
    context()->AddDebug2Inst(NewMemberName(obufTyId, 1, "written_count"));
    context()->AddDebug2Inst(NewMemberName(obufTyId, 2, "data"));
    context()->AddDebug2Inst(NewGlobalName(output_buffer_id_, "output_buffer"));
    deco_mgr->AddDecorationVal(
        output_buffer_id_, uint32_t(spv::Decoration::DescriptorSet), desc_set_);
    deco_mgr->AddDecorationVal(output_buffer_id_,
                               uint32_t(spv::Decoration::Binding),
                               GetOutputBufferBinding());
    AddStorageBufferExt();
    if (get_module()->version() >= SPV_SPIRV_VERSION_WORD(1, 4)) {
      // Add the new buffer to all entry points.
      for (auto& entry : get_module()->entry_points()) {
        entry.AddOperand({SPV_OPERAND_TYPE_ID, {output_buffer_id_}});
        context()->AnalyzeUses(&entry);
      }
    }
  }
  return output_buffer_id_;
}

uint32_t InstDebugPrintfPass::GetOutputBufferPtrId() {
  if (output_buffer_ptr_id_ == 0) {
    output_buffer_ptr_id_ = context()->get_type_mgr()->FindPointerToType(
        GetUintId(), spv::StorageClass::StorageBuffer);
  }
  return output_buffer_ptr_id_;
}

uint32_t InstDebugPrintfPass::GetOutputBufferBinding() {
  return kDebugOutputPrintfStream;
}

void InstDebugPrintfPass::GenDebugOutputFieldCode(uint32_t base_offset_id,
                                                  uint32_t field_offset,
                                                  uint32_t field_value_id,
                                                  InstructionBuilder* builder) {
  // Cast value to 32-bit unsigned if necessary
  uint32_t val_id = GenUintCastCode(field_value_id, builder);
  // Store value
  Instruction* data_idx_inst = builder->AddIAdd(
      GetUintId(), base_offset_id, builder->GetUintConstantId(field_offset));
  uint32_t buf_id = GetOutputBufferId();
  uint32_t buf_uint_ptr_id = GetOutputBufferPtrId();
  Instruction* achain_inst = builder->AddAccessChain(
      buf_uint_ptr_id, buf_id,
      {builder->GetUintConstantId(kDebugOutputDataOffset),
       data_idx_inst->result_id()});
  (void)builder->AddStore(achain_inst->result_id(), val_id);
}

uint32_t InstDebugPrintfPass::GetStreamWriteFunctionId(uint32_t param_cnt) {
  enum {
    kShaderId = 0,
    kInstructionIndex = 1,
    kFirstParam = 2,
  };
  // Total param count is common params plus validation-specific
  // params
  if (param2output_func_id_[param_cnt] == 0) {
    // Create function
    param2output_func_id_[param_cnt] = TakeNextId();
    analysis::TypeManager* type_mgr = context()->get_type_mgr();

    const analysis::Type* uint_type = GetInteger(32, false);

    std::vector<const analysis::Type*> param_types(kFirstParam + param_cnt,
                                                   uint_type);
    std::unique_ptr<Function> output_func = StartFunction(
        param2output_func_id_[param_cnt], type_mgr->GetVoidType(), param_types);

    std::vector<uint32_t> param_ids = AddParameters(*output_func, param_types);

    // Create first block
    auto new_blk_ptr = MakeUnique<BasicBlock>(NewLabel(TakeNextId()));

    InstructionBuilder builder(
        context(), &*new_blk_ptr,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
    // Gen test if debug output buffer size will not be exceeded.
    const uint32_t first_param_offset = kInstCommonOutInstructionIdx + 1;
    const uint32_t obuf_record_sz = first_param_offset + param_cnt;
    const uint32_t buf_id = GetOutputBufferId();
    const uint32_t buf_uint_ptr_id = GetOutputBufferPtrId();
    Instruction* obuf_curr_sz_ac_inst = builder.AddAccessChain(
        buf_uint_ptr_id, buf_id,
        {builder.GetUintConstantId(kDebugOutputSizeOffset)});
    // Fetch the current debug buffer written size atomically, adding the
    // size of the record to be written.
    uint32_t obuf_record_sz_id = builder.GetUintConstantId(obuf_record_sz);
    uint32_t mask_none_id =
        builder.GetUintConstantId(uint32_t(spv::MemoryAccessMask::MaskNone));
    uint32_t scope_invok_id =
        builder.GetUintConstantId(uint32_t(spv::Scope::Invocation));
    Instruction* obuf_curr_sz_inst = builder.AddQuadOp(
        GetUintId(), spv::Op::OpAtomicIAdd, obuf_curr_sz_ac_inst->result_id(),
        scope_invok_id, mask_none_id, obuf_record_sz_id);
    uint32_t obuf_curr_sz_id = obuf_curr_sz_inst->result_id();
    // Compute new written size
    Instruction* obuf_new_sz_inst =
        builder.AddIAdd(GetUintId(), obuf_curr_sz_id,
                        builder.GetUintConstantId(obuf_record_sz));
    // Fetch the data bound
    Instruction* obuf_bnd_inst =
        builder.AddIdLiteralOp(GetUintId(), spv::Op::OpArrayLength,
                               GetOutputBufferId(), kDebugOutputDataOffset);
    // Test that new written size is less than or equal to debug output
    // data bound
    Instruction* obuf_safe_inst = builder.AddBinaryOp(
        GetBoolId(), spv::Op::OpULessThanEqual, obuf_new_sz_inst->result_id(),
        obuf_bnd_inst->result_id());
    uint32_t merge_blk_id = TakeNextId();
    uint32_t write_blk_id = TakeNextId();
    std::unique_ptr<Instruction> merge_label(NewLabel(merge_blk_id));
    std::unique_ptr<Instruction> write_label(NewLabel(write_blk_id));
    (void)builder.AddConditionalBranch(
        obuf_safe_inst->result_id(), write_blk_id, merge_blk_id, merge_blk_id,
        uint32_t(spv::SelectionControlMask::MaskNone));
    // Close safety test block and gen write block
    output_func->AddBasicBlock(std::move(new_blk_ptr));
    new_blk_ptr = MakeUnique<BasicBlock>(std::move(write_label));
    builder.SetInsertPoint(&*new_blk_ptr);
    // Generate common and stage-specific debug record members
    GenDebugOutputFieldCode(obuf_curr_sz_id, kInstCommonOutSize,
                            builder.GetUintConstantId(obuf_record_sz),
                            &builder);
    // Store Shader Id
    GenDebugOutputFieldCode(obuf_curr_sz_id, kInstCommonOutShaderId,
                            param_ids[kShaderId], &builder);
    // Store Instruction Idx
    GenDebugOutputFieldCode(obuf_curr_sz_id, kInstCommonOutInstructionIdx,
                            param_ids[kInstructionIndex], &builder);
    // Gen writes of validation specific data
    for (uint32_t i = 0; i < param_cnt; ++i) {
      GenDebugOutputFieldCode(obuf_curr_sz_id, first_param_offset + i,
                              param_ids[kFirstParam + i], &builder);
    }
    // Close write block and gen merge block
    (void)builder.AddBranch(merge_blk_id);
    output_func->AddBasicBlock(std::move(new_blk_ptr));
    new_blk_ptr = MakeUnique<BasicBlock>(std::move(merge_label));
    builder.SetInsertPoint(&*new_blk_ptr);
    // Close merge block and function and add function to module
    (void)builder.AddNullaryOp(0, spv::Op::OpReturn);

    output_func->AddBasicBlock(std::move(new_blk_ptr));
    output_func->SetFunctionEnd(EndFunction());
    context()->AddFunction(std::move(output_func));

    std::string name("stream_write_");
    name += spvtools::to_string(param_cnt);

    context()->AddDebug2Inst(
        NewGlobalName(param2output_func_id_[param_cnt], name));
  }
  return param2output_func_id_[param_cnt];
}

void InstDebugPrintfPass::GenDebugStreamWrite(
    uint32_t shader_id, uint32_t instruction_idx_id,
    const std::vector<uint32_t>& validation_ids, InstructionBuilder* builder) {
  // Call debug output function. Pass func_idx, instruction_idx and
  // validation ids as args.
  uint32_t val_id_cnt = static_cast<uint32_t>(validation_ids.size());
  std::vector<uint32_t> args = {shader_id, instruction_idx_id};
  (void)args.insert(args.end(), validation_ids.begin(), validation_ids.end());
  (void)builder->AddFunctionCall(GetVoidId(),
                                 GetStreamWriteFunctionId(val_id_cnt), args);
}

std::unique_ptr<Instruction> InstDebugPrintfPass::NewGlobalName(
    uint32_t id, const std::string& name_str) {
  std::string prefixed_name{"inst_printf_"};
  prefixed_name += name_str;
  return NewName(id, prefixed_name);
}

std::unique_ptr<Instruction> InstDebugPrintfPass::NewMemberName(
    uint32_t id, uint32_t member_index, const std::string& name_str) {
  return MakeUnique<Instruction>(
      context(), spv::Op::OpMemberName, 0, 0,
      std::initializer_list<Operand>{
          {SPV_OPERAND_TYPE_ID, {id}},
          {SPV_OPERAND_TYPE_LITERAL_INTEGER, {member_index}},
          {SPV_OPERAND_TYPE_LITERAL_STRING, utils::MakeVector(name_str)}});
}

void InstDebugPrintfPass::InitializeInstDebugPrintf() {
  // Initialize base class
  InitializeInstrument();
  output_buffer_id_ = 0;
  output_buffer_ptr_id_ = 0;
}

Pass::Status InstDebugPrintfPass::ProcessImpl() {
  // Perform printf instrumentation on each entry point function in module
  InstProcessFunction pfn =
      [this](BasicBlock::iterator ref_inst_itr,
             UptrVectorIterator<BasicBlock> ref_block_itr,
             [[maybe_unused]] uint32_t stage_idx,
             std::vector<std::unique_ptr<BasicBlock>>* new_blocks) {
        return GenDebugPrintfCode(ref_inst_itr, ref_block_itr, new_blocks);
      };
  (void)InstProcessEntryPointCallTree(pfn);
  // Remove DebugPrintf OpExtInstImport instruction
  Instruction* ext_inst_import_inst =
      get_def_use_mgr()->GetDef(ext_inst_printf_id_);
  context()->KillInst(ext_inst_import_inst);
  // If no remaining non-semantic instruction sets, remove non-semantic debug
  // info extension from module and feature manager
  bool non_sem_set_seen = false;
  for (auto c_itr = context()->module()->ext_inst_import_begin();
       c_itr != context()->module()->ext_inst_import_end(); ++c_itr) {
    const std::string set_name = c_itr->GetInOperand(0).AsString();
    if (spvtools::utils::starts_with(set_name, "NonSemantic.")) {
      non_sem_set_seen = true;
      break;
    }
  }
  if (!non_sem_set_seen) {
    context()->RemoveExtension(kSPV_KHR_non_semantic_info);
  }
  return Status::SuccessWithChange;
}

Pass::Status InstDebugPrintfPass::Process() {
  ext_inst_printf_id_ =
      get_module()->GetExtInstImportId("NonSemantic.DebugPrintf");
  if (ext_inst_printf_id_ == 0) return Status::SuccessWithoutChange;
  InitializeInstDebugPrintf();
  return ProcessImpl();
}

}  // namespace opt
}  // namespace spvtools
