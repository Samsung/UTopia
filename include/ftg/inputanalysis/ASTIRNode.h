#ifndef FTG_INPUTANALYSIS_ASTIRNODE_H
#define FTG_INPUTANALYSIS_ASTIRNODE_H

#include "ftg/astirmap/ASTDefNode.h"
#include "ftg/rootdefanalysis/RDNode.h"

namespace ftg {

struct ASTIRNode {
  ASTDefNode &AST;
  const RDNode IR;

  ASTIRNode(ASTDefNode &AST, const RDNode &IR) : AST(AST), IR(IR) {}
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_ASTIRNODE_H
