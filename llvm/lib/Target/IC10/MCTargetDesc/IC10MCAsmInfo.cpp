// IC10
#include "IC10MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCTargetOptions.h"

using namespace llvm;

void IC10MCAsmInfo::anchor() {}

IC10MCAsmInfo::IC10MCAsmInfo(const MCTargetOptions &Options): MCAsmInfo(Options) {
  CommentString = "//";
  InternalSymbolPrefix = "";
  HasDotTypeDotSizeDirective = false;
}
