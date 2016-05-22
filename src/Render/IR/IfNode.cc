#include "IRRenderer.h"

#include "kscope/AST/IfNode.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"


llvm::Value *
IRRenderer::visit_node(IfNode *node) {
  auto &context = get_render_context();
  auto &llvm_context = context.get_llvm_context();
  auto &builder = context.get_builder();

  auto zero = llvm::ConstantFP::get(llvm_context, llvm::APFloat(0.0));

  auto *cond_value = visit(node->condition);
  if ( cond_value == 0 ) { return nullptr; }

  cond_value = builder.CreateFCmpONE(cond_value, zero, "ifcond");

  auto *func = builder.GetInsertBlock()->getParent();

  auto *then_block = llvm::BasicBlock::Create(llvm_context, "then", func);
  auto *else_block = llvm::BasicBlock::Create(llvm_context, "else");
  auto *merge_block = llvm::BasicBlock::Create(llvm_context, "ifcont");

  builder.CreateCondBr(cond_value, then_block, else_block);
  builder.SetInsertPoint(then_block);

  auto *then_value = visit(node->then);
  if ( then_value == 0 ) { return nullptr; }


  builder.CreateBr(merge_block);
  then_block = builder.GetInsertBlock();

  func->getBasicBlockList().push_back(else_block);
  builder.SetInsertPoint(else_block);

  auto *else_value = visit(node->_else);
  if ( else_value == 0 ) { return nullptr; }

  builder.CreateBr(merge_block);
  else_block = builder.GetInsertBlock();

  func->getBasicBlockList().push_back(merge_block);
  builder.SetInsertPoint(merge_block);
  auto *phi_node = builder.CreatePHI(llvm::Type::getDoubleTy(llvm_context),
                                     2,
                                     "iftmp");

  phi_node->addIncoming(then_value, then_block);
  phi_node->addIncoming(else_value, else_block);

  return phi_node;
}