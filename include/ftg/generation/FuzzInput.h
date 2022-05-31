#ifndef FTG_GENERATION_FUZZINPUT_H
#define FTG_GENERATION_FUZZINPUT_H

#include "ftg/inputanalysis/Definition.h"
#include "llvm/Support/raw_ostream.h"

namespace ftg {

// Temporary relocation.
// This class should be merged into FuzzInput or
// functionality should be moved to assigner.
class AssignVar {
public:
  void setName(std::string VarName);
  void setType(Type &FTGType);
  void setNameSpace(std::string NameSpace);
  std::string getTypeStr();
  std::string getName();
  void addPtr();

private:
  std::string TypeStr;
  std::string PtrStr;
  std::string VarName;
  std::string NameSpace;
};

class FuzzInput {

public:
  const Definition *ArrayDef;
  const Definition *ArrayLenDef;

  FuzzInput(std::shared_ptr<const Definition> Def);
  unsigned getID() const;
  const Definition &getDef() const;
  std::shared_ptr<const Definition> &getDefPtr();
  Type &getFTGType();

  AssignVar &getLocalVar();
  AssignVar &getLocalPtrVar();

  std::string getFuzzVarName() const;
  std::string getProtoVarName() const;

private:
  std::shared_ptr<const Definition> Def;
  AssignVar LocalVar;
  AssignVar LocalPtrVar;

  std::string FuzzVarName;
  std::string ProtoVarName;
};

class FuzzNoInput {

public:
  /*! Enum values to specify reason why definition
      is classifed as FuzzNoInput. */
  typedef enum {
    Type_NullPtr,     /*! Definition is null value assignment. */
    Type_ReliedConst, /*! Definition is constant that is related to other
                          constant such as sizeof(var) / sizeof(var[0]). */
    Type_Dependent,   /*! Definition is assignment of a value whose definition
                          is not found within a given code. For example,
                          value comes from a external function result. */
    Type_MeshArrayRelation, /*! Definition has a complex array arraylen
                                relationship that may lead to side-effect.
                                Two array and two arraylen is complex
                                array arraylen relationship for example. */
    Type_TypeUnavailable,   /*! Definitions whose type definitions are not found
                                within a given code. */
    Type_UnsupportType,     /*! Definitions whose type is not supported yet */
    Type_GroupUnsupport,    /*! Definition has array arraylen relationship with
                                others that are classified as no input. */
    Type_NoNamedArray,      /*! Definition whose type is array has no name. */
    Type_Unknown,           /*! ETC. */
  } Type;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O,
                                       const FuzzNoInput &RHS);

  virtual std::string getExtraInfo() const;

  FuzzNoInput(Type Kind, std::shared_ptr<const Definition> Def);
  virtual ~FuzzNoInput() = default;

  unsigned getDefID() const;
  const Definition &getDef() const;
  Type getType() const;

private:
  Type Kind;
  std::shared_ptr<const Definition> Def;

protected:
  std::string getAsString(Type Kind) const;
};

class NoInputByGroup : public FuzzNoInput {

public:
  std::string getExtraInfo() const override;
  NoInputByGroup(std::shared_ptr<const Definition> Def,
                 unsigned DefIDAffectedBy);

private:
  unsigned DefIDAffectedBy;
};

class FuzzInputGenerator {

public:
  // Queries
  static bool isSupportedType(const Type &T);
  static bool isSupportedType(const StructType &T, ParamReport *P);
  static bool isSupportedType(Field &T, ParamReport *P);

  FuzzInputGenerator() = default;
  // TODO: Use const specifier for ASTInfo
  std::pair<std::map<unsigned, std::shared_ptr<FuzzInput>>,
            std::map<unsigned, std::shared_ptr<FuzzNoInput>>>
  generate(const std::vector<std::shared_ptr<const Definition>> &Defs);

private:
  std::map<unsigned, std::shared_ptr<FuzzInput>> FuzzInputMap;
  std::map<unsigned, std::shared_ptr<FuzzNoInput>> FuzzNoInputMap;

  static bool isMultiDimensionArray(const Type &T);

  std::map<unsigned, std::shared_ptr<const Definition>> generateDefMap(
      const std::vector<std::shared_ptr<const Definition>> &Defs) const;

  void updateGroups(std::vector<std::set<unsigned>> &Result,
                    const std::set<unsigned> &Group) const;

  std::pair<std::map<unsigned, std::shared_ptr<FuzzInput>>,
            std::map<unsigned, std::shared_ptr<FuzzNoInput>>>
  generateFuzzInputMap(
      const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap)
      const;
  void applyArrayProperty(
      std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
      std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
      const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap)
      const;
  std::vector<std::set<unsigned>> generateArrayGroups(
      const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap)
      const;
  void updateFuzzInputMapAndArrayGroups(
      std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
      std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
      std::vector<std::set<unsigned>> &ArrayGroups) const;
  void applyArrayPolicies(
      std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
      std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
      std::vector<std::set<unsigned>> &ArrayGroups) const;
  void
  updateArrayInfo(std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
                  const std::vector<std::set<unsigned>> &ArrayGroups) const;
};

class FuzzNoInputFactory {
public:
  FuzzNoInputFactory() = default;
  std::shared_ptr<FuzzNoInput>
  generate(std::shared_ptr<const Definition> Def) const;
  std::shared_ptr<FuzzNoInput> generate(std::shared_ptr<const Definition> Def,
                                        unsigned AffectedDefID) const;
  std::shared_ptr<FuzzNoInput>
  generateMeshArrayRelation(std::shared_ptr<const Definition> Def) const;
};

class FuzzInputFactory {

public:
  FuzzInputFactory() = default;
  std::shared_ptr<FuzzInput>
  generate(std::shared_ptr<const Definition> &Def) const;
  std::pair<std::shared_ptr<FuzzInput>, std::shared_ptr<FuzzInput>>
  generateArrayDefs(std::shared_ptr<const Definition> &Def) const;
};

} // end namespace ftg

#endif // FTG_GENERATION_FUZZINPUT_H
