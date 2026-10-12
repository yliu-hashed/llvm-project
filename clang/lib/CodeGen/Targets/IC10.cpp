// IC10

#include "ABIInfoImpl.h"
#include "TargetInfo.h"
#include "clang/Basic/BuiltinTraits.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {
class IC10ABIInfo : public DefaultABIInfo {
public:
  IC10ABIInfo(CodeGen::CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}
};

class IC10TargetCodeGenInfo : public TargetCodeGenInfo {
public:
  IC10TargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<IC10ABIInfo>(CGT)) {}
};

} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createIC10TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<IC10TargetCodeGenInfo>(CGM.getTypes());
}
