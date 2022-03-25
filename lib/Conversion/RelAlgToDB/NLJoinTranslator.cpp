#include <mlir/Conversion/RelAlgToDB/NLJoinTranslator.h>
using namespace mlir::relalg;
void NLJoinTranslator::setInfo(mlir::relalg::Translator* consumer, mlir::relalg::ColumnSet requiredAttributes) {
   this->consumer = consumer;
   this->requiredAttributes = requiredAttributes;
   addJoinRequiredColumns();
   impl->addAdditionalRequiredColumns();
   propagateInfo();
}

void NLJoinTranslator::build(mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) {
   auto const0 = builder.create<mlir::arith::ConstantOp>(loc, builder.getIntegerType(64), builder.getI64IntegerAttr(0));
   mlir::Value packed = orderedAttributesLeft.pack(context, builder, op->getLoc(), impl->markable ? std::vector<Value>{const0} : std::vector<Value>());
   builder.create<mlir::db::Append>(loc, vector, packed);
}
void NLJoinTranslator::scanHT(mlir::relalg::TranslatorContext& context, mlir::OpBuilder& builder) {
   auto scope = context.createScope();
   {
      auto forOp2 = builder.create<mlir::db::ForOp>(loc, getRequiredBuilderTypes(context), vector, context.pipelineManager.getCurrentPipeline()->getFlag(), getRequiredBuilderValues(context));
      mlir::Block* block2 = new mlir::Block;
      block2->addArgument(tupleType, loc);
      block2->addArguments(getRequiredBuilderTypes(context), getRequiredBuilderLocs(context));
      forOp2.getBodyRegion().push_back(block2);
      mlir::OpBuilder builder2(forOp2.getBodyRegion());
      setRequiredBuilderValues(context, block2->getArguments().drop_front(1));
      auto unpacked = builder2.create<mlir::util::UnPackOp>(loc, forOp2.getInductionVar());
      orderedAttributesLeft.setValuesForColumns(context, scope, unpacked.getResults());
      Value marker = impl->markable ? unpacked.getResult(unpacked.getNumResults() - 1) : Value();
      impl->handleScanned(marker, context, builder2);
      builder2.create<mlir::db::YieldOp>(loc, getRequiredBuilderValues(context));
      setRequiredBuilderValues(context, forOp2.results());
   }
}

void NLJoinTranslator::probe(mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) {
   auto scope = context.createScope();
   impl->beforeLookup(context, builder);
   {
      auto forOp2 = builder.create<mlir::db::ForOp>(loc, getRequiredBuilderTypes(context), vector, impl->getFlag(), getRequiredBuilderValues(context));
      mlir::Block* block2 = new mlir::Block;
      block2->addArgument(tupleType, loc);
      block2->addArguments(getRequiredBuilderTypes(context), getRequiredBuilderLocs(context));
      forOp2.getBodyRegion().push_back(block2);
      mlir::OpBuilder builder2(forOp2.getBodyRegion());
      setRequiredBuilderValues(context, block2->getArguments().drop_front(1));
      auto unpacked = builder2.create<mlir::util::UnPackOp>(loc, forOp2.getInductionVar());
      orderedAttributesLeft.setValuesForColumns(context, scope, unpacked.getResults());
      Value markerLeft = impl->markable ? unpacked.getResult(unpacked.getNumResults() - 1) : Value();
      Value matched = evaluatePredicate(context, builder2, scope);
      impl->handleLookup(matched, markerLeft, context, builder2);
      builder2.create<mlir::db::YieldOp>(loc, getRequiredBuilderValues(context));
      setRequiredBuilderValues(context, forOp2.results());
   }
   impl->afterLookup(context, builder);
}
void NLJoinTranslator::consume(mlir::relalg::Translator* child, mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) {
   if (child == this->children[0].get()) {
      build(builder, context);
   } else if (child == this->children[1].get()) {
      probe(builder, context);
   }
}
void NLJoinTranslator::produce(mlir::relalg::TranslatorContext& context, mlir::OpBuilder& builder) {
   auto leftAttributes = this->requiredAttributes.intersect(children[0]->getAvailableColumns());
   orderedAttributesLeft = mlir::relalg::OrderedAttributes::fromColumns(leftAttributes);
   tupleType = orderedAttributesLeft.getTupleType(op.getContext(), impl->markable ? std::vector<Type>({mlir::IntegerType::get(op->getContext(), 64)}) : std::vector<Type>());
   vector = builder.create<mlir::db::CreateDS>(loc, mlir::db::VectorType::get(builder.getContext(), tupleType));
   children[0]->produce(context, builder);
   children[1]->produce(context, builder);
   impl->after(context, builder);
   builder.create<mlir::db::FreeOp>(loc, vector);
}
