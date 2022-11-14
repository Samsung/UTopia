#include "ftg/indcallsolver/GlobalInitializerSolverHandler.h"
#include <llvm/IR/Function.h>

using namespace ftg;
using namespace llvm;

GlobalInitializerSolverHandler::GlobalInitializerSolverHandler(
    GlobalInitializerSolverHandler &&Handler) {
  this->Map = std::move(Handler.Map);
}

std::set<const Function *>
GlobalInitializerSolverHandler::get(const Type *Ty, unsigned Idx) const {
  auto Iter = Map.find(MapKey(Ty, Idx));
  if (Iter == Map.end())
    return {};

  return Iter->second;
}

void GlobalInitializerSolverHandler::handle(const GlobalVariable &GV) {
  if (!GV.hasInitializer())
    return;

  const auto *Initializer = GV.getInitializer();
  assert(Initializer && "Unexpected LLVM API Behavior");

  const auto *Ty = dyn_cast_or_null<StructType>(Initializer->getType());
  const auto *GVTy = GV.getType();
  if (!Ty) {
    if (const auto *F = dyn_cast<Function>(Initializer)) {
      Map[MapKey(GVTy, (unsigned)0)].insert(F);
    }
    return;
  }

  for (unsigned S = 0, E = Ty->getNumElements(); S < E; ++S) {
    const auto *F =
        dyn_cast_or_null<Function>(Initializer->getAggregateElement(S));
    if (!F)
      return;

    Map[MapKey(GVTy, S)].insert(F);
  }
}

GlobalInitializerSolverHandler::MapKey::MapKey(const Type *Ty, unsigned Idx)
    : Ty(Ty), Idx(Idx) {}

bool GlobalInitializerSolverHandler::MapKey::operator<(
    const MapKey &Key) const {
  if (Ty != Key.Ty)
    return Ty < Key.Ty;
  return Idx < Key.Idx;
}
