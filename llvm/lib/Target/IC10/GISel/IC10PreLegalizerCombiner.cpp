// IC10

#include "IC10.h"
#include "IC10LegalizerInfo.h"
#include "IC10Subtarget.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GIMatchTableExecutorImpl.h"
#include "llvm/CodeGen/GlobalISel/GISelValueTracking.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Target/TargetMachine.h"

#define GET_GICOMBINER_DEPS
#include "IC10GenPreLegalizeGICombiner.inc"
#undef GET_GICOMBINER_DEPS

#define DEBUG_TYPE "ic10-prelegalizer-combiner"

using namespace llvm;
using namespace MIPatternMatch;
namespace {

#define GET_GICOMBINER_TYPES
#include "IC10GenPreLegalizeGICombiner.inc"
#undef GET_GICOMBINER_TYPES

class IC10PreLegalizerCombinerImpl : public Combiner {
protected:
  const IC10PreLegalizerCombinerImplRuleConfig &RuleConfig;
  const IC10Subtarget &STI;
  const CombinerHelper Helper;

public:
  IC10PreLegalizerCombinerImpl(
      MachineFunction &MF, CombinerInfo &CInfo, GISelValueTracking &VT,
      GISelCSEInfo *CSEInfo,
      const IC10PreLegalizerCombinerImplRuleConfig &RuleConfig,
      const IC10Subtarget &STI, MachineDominatorTree *MDT,
      const LegalizerInfo *LI);

  static const char *getName() { return "IC10PreLegalizerCombinerImpl"; }

  bool tryCombineAllImpl(MachineInstr &MI) const;
  bool tryCombineAll(MachineInstr &I) const override;

private:
#define GET_GICOMBINER_CLASS_MEMBERS
#include "IC10GenPreLegalizeGICombiner.inc"
#undef GET_GICOMBINER_CLASS_MEMBERS
};

#define GET_GICOMBINER_IMPL
#include "IC10GenPreLegalizeGICombiner.inc"
#undef GET_GICOMBINER_IMPL

IC10PreLegalizerCombinerImpl::IC10PreLegalizerCombinerImpl(
    MachineFunction &MF, CombinerInfo &CInfo, GISelValueTracking &VT,
    GISelCSEInfo *CSEInfo,
    const IC10PreLegalizerCombinerImplRuleConfig &RuleConfig,
    const IC10Subtarget &STI, MachineDominatorTree *MDT,
    const LegalizerInfo *LI)
    : Combiner(MF, CInfo, &VT, CSEInfo), RuleConfig(RuleConfig), STI(STI),
      Helper(Observer, B, /*IsPreLegalize*/ true, &VT, MDT, LI),
#define GET_GICOMBINER_CONSTRUCTOR_INITS
#include "IC10GenPreLegalizeGICombiner.inc"
#undef GET_GICOMBINER_CONSTRUCTOR_INITS
{
}

static bool runCombiner(MachineFunction &MF,
                        function_ref<GISelCSEInfo *()> GetCSEInfo,
                        function_ref<GISelValueTracking *()> GetVT,
                        function_ref<MachineDominatorTree *()> GetMDT,
                        bool EnableOpt) {
  IC10PreLegalizerCombinerImplRuleConfig RuleConfig;
  if (!RuleConfig.parseCommandLineOption())
    reportFatalUsageError("Invalid rule identifier");

  // If the ISel pipeline failed, do not bother running that pass.
  if (MF.getProperties().hasFailedISel())
    return false;

  const IC10Subtarget &STI = MF.getSubtarget<IC10Subtarget>();
  const Function &F = MF.getFunction();
  CombinerInfo CInfo(/*AllowIllegalOps=*/true, /*ShouldLegalizeIllegal=*/false,
                     nullptr, EnableOpt, F.hasOptSize(), F.hasMinSize());
  // Disable fixed-point iteration to reduce compile-time
  CInfo.MaxIterations = 1;
  CInfo.ObserverLvl = CombinerInfo::ObserverLevel::SinglePass;
  // This is the first Combiner, so the input IR might contain dead
  // instructions.
  CInfo.EnableFullDCE = true;

  GISelValueTracking *VT = GetVT();
  GISelCSEInfo *CSEInfo = GetCSEInfo();
  MachineDominatorTree *MDT = GetMDT();
  IC10PreLegalizerCombinerImpl Impl(MF, CInfo, *VT, CSEInfo, RuleConfig, STI,
                                    MDT, STI.getLegalizerInfo());
  return Impl.combineMachineInstrs();
}

} // end anonymous namespace

PreservedAnalyses
IC10PreLegalizerCombinerPass::run(MachineFunction &MF,
                                  MachineFunctionAnalysisManager &MFAM) {
  bool IsOptLevelNone = MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
                        shouldSkipOptimizationForOptBisect(MF.getFunction());

  if (!runCombiner(
          MF, [&]() { return MFAM.getResult<GISelCSEAnalysis>(MF).get(); },
          [&]() { return &MFAM.getResult<GISelValueTrackingAnalysis>(MF); },
          [&]() -> MachineDominatorTree * {
            return IsOptLevelNone
                       ? nullptr
                       : &MFAM.getResult<MachineDominatorTreeAnalysis>(MF);
          },
          /*EnableOpt=*/!IsOptLevelNone))
    return PreservedAnalyses::all();

  PreservedAnalyses PA = getMachineFunctionPassPreservedAnalyses();
  PA.preserveSet<CFGAnalyses>();
  PA.preserve<GISelValueTrackingAnalysis>();
  PA.preserve<GISelCSEAnalysis>();
  return PA;
}
