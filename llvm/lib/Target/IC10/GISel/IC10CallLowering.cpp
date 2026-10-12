// IC10

#include "IC10CallLowering.h"
#include "IC10InstrInfo.h"
#include "IC10RegisterInfo.h"
#include "IC10Subtarget.h"
#include "IC10TargetLowering.h"
#include "MCTargetDesc/IC10MCTargetDesc.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetCallingConv.h"
#include "llvm/CodeGenTypes/MachineValueType.h"
#include <cassert>

using namespace llvm;

namespace {

static const TargetRegisterClass *getIC10ArgRegClass(const CCValAssign &VA) {
  MVT Ty = VA.getValVT().SimpleTy;
  if (Ty.isFloatingPoint()) {
    return &IC10::FPRRegClass;
  }
  if (Ty.isInteger()) {
    return &IC10::GPRRegClass;
  }
  llvm_unreachable("unsupported argument type");
}

struct IC10FormalArgHandler : public IC10IncomingValueHandler {
  IC10FormalArgHandler(MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI)
      : IC10IncomingValueHandler(MIRBuilder, MRI) {}
};

struct CallReturnHandler : public IC10IncomingValueHandler {
  CallReturnHandler(MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI,
                    MachineInstrBuilder &MIB)
      : IC10IncomingValueHandler(MIRBuilder, MRI), MIB(MIB) {}

private:
  void assignValueToReg(Register ValVReg, Register PhysReg,
                        const CCValAssign &VA,
                        ISD::ArgFlagsTy Flags = {}) override;

  MachineInstrBuilder &MIB;
};

} // end anonymous namespace

IC10CallLowering::IC10CallLowering(const IC10TargetLowering &TLI)
    : CallLowering(&TLI) {}

struct IC10OutgoingArgHandler : public CallLowering::OutgoingValueHandler {
  IC10OutgoingArgHandler(MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI,
                         MachineInstrBuilder MIB)
      : OutgoingValueHandler(MIRBuilder, MRI), MIB(MIB),
        DL(MIRBuilder.getMF().getDataLayout()),
        STI(MIRBuilder.getMF().getSubtarget<IC10Subtarget>()) {}

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        const CCValAssign &VA,
                        ISD::ArgFlagsTy Flags = {}) override {
    // TODO: IC10 Call Lowering
    const TargetRegisterClass *RC = getIC10ArgRegClass(VA);
    MRI.setRegClass(ValVReg, RC);
    Register ExtReg = extendRegister(ValVReg, VA);
    MIRBuilder.buildCopy(PhysReg, ExtReg);
    MIB.addUse(PhysReg, RegState::Implicit);
  }

  void assignValueToAddress(Register ValVReg, Register Addr, LLT MemTy,
                            const MachinePointerInfo &MPO,
                            const CCValAssign &VA) override {
    // TODO: IC10 Call Lowering
    MachineFunction &MF = MIRBuilder.getMF();
    Register ExtReg = extendRegister(ValVReg, VA);

    auto *MMO = MF.getMachineMemOperand(MPO, MachineMemOperand::MOStore, MemTy,
                                        inferAlignFromPtrInfo(MF, MPO));
    MIRBuilder.buildStore(ExtReg, Addr, *MMO);
  }

  Register getStackAddress(uint64_t Size, int64_t Offset,
                           MachinePointerInfo &MPO,
                           ISD::ArgFlagsTy Flags) override {
    // TODO: IC10 Call Lowering
    LLT p0 = LLT::pointer(0, DL.getPointerSizeInBits(0));
    LLT SType = LLT::scalar(DL.getPointerSizeInBits(0));
    Register StackReg = IC10::R15;
    auto SPReg = MIRBuilder.buildCopy(p0, StackReg).getReg(0);
    auto OffsetReg = MIRBuilder.buildConstant(SType, Offset);
    auto AddrReg = MIRBuilder.buildPtrAdd(p0, SPReg, OffsetReg);
    MPO = MachinePointerInfo::getStack(MIRBuilder.getMF(), Offset);
    return AddrReg.getReg(0);
  }

  MachineInstrBuilder MIB;
  const DataLayout &DL;
  const IC10Subtarget &STI;
};

