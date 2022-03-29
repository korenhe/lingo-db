#include "mlir/Conversion/DSAToStd/FunctionRegistry.h"
#include "mlir/Dialect/util/UtilOps.h"

void mlir::dsa::codegen::FunctionRegistry::registerFunctions(){
#define INT_TYPE(W) IntegerType::get(context, W)
#define FLOAT_TYPE FloatType::getF32(context)
#define DOUBLE_TYPE FloatType::getF64(context)

#define BOOL_TYPE INT_TYPE(1)

#define INDEX_TYPE IndexType::get(context)

#define POINTER_TYPE mlir::util::RefType::get(context, IntegerType::get(context, 8))
#define STRING_TYPE mlir::util::VarLen32Type::get(context)
#define TUPLE_TYPE(...) TupleType::get(context, TypeRange({__VA_ARGS__}))
#define OPERANDS_(...) {__VA_ARGS__}
#define RETURNS_(...) \
   { __VA_ARGS__ }
#define FUNCTION_TYPE(operands, returns) FunctionType::get(context, operands, returns)
#define REGISTER_FUNC(inst, name, operands, returns) registerFunction(FunctionId::inst, #name, operands, returns);
   FUNC_LIST(REGISTER_FUNC, OPERANDS_, RETURNS_)
#undef REGISTER_FUNC
#undef RETURNS_
#undef OPERANDS_
} mlir::FuncOp mlir::dsa::codegen::FunctionRegistry::insertFunction(mlir::OpBuilder builder, mlir::dsa::codegen::FunctionRegistry::RegisteredFunction& function) {
   OpBuilder::InsertionGuard insertionGuard(builder);
   builder.setInsertionPointToStart(parentModule.getBody());
   FuncOp funcOp = builder.create<FuncOp>(parentModule.getLoc(), function.useWrapper ? "rt_" + function.name : function.name, builder.getFunctionType(function.operands, function.results), builder.getStringAttr("private"));
   if (function.name.starts_with("cmp_string")) {
      funcOp->setAttr("const", builder.getUnitAttr());
   }
   return funcOp;
}

mlir::FuncOp mlir::dsa::codegen::FunctionRegistry::getFunction(OpBuilder builder, FunctionId function) {
   size_t offset = static_cast<size_t>(function);
   if (insertedFunctions.size() > offset && insertedFunctions[offset]) {
      return insertedFunctions[offset];
   }
   if (registeredFunctions.size() > offset && !registeredFunctions[offset].name.empty()) {
      FuncOp inserted = insertFunction(builder, registeredFunctions[offset]);
      insertedFunctions[offset] = inserted;
      return inserted;
   }
   assert(false && "could not find function");
}
mlir::ResultRange mlir::dsa::codegen::FunctionRegistry::call(OpBuilder builder, Location loc, FunctionId function, ValueRange values) {
   FuncOp func = getFunction(builder, function);
   auto funcCall = builder.create<CallOp>(loc, func, values);
   return funcCall.getResults();
}
void mlir::dsa::codegen::FunctionRegistry::registerFunction(FunctionId funcId, std::string name, std::vector<mlir::Type> ops, std::vector<mlir::Type> returns, bool useWrapper) {
   registeredFunctions.push_back({name, useWrapper, ops, returns});
   insertedFunctions.push_back(FuncOp());
}
mlir::dsa::codegen::FunctionRegistry::RegisteredFunction& mlir::dsa::codegen::FunctionRegistry::getRegisteredFunction(FunctionId functionId) {
   return registeredFunctions[static_cast<size_t>(functionId)];
}