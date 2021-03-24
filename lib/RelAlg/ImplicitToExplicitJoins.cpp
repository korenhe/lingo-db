
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"

#include "mlir/Dialect/RelAlg/Passes.h"
#include "mlir/IR/BlockAndValueMapping.h"

#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include <iostream>
#include <list>
#include <queue>
#include <unordered_set>
#include <mlir/Dialect/RelAlg/IR/RelAlgDialect.h>

namespace {

class ImplicitToExplicitJoins : public mlir::PassWrapper<ImplicitToExplicitJoins, mlir::FunctionPass> {
   void addRequirements(mlir::Operation* op,mlir::Block* b,llvm::SmallVector<mlir::Operation*,8>& extracted, llvm::SmallPtrSet<mlir::Operation*,8>& alreadyPresent){
      if(!op)
         return;
      if(op->getBlock()!=b)
         return;
      if(alreadyPresent.contains(op))
         return;
      for(auto operand:op->getOperands()) {
         addRequirements(operand.getDefiningOp(), b, extracted, alreadyPresent);
      }
      alreadyPresent.insert(op);
      extracted.push_back(op);
   }
   mlir::Value extract(mlir::Value v,TupleLamdaOperator parent,TupleLamdaOperator newParent ) {
      using namespace mlir;
      auto attributeManager = getContext().getLoadedDialect<mlir::relalg::RelAlgDialect>()->getRelationalAttributeManager();
      std::string scope_name=attributeManager.getUniqueScope("extracted_map");
      attributeManager.setCurrentScope(scope_name);
      llvm::SmallVector<mlir::Operation*, 8> extracted;
      llvm::SmallPtrSet<mlir::Operation*, 8> alreadyPresent;
      addRequirements(v.getDefiningOp(), &parent.getLambdaBlock(), extracted, alreadyPresent);
      OpBuilder builder(parent.getOperation());
      mlir::BlockAndValueMapping mapping;

      mapping.map(parent.getLambdaArgument(), newParent.getLambdaArgument());
      builder.setInsertionPointToStart(&newParent.getLambdaBlock());
      auto returnop = builder.create<relalg::ReturnOp>(builder.getUnknownLoc());
      builder.setInsertionPointToStart(&newParent.getLambdaBlock());
      for (auto op : extracted) {
         auto clone_op = builder.clone(*op, mapping);
         clone_op->moveBefore(returnop);
      }
      builder.setInsertionPoint(returnop);
      return mapping.lookup(v);
   }


