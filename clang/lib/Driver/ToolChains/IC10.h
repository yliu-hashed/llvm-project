// IC10

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_IC10_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_IC10_H

#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY IC10ToolChain : public ToolChain {
public:
  IC10ToolChain(const Driver &D, const llvm::Triple &Triple,
                const llvm::opt::ArgList &Args)
      : ToolChain(D, Triple, Args) {}

  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }
  bool IsMathErrnoDefault() const override { return false; }
  bool isPICDefaultForced() const override { return false; }
  bool IsIntegratedAssemblerDefault() const override { return true; }
};

} // end namespace toolchains
} // end namespace driver
} // end namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_IC10_H
