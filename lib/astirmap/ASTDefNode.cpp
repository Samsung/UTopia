#include "ftg/astirmap/ASTDefNode.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "clang/AST/ExprCXX.h"

using namespace clang;

namespace ftg {

ASTDefNode::ASTDefNode(VarDecl &D, ASTUnit &Unit) {
  Assignee =
      std::make_unique<ASTNode>(ASTNode::DECL, DynTypedNode::create(D), Unit);
  if (!Assignee)
    throw std::runtime_error("Failed to create ASTDeclNode");

  if (auto *E = D.getInit()) {
    Assigned = std::make_unique<ASTNode>(ASTNode::STMT,
                                         DynTypedNode::create(*E), Unit);
    assert(Assigned && "Unexpected Program State");

    // NOTE: If initializer of declaration has same location as declaration,
    // it should be considered as implicitly generated initializer.
    // Thus, it should be considered that it has no intiizlier.
    if (Assignee->getIndex() == Assigned->getIndex())
      Assigned.reset();
  }
  SourceLoc = LocIndex(Unit.getSourceManager(), D.getLocation());
}

ASTDefNode::ASTDefNode(BinaryOperator &B, ASTUnit &Unit) {
  if (!B.isAssignmentOp())
    throw std::runtime_error("Unsupported binary operator");

  auto *LHS = B.getLHS();
  auto *RHS = B.getRHS();
  assert(LHS && RHS && "Unexpected Program State");

  while (auto *BinOp =
             llvm::dyn_cast_or_null<BinaryOperator>(RHS->IgnoreCasts())) {
    if (!BinOp->isAssignmentOp())
      break;

    RHS = BinOp->getRHS();
    assert(RHS && "Unexpected Program State");
  }

  Assignee = std::make_unique<ASTNode>(ASTNode::STMT,
                                       DynTypedNode::create(*LHS), Unit);
  Assigned = std::make_unique<ASTNode>(ASTNode::STMT,
                                       DynTypedNode::create(*RHS), Unit);
  assert(Assignee && Assigned && "Unexpected Program State");

  SourceLoc = LocIndex(Unit.getSourceManager(), B.getOperatorLoc());
}

ASTDefNode::ASTDefNode(Expr &E, ASTUnit &Unit) {
  Assignee =
      std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), Unit);
  assert(Assignee && "Unexpected Program State");

  Assigned =
      std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), Unit);
  assert(Assignee && "Unexpected Program State");

  SourceLoc = LocIndex(Unit.getSourceManager(), util::getDebugLoc(E));
}

ASTDefNode::ASTDefNode(Expr &E, unsigned ArgIdx, ASTUnit &Unit) {
  if (util::isImplicitArgument(E, ArgIdx) || util::isDefaultArgument(E, ArgIdx))
    throw std::runtime_error("Not supported argument type");

  auto Args = util::getArgExprs(E);
  if (Args.size() < ArgIdx)
    throw std::runtime_error("Not supported argument number");

  auto *Arg = Args[ArgIdx];
  if (!Arg)
    throw std::runtime_error("Unexpected null pointer from LLVM API");

  Assigned = std::make_unique<ASTNode>(ASTNode::PARAM,
                                       DynTypedNode::create(*Arg), Unit);
  Assignee =
      std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), Unit);
  if (!Assigned || !Assignee)
    throw std::runtime_error("Fail to create Assignee and Assigned");

  SourceLoc = LocIndex(Unit.getSourceManager(), Arg->getBeginLoc());
}

ASTDefNode::ASTDefNode(ReturnStmt &S, ASTUnit &Unit) {
  auto *RetValue = S.getRetValue();
  if (!RetValue)
    throw std::runtime_error("Unsupported ReturnStmt state");

  Assignee =
      std::make_unique<ASTNode>(ASTNode::STMT, DynTypedNode::create(S), Unit);
  Assigned = std::make_unique<ASTNode>(ASTNode::STMT,
                                       DynTypedNode::create(*RetValue), Unit);
  SourceLoc = LocIndex(Unit.getSourceManager(), S.getBeginLoc());
}

ASTDefNode::ASTDefNode(CXXCtorInitializer &CCI, ASTUnit &Unit) {
  auto *Init = CCI.getInit();
  if (!Init)
    throw std::runtime_error("Unsupported CXXCtorInitializer state");
  Assignee = std::make_unique<ASTNode>(ASTNode::CTORINIT,
                                       DynTypedNode::create(CCI), Unit);
  Assigned = std::make_unique<ASTNode>(ASTNode::STMT,
                                       DynTypedNode::create(*Init), Unit);
  if (!Assigned || !Assignee)
    throw std::runtime_error("Fail to create Assignee and Assigned");

  SourceLoc = LocIndex(Unit.getSourceManager(), CCI.getSourceLocation());
}

LocIndex ASTDefNode::getLocIndex() const { return SourceLoc; }

const ASTNode &ASTDefNode::getAssignee() const {
  assert(Assignee && "Unexpected Program State");
  return *Assignee;
}

const ASTNode *ASTDefNode::getAssigned() const { return Assigned.get(); }

const ASTNode *ASTDefNode::getNodeForType() const {
  if (!Assigned)
    return Assignee.get();

  assert(Assignee && "Unexpected Program State");
  if ((Assignee->getNodeType() == ASTNode::CALL) &&
      (Assigned->getNodeType() != ASTNode::CALL))
    return Assigned.get();

  if (const auto *E = Assigned->getNode().get<Expr>()) {
    if (E->isNullPointerConstant(Assigned->getASTUnit().getASTContext(),
                                 Expr::NPC_NeverValueDependent)) {
      return Assignee.get();
    }
  }
  return Assigned.get();
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const ASTDefNode &Src) {
  O << "Index: " << Src.getLocIndex().getIDAsString() << "\n"
    << "Assignee: " << *Src.Assignee << "\nAssigned: ";
  if (Src.Assigned)
    O << *Src.Assigned << "\n";
  else
    O << "Not specified\n";
  return O;
}

} // namespace ftg