bool IC10CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                   const Value *Val, ArrayRef<Register> VRegs,
                                   FunctionLoweringInfo &FLI,
                                   Register SwiftErrorVReg) const {
  assert(SwiftErrorVReg == 0 && "IC10 does not support SwiftErrorVReg yet.");

  // TODO: IC10 Call Lowering
  auto MIB = MIRBuilder.buildInstrNoInsert(IC10::RET_PSEUDO);
  bool Success = true;
  MachineFunction &MF = MIRBuilder.getMF();
  const Function &F = MF.getFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const IC10TargetLowering &TLI = *getTLI<IC10TargetLowering>();
  CCAssignFn *AssignFn = TLI.getCCAssignFn(true);
  auto &DL = F.getDataLayout();
  if (!VRegs.empty()) {
    SmallVector<ArgInfo, 8> SplitArgs;
    ArgInfo OrigArg{VRegs, Val->getType(), 0};
    setArgFlags(OrigArg, AttributeList::ReturnIndex, DL, F);
    splitToValueTypes(OrigArg, SplitArgs, DL, F.getCallingConv());
    OutgoingValueAssigner ArgAssigner(AssignFn);
    IC10OutgoingArgHandler ArgHandler(MIRBuilder, MRI, MIB);
    Success = determineAndHandleAssignments(ArgHandler, ArgAssigner, SplitArgs,
                                            MIRBuilder, F.getCallingConv(),
                                            F.isVarArg());
  }
  MIRBuilder.insertInstr(MIB);
  return Success;
}

bool IC10CallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                            const Function &F,
                                            ArrayRef<ArrayRef<Register>> VRegs,
                                            FunctionLoweringInfo &FLI) const {

  // TODO: IC10 Call Lowering
  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const auto &DL = F.getDataLayout();
  auto &TLI = *getTLI<IC10TargetLowering>();

  const CallingConv::ID CallConv = F.getCallingConv();

  SmallVector<ArgInfo, 8> SplitArgs;
  unsigned I = 0;
  for (const auto &Arg : F.args()) {
    ArgInfo OrigArg{VRegs[I], Arg.getType(), I};
    setArgFlags(OrigArg, I + AttributeList::FirstArgIndex, DL, F);
    splitToValueTypes(OrigArg, SplitArgs, DL, CallConv);
    ++I;
  }

  CCAssignFn *AssignFn = TLI.getCCAssignFn(false);
  IncomingValueAssigner ArgAssigner(AssignFn);
  IC10FormalArgHandler ArgHandler(MIRBuilder, MRI);
  return determineAndHandleAssignments(ArgHandler, ArgAssigner, SplitArgs,
                                       MIRBuilder, CallConv, F.isVarArg());
}

void IC10IncomingValueHandler::assignValueToReg(Register ValVReg,
                                                Register PhysReg,
                                                const CCValAssign &VA,
                                                ISD::ArgFlagsTy Flags) {
  // TODO: IC10 Call Lowering
  const TargetRegisterClass *RC = getIC10ArgRegClass(VA);
  MRI.setRegClass(ValVReg, RC);
  MIRBuilder.getMRI()->addLiveIn(PhysReg);
  MIRBuilder.getMBB().addLiveIn(PhysReg);
  IncomingValueHandler::assignValueToReg(ValVReg, PhysReg, VA);
}

void IC10IncomingValueHandler::assignValueToAddress(
    Register ValVReg, Register Addr, LLT MemTy, const MachinePointerInfo &MPO,
    const CCValAssign &VA) {
  // TODO: IC10 Call Lowering
  MachineFunction &MF = MIRBuilder.getMF();
  auto *MMO = MF.getMachineMemOperand(MPO, MachineMemOperand::MOLoad, MemTy,
                                      inferAlignFromPtrInfo(MF, MPO));
  MIRBuilder.buildLoad(ValVReg, Addr, *MMO);
}

