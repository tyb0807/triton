//===-- LLVMABIGuard.h - Reject an LLVM layout mismatch at compile time ---===//
//
// This library is configured and built separately from the LLVM it links
// against, so nothing makes their assertion settings agree by construction.
// They have to agree: LLVM's public headers gate *data members* on bare
// NDEBUG, so the two sides otherwise disagree about object layout.
//
//   struct MCSchedClassDesc       // llvm/MC/MCSchedule.h
//     uint32_t NameOff;           // present only when !NDEBUG
//
//   class ScheduleDAG             // llvm/CodeGen/ScheduleDAG.h
//     bool StressSched;           // a static constant when NDEBUG
//
// Measured against this LLVM pin, defining NDEBUG shrinks MCSchedClassDesc
// from 16 bytes to 12 and ScheduleDAGMILive from 6080 to 6072.
//
// MCSchedClassDesc is the dangerous one. The AMDGPU subtarget's
// SchedClassTable is an array compiled into libLLVMAMDGPUDesc.a, and
// MCSchedModel::getSchedClassDesc indexes it from an inline header function,
// which means the stride comes from whichever translation unit does the
// indexing. A mismatch there misreads every scheduling class instead of
// crashing -- it degrades scheduling silently. ScheduleDAG is merely louder:
// ~ScheduleDAGInstrs frees a pointer read at the wrong offset.
//
// LLVM's own link-time guard (llvm/Config/abi-breaking.h) does not cover this.
// It compares LLVM_ENABLE_ABI_BREAKING_CHECKS, which comes from a generated
// header and so reads identically on both sides no matter what NDEBUG is.
//
// LLVM_ENABLE_ABI_BREAKING_CHECKS is still the right thing to test against,
// because it is generated from the linked LLVM's own configuration and
// LLVM_ABI_BREAKING_CHECKS defaults to WITH_ASSERTS. A build that overrides it
// to FORCE_ON or FORCE_OFF can define TRITON_AMD_SKIP_LLVM_ABI_GUARD and take
// responsibility for matching NDEBUG itself.
//
// The other axis these members key on, LLVM_ENABLE_DUMP, needs no guard: it is
// recorded in llvm/Config/llvm-config.h, so consumers inherit the linked
// LLVM's value automatically.
//
// Including this from one translation unit is enough. The settings it checks
// are properties of the target, not of a single file, so one failing compile
// reports the misconfiguration for the whole library.
//
//===----------------------------------------------------------------------===//

#ifndef TRITON_AMD_BACKEND_CODEGEN_LLVMABIGUARD_H
#define TRITON_AMD_BACKEND_CODEGEN_LLVMABIGUARD_H

#include "llvm/Config/abi-breaking.h"

#ifndef TRITON_AMD_SKIP_LLVM_ABI_GUARD

#if LLVM_ENABLE_ABI_BREAKING_CHECKS && defined(NDEBUG)
#error "NDEBUG is defined but the linked LLVM was built with assertions. "    \
       "LLVM's headers gate data members on NDEBUG, so this library and "     \
       "libLLVM*.a would disagree about object layout. Build this library "   \
       "with -UNDEBUG (the CMakeLists does this for you), or link an LLVM "   \
       "built without assertions."
#endif

#if !LLVM_ENABLE_ABI_BREAKING_CHECKS && !defined(NDEBUG)
#error "NDEBUG is not defined but the linked LLVM was built without "         \
       "assertions. LLVM's headers gate data members on NDEBUG, so this "     \
       "library and libLLVM*.a would disagree about object layout. Build "    \
       "this library with -DNDEBUG (the CMakeLists does this for you), or "   \
       "link an LLVM built with assertions."
#endif

#endif // TRITON_AMD_SKIP_LLVM_ABI_GUARD

#endif // TRITON_AMD_BACKEND_CODEGEN_LLVMABIGUARD_H
