// IC10

#include "IC10.h"
#include "IC10Subtarget.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GIMatchTableExecutorImpl.h"
#include "llvm/CodeGen/GlobalISel/GISelValueTracking.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Analysis.h"

#define GET_GICOMBINER_DEPS
#include "IC10GenPostLegalizeGICombiner.inc"
#undef GET_GICOMBINER_DEPS

#define DEBUG_TYPE "ic10-postlegalizer-combiner"

using namespace llvm;

namespace {

#define GET_GICOMBINER_TYPES
#include "IC10GenPostLegalizeGICombiner.inc"
#undef GET_GICOMBINER_TYPES

class IC10PostLegalizerCombinerImpl : public Combiner {
protected:
  const CombinerHelper Helper;
  const IC10PostLegalizerCombinerImplRuleConfig &RuleConfig;
  const IC10Subtarget &STI;

public:
  IC10PostLegalizerCombinerImpl(
      MachineFunction &MF, CombinerInfo &CInfo, GISelValueTracking &VT,
      GISelCSEInfo *CSEInfo,
      const IC10PostLegalizerCombinerImplRuleConfig &RuleConfig,
      const IC10Subtarget &STI, MachineDominatorTree *MDT,
      const LegalizerInfo *LI);

  static const char *getName() { return "IC10PostLegalizerCombiner"; }

  bool tryCombineAll(MachineInstr &I) const override;

  bool matchRedundantBooleanMask(MachineInstr &MI) const;

private:
#define GET_GICOMBINER_CLASS_MEMBERS
#include "IC10GenPostLegalizeGICombiner.inc"
#undef GET_GICOMBINER_CLASS_MEMBERS
};

#define GET_GICOMBINER_IMPL
#include "IC10GenPostLegalizeGICombiner.inc"
#undef GET_GICOMBINER_IMPL

IC10PostLegalizerCombinerImpl::IC10PostLegalizerCombinerImpl(
    MachineFunction &MF, CombinerInfo &CInfo, GISelValueTracking &VT,
    GISelCSEInfo *CSEInfo,
    const IC10PostLegalizerCombinerImplRuleConfig &RuleConfig,
    const IC10Subtarget &STI, MachineDominatorTree *MDT,
    const LegalizerInfo *LI)
    : Combiner(MF, CInfo, &VT, CSEInfo),
      Helper(Observer, B, /*IsPreLegalize*/ false, &VT, MDT, LI),
      RuleConfig(RuleConfig), STI(STI),
#define GET_GICOMBINER_CONSTRUCTOR_INITS
#include "IC10GenPostLegalizeGICombiner.inc"
#undef GET_GICOMBINER_CONSTRUCTOR_INITS
{
}

} // end anonymous namespace

static bool
runCombinerOnMachineFunction(MachineFunction &MF,
                             function_ref<bool()> ShouldSkip,
                             function_ref<GISelValueTracking *()> GetVT,
                             function_ref<MachineDominatorTree *()> GetMDT,
                             function_ref<GISelCSEInfo *()> GetCSEInfo) {
  if (MF.getProperties().hasFailedISel())
    return false;
  assert(MF.getProperties().hasLegalized() && "Expected a legalized function?");
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOptLevel::None && !ShouldSkip;

  IC10PostLegalizerCombinerImplRuleConfig RuleConfig;
  if (!RuleConfig.parseCommandLineOption())
    reportFatalUsageError("Invalid rule identifier");

  const IC10Subtarget &ST = MF.getSubtarget<IC10Subtarget>();
  const auto *LI = ST.getLegalizerInfo();

  GISelValueTracking *VT = GetVT();
  MachineDominatorTree *MDT = GetMDT();
  GISelCSEInfo *CSEInfo = GetCSEInfo();

  CombinerInfo CInfo(/*AllowIllegalOps*/ true, /*ShouldLegalizeIllegal*/ false,
                     /*LegalizerInfo*/ nullptr, EnableOpt, F.hasOptSize(),
                     F.hasMinSize());
  // Disable fixed-point iteration to reduce compile-time
  CInfo.ObserverLvl = CombinerInfo::ObserverLevel::SinglePass;
  // Legalizer performs DCE, so a full DCE pass is unnecessary.
  CInfo.EnableFullDCE = false;
  IC10PostLegalizerCombinerImpl Impl(MF, CInfo, *VT, CSEInfo, RuleConfig, ST,
                                     MDT, LI);
  return Impl.combineMachineInstrs();
}

PreservedAnalyses
IC10PostLegalizerCombinerPass::run(MachineFunction &MF,
                                   MachineFunctionAnalysisManager &MFAM) {
  bool Changed = runCombinerOnMachineFunction(
      MF, [&]() { return MF.getFunction().hasOptNone(); },
      [&]() { return &MFAM.getResult<GISelValueTrackingAnalysis>(MF); },
      [&]() { return &MFAM.getResult<MachineDominatorTreeAnalysis>(MF); },
      [&]() { return MFAM.getResult<GISelCSEAnalysis>(MF).get(); });
  return Changed ? getMachineFunctionPassPreservedAnalyses()
                       .preserveSet<CFGAnalyses>()
                 : PreservedAnalyses::all();
}