      void runOnFunction() override {
      auto attributeManager = getContext().getLoadedDialect<mlir::relalg::RelAlgDialect>()->getRelationalAttributeManager();
      using namespace mlir;
      getFunction().walk([&](mlir::Operation* op) {
         TupleLamdaOperator surrounding_operator = op->getParentOfType<TupleLamdaOperator>();
         if (!surrounding_operator) {
            return;
         }

         Value tree_val = surrounding_operator->getOperand(0);
         if (auto existsop = mlir::dyn_cast_or_null<mlir::relalg::ExistsOp>(op)) {
            OpBuilder builder(surrounding_operator);
            std::string scope_name = attributeManager.getUniqueScope("markjoin");
            std::string attribute_name = "markattr";
            attributeManager.setCurrentScope(scope_name);
            relalg::RelationalAttributeDefAttr defAttr = attributeManager.createDef(attribute_name);
            relalg::RelationalAttributeRefAttr refAttr = attributeManager.createRef(scope_name, attribute_name);
            auto& ra = defAttr.getRelationalAttribute();
            ra.type = mlir::db::BoolType::get(&getContext());

            //::mlir::Type result, ::mlir::StringAttr sym_name, ::mlir::relalg::RelationalAttributeDefAttr markattr, ::mlir::Value left, ::mlir::Value right
            auto mjop = builder.create<relalg::MarkJoinOp>(builder.getUnknownLoc(), mlir::relalg::RelationType::get(builder.getContext()), mlir::relalg::JoinDirection::left, scope_name, defAttr, tree_val, existsop.rel());
            mjop.getRegion().push_back(new Block);
            builder.setInsertionPointToStart(&mjop.getRegion().front());
            builder.create<relalg::ReturnOp>(builder.getUnknownLoc());
            builder.setInsertionPoint(existsop);
            auto replacement = builder.create<relalg::GetAttrOp>(builder.getUnknownLoc(), db::BoolType::get(builder.getContext()), refAttr, surrounding_operator.getLambdaRegion().getArgument(0));
            existsop->replaceAllUsesWith(replacement);
            existsop->remove();
            existsop->destroy();
            tree_val = mjop;
         } else if (auto getscalarop = mlir::dyn_cast_or_null<mlir::relalg::GetScalarOp>(op)) {
            OpBuilder builder(surrounding_operator);
            auto mjop = builder.create<relalg::SingleJoinOp>(builder.getUnknownLoc(), mlir::relalg::RelationType::get(builder.getContext()), mlir::relalg::JoinDirection::left, tree_val, getscalarop.rel());
            mjop.getRegion().push_back(new Block);
            builder.setInsertionPointToStart(&mjop.getRegion().front());
            builder.create<relalg::ReturnOp>(builder.getUnknownLoc());
            builder.setInsertionPoint(getscalarop);
            Operation* replacement = builder.create<relalg::GetAttrOp>(builder.getUnknownLoc(), db::BoolType::get(builder.getContext()), getscalarop.attr(), surrounding_operator.getLambdaRegion().getArgument(0));
            getscalarop.replaceAllUsesWith(replacement);
            getscalarop->remove();
            getscalarop->destroy();
            tree_val = mjop;
         } else if(auto inop= mlir::dyn_cast_or_null<mlir::relalg::InOp>(op)){
            //get attribute of relation to search in
            Operator relOperator=inop.rel().getDefiningOp();
            auto available_attrs=relOperator.getAvailableAttributes();
            assert(available_attrs.size()==1);
            auto attr=*available_attrs.begin();
            auto searchInAttr =attributeManager.createRef(attr);
            //get attribute f relation to search in
            OpBuilder builder(surrounding_operator);
            std::string scope_name = attributeManager.getUniqueScope("markjoin");
            std::string attribute_name = "markattr";
            attributeManager.setCurrentScope(scope_name);
            relalg::RelationalAttributeDefAttr markAttrDef = attributeManager.createDef(attribute_name);
            relalg::RelationalAttributeRefAttr markAttrRef = attributeManager.createRef(scope_name, attribute_name);
            auto& ra = markAttrDef.getRelationalAttribute();
            ra.type = mlir::db::BoolType::get(&getContext());
            TupleLamdaOperator mjop = builder.create<relalg::MarkJoinOp>(builder.getUnknownLoc(), mlir::relalg::RelationType::get(builder.getContext()), mlir::relalg::JoinDirection::left, scope_name, markAttrDef, tree_val, inop.rel());
            mjop.getLambdaRegion().push_back(new Block);
            mjop.getLambdaBlock().addArgument(mlir::relalg::TupleType::get(&getContext()));
            Value val=extract(inop.val(),surrounding_operator,mjop);
            builder.setInsertionPoint(mjop.getLambdaBlock().getTerminator());
            auto other_val= builder.create<relalg::GetAttrOp>(builder.getUnknownLoc(), searchInAttr.getRelationalAttribute().type, searchInAttr, mjop.getLambdaArgument());
            bool nullable=val.getType().dyn_cast_or_null<mlir::db::DBType>().isNullable()||other_val.getType().dyn_cast_or_null<mlir::db::DBType>().isNullable();
            Value predicate=builder.create<mlir::db::CmpOp>(builder.getUnknownLoc(), mlir::db::BoolType::get(&getContext(),nullable),mlir::db::DBCmpPredicate::eq,val,other_val);
            auto previous_return=mjop.getLambdaBlock().getTerminator();
            builder.create<mlir::relalg::ReturnOp>(builder.getUnknownLoc(),predicate);
            previous_return->remove();
            previous_return->destroy();
            builder.setInsertionPoint(inop);
            auto replacement = builder.create<relalg::GetAttrOp>(builder.getUnknownLoc(), db::BoolType::get(builder.getContext()), markAttrRef, surrounding_operator.getLambdaRegion().getArgument(0));
            inop->replaceAllUsesWith(replacement);
            inop->remove();
            inop->destroy();
            tree_val = mjop->getResult(0);
         }
         surrounding_operator->setOperand(0, tree_val);
      });
   }
};
} // end anonymous namespace

namespace mlir {
namespace relalg {
std::unique_ptr<Pass> createImplicitToExplicitJoinsPass() { return std::make_unique<ImplicitToExplicitJoins>(); }
} // end namespace relalg
} // end namespace mlir