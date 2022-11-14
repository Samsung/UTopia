#include "ftg/indcallsolver/TBAASimpleSolverHandler.h"
#include <llvm/IR/Instructions.h>

using namespace ftg;
using namespace llvm;

TBAASimpleSolverHandler::TBAASimpleSolverHandler(
    TBAASimpleSolverHandler &&Handler)
    : Map(std::move(Handler.Map)) {}

std::set<const llvm::Function *>
TBAASimpleSolverHandler::get(const llvm::MDNode *Node,
                             const llvm::Type *Ty) const {
  auto Iter = Map.find(MapKey(Node, Ty));
  if (Iter == Map.end())
    return {};

  return Iter->second;
}

void TBAASimpleSolverHandler::handle(const llvm::Instruction &I) {
  if (!isa<StoreInst>(&I))
    return;

  const auto *Node = I.getMetadata(LLVMContext::MD_tbaa);
  if (!Node)
    return;

  auto *F = dyn_cast<Function>(I.getOperand(0));
  if (!F)
    return;

  Map[MapKey(Node, F->getType())].emplace(F);
}

TBAASimpleSolverHandler::MapKey::MapKey(const MDNode *Node, const Type *Ty)
    : Node(Node), Ty(Ty) {}

bool TBAASimpleSolverHandler::MapKey::operator<(const MapKey &Key) const {
  if (Node != Key.Node)
    return Node < Key.Node;
  return Ty < Key.Ty;
}
