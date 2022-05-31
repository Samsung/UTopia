#include "ftg/inputfilter/ExternalFilter.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

const std::string ExternalFilter::FilterName = "ExternalFilter";

ExternalFilter::ExternalFilter(const DirectionAnalysisReport &DirectionReport,
                               std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)),
      DirectionReport(DirectionReport) {}

bool ExternalFilter::check(const ASTIRNode &Node) const {
  if (isCallReturn(Node.AST))
    return true;

  auto Def = Node.IR.getDefinition();
  auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(Def.first);
  if (!CB)
    return false;

  // Generally, definition indicates return of call when an instruction is
  // callbase and its index is -1 in DFNode. However, there is an exception case
  // for example, consant memset for aggregate type. This case is an assignment
  // definition to aggregate type variable. Thus, this case is not considered
  // externally assigned because call return case is checked using
  // Node.AST.isCallReturn() above call statement in this function.
  if (Def.second == -1)
    return false;

  auto *CV = CB->getCalledValue();
  if (!CV)
    return false;

  CV = CV->stripPointerCasts();
  auto *F = llvm::dyn_cast_or_null<llvm::Function>(CV);
  if (!F)
    return false;

  auto *A = F->getArg(Def.second);
  if (!A)
    return false;

  if (!DirectionReport.has(*A))
    return false;

  return DirectionReport.get(*A) == Dir_Out;
}

bool ExternalFilter::isCallReturn(const ASTDefNode &Node) const {
  auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return false;

  const auto *E = Assigned->getNode().get<clang::Expr>();
  if (!util::isCallRelatedExpr(*const_cast<clang::Expr *>(E)))
    return false;

  auto *FuncDecl = util::getFunctionDecl(*const_cast<clang::Expr *>(E));
  if (!FuncDecl)
    return true;

  if (llvm::isa<clang::CXXConstructorDecl>(FuncDecl) &&
      !util::regex(FuncDecl->getQualifiedNameAsString(), "std::.*basic_string")
           .empty())
    return false;

  return true;
}