Register IC10IncomingValueHandler::getStackAddress(uint64_t Size,
                                                   int64_t Offset,
                                                   MachinePointerInfo &MPO,
                                                   ISD::ArgFlagsTy Flags) {
  // TODO: IC10 Call Lowering
  auto &MFI = MIRBuilder.getMF().getFrameInfo();
  const bool IsImmutable = !Flags.isByVal();
  int FI = MFI.CreateFixedObject(Size, Offset, IsImmutable);
  MPO = MachinePointerInfo::getFixedStack(MIRBuilder.getMF(), FI);

  // Build Frame Index
  llvm::LLT FramePtr = LLT::pointer(
      0, MIRBuilder.getMF().getDataLayout().getPointerSizeInBits());
  MachineInstrBuilder AddrReg = MIRBuilder.buildFrameIndex(FramePtr, FI);
  StackUsed = std::max(StackUsed, Size + Offset);
  return AddrReg.getReg(0);
}

void CallReturnHandler::assignValueToReg(Register ValVReg, Register PhysReg,
                                         const CCValAssign &VA,
                                         ISD::ArgFlagsTy Flags) {
  // TODO: IC10 Call Lowering
  const TargetRegisterClass *RC = getIC10ArgRegClass(VA);
  MRI.setRegClass(ValVReg, RC);
  MIB.addDef(PhysReg, RegState::Implicit);
  MIRBuilder.buildCopy(ValVReg, PhysReg);
}

bool IC10CallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                 CallLoweringInfo &Info) const {
  // TODO: IC10 Call Lowering
  MachineFunction &MF = MIRBuilder.getMF();
  Function &F = MF.getFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  auto &DL = F.getDataLayout();
  const IC10TargetLowering &TLI = *getTLI<IC10TargetLowering>();
  const IC10Subtarget &STI = MF.getSubtarget<IC10Subtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  const IC10RegisterInfo *TRI = STI.getRegisterInfo();

  SmallVector<ArgInfo, 8> OutArgs;
  for (auto &OrigArg : Info.OrigArgs)
    splitToValueTypes(OrigArg, OutArgs, DL, Info.CallConv);

  SmallVector<ArgInfo, 8> InArgs;
  if (!Info.OrigRet.Ty->isVoidTy())
    splitToValueTypes(Info.OrigRet, InArgs, DL, Info.CallConv);

  unsigned AdjStackDown = TII.getCallFrameSetupOpcode();
  auto CallSeqStart = MIRBuilder.buildInstr(AdjStackDown);

  auto MIB = MIRBuilder.buildInstrNoInsert(IC10::CALL_PSEUDO)
                 .add(Info.Callee)
                 .addRegMask(TRI->getCallPreservedMask(MF, Info.CallConv));

  CCAssignFn *AssignFn = TLI.getCCAssignFn(false);
  OutgoingValueAssigner Assigner(AssignFn);
  IC10OutgoingArgHandler Handler(MIRBuilder, MRI, MIB);
  if (!determineAndHandleAssignments(Handler, Assigner, OutArgs, MIRBuilder,
                                     Info.CallConv, Info.IsVarArg))
    return false;

  if (Info.Callee.isReg())
    constrainOperandRegClass(MF, *TRI, MRI, *STI.getInstrInfo(),
                             *STI.getRegBankInfo(), *MIB, MIB->getDesc(),
                             Info.Callee, 0);

  MIRBuilder.insertInstr(MIB);

  if (!Info.OrigRet.Ty->isVoidTy()) {
    CCAssignFn *RetAssignFn = TLI.getCCAssignFn(true);

    IncomingValueAssigner Assigner(RetAssignFn, RetAssignFn);
    CallReturnHandler Handler(MIRBuilder, MRI, MIB);
    if (!determineAndHandleAssignments(Handler, Assigner, InArgs, MIRBuilder,
                                       Info.CallConv, Info.IsVarArg))
      return false;
  }

  CallSeqStart.addImm(Assigner.StackSize).addImm(0);

  unsigned AdjStackUp = TII.getCallFrameDestroyOpcode();
  MIRBuilder.buildInstr(AdjStackUp).addImm(Assigner.StackSize).addImm(0);

  return true;
}
