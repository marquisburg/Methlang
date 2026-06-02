#include "codegen/binary/internal.h"
#include "codegen/binary/simd_internal.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Floating-point SIMD kernels (AVX2 + FMA3 horizontal sums, dot products, affine maps). Encoders live in simd_encoders.c; see simd_internal.h. */

/* Horizontal sum of base[0..len-1] doubles, ADDED to dest's prior value.
 * dest = float64 sum symbol, lhs = base pointer, rhs = element count. */
int code_generator_binary_emit_simd_sum_f64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec2 = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  /* rax=prior bits, rcx=walk, r9=end, r10=bytes-remaining scratch.
   * xmm3=scalar running total, ymm2/ymm4=packed accumulators, ymm0/1=scratch. */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !binary_emit_movq_xmm_reg(b, BINARY_XMM3, BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R8) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 3) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_avx_vpxor_ymm(b, 2, 2, 2) ||
      !wcs_avx_vpxor_ymm(b, 4, 4, 4)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec2) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  /* Two-accumulator unroll: 8 doubles/iter summed into the independent ymm2 and
   * ymm4 chains so the ~4-cycle vaddpd latency overlaps instead of serializing
   * a single accumulator. The 32-byte and scalar tiers below mop up the < 64B
   * remainder once the loop re-dispatches. */
  if (!wcs_patch_here(b, j_vec2) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RCX, 32) ||
      !wcs_avx_vaddpd_ymm(b, 2, 2, 0) ||
      !wcs_avx_vaddpd_ymm(b, 4, 4, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vaddpd_ymm(b, 2, 2, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movsd_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !binary_emit_addsd_xmm_xmm(b, BINARY_XMM3, BINARY_XMM0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 8)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  /* Fold the second accumulator in before the horizontal reduce. */
  if (!wcs_patch_here(b, j_done) ||
      !wcs_avx_vaddpd_ymm(b, 2, 2, 4) ||
      !wcs_reduce_pd_acc_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* Horizontal sum of base[0..len-1] floats, ADDED to dest's prior value.
 * dest = float32 sum symbol, lhs = base pointer, rhs = element count. */
int code_generator_binary_emit_simd_sum_f32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec2 = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !binary_emit_movd_xmm_reg(b, BINARY_XMM3, BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R8) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_avx_vpxor_ymm(b, 2, 2, 2) ||
      !wcs_avx_vpxor_ymm(b, 4, 4, 4)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec2) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  /* Two-accumulator unroll: 16 floats/iter into the independent ymm2 and ymm4
   * chains; the 32-byte and scalar tiers handle the < 64B remainder. */
  if (!wcs_patch_here(b, j_vec2) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RCX, 32) ||
      !wcs_avx_vaddps_ymm(b, 2, 2, 0) ||
      !wcs_avx_vaddps_ymm(b, 4, 4, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vaddps_ymm(b, 2, 2, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movss_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !binary_emit_addss_xmm_xmm(b, BINARY_XMM3, BINARY_XMM0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done) ||
      !wcs_avx_vaddps_ymm(b, 2, 2, 4) ||
      !wcs_reduce_ps_acc_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* Float64 dot product of a[0..n-1]*b[0..n-1], ADDED to dest's prior value.
 * dest = float64 sum, lhs = a, rhs = b, arguments[0] = element count. */
int code_generator_binary_emit_simd_dot_f64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec2 = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 1 || !instruction->arguments) {
    code_generator_set_error(generator, "Malformed simd_dot_f64");
    return 0;
  }
  b = &context->code;

  /* rcx=a walk, rdx=b walk, r9=a_end, r10=scratch, rax=prior/result.
   * xmm3=scalar total, ymm2/ymm4=packed FMA accumulators, ymm0/ymm1=scratch. */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !binary_emit_movq_xmm_reg(b, BINARY_XMM3, BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R8) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 3) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_avx_vpxor_ymm(b, 2, 2, 2) ||
      !wcs_avx_vpxor_ymm(b, 4, 4, 4)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec2) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  /* Two-accumulator FMA unroll: 8 elements/iter. Each vfmadd231pd folds a*b
   * into its accumulator in a single rounding step, and the two chains (ymm2,
   * ymm4) run independently so the FMA latency is hidden. */
  if (!wcs_patch_here(b, j_vec2) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vfmadd231pd_ymm(b, 2, 0, 1) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 32) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 32) ||
      !wcs_avx_vfmadd231pd_ymm(b, 4, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vfmadd231pd_ymm(b, 2, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movsd_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_movsd_xmm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_fmadd231sd(b, 3, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 8) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 8)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done) ||
      !wcs_avx_vaddpd_ymm(b, 2, 2, 4) ||
      !wcs_reduce_pd_acc_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* Float64 affine map: dst[i] = a * src[i] + b * dst[i] + c.
 * lhs=src, rhs=dst, arguments[0]=count, [1]=a, [2]=b, [3]=c. */
int code_generator_binary_emit_simd_affine_map_f64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 4 || !instruction->arguments) {
    code_generator_set_error(generator, "Malformed simd_affine_map_f64");
    return 0;
  }
  b = &context->code;

  /* rcx=src walk, rdx=dst walk, r9=src_end, r10=bytes remaining.
   * ymm4=a, ymm5=b, ymm3=c; ymm0/ymm1 are vector scratch. */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R8) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[1],
                                               BINARY_GP_RAX) ||
      !binary_emit_movq_xmm_reg(b, BINARY_XMM4, BINARY_GP_RAX) ||
      !wcs_avx_vbroadcastsd_ymm_xmm(b, 4, 4) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[2],
                                               BINARY_GP_RAX) ||
      !binary_emit_movq_xmm_reg(b, BINARY_XMM5, BINARY_GP_RAX) ||
      !wcs_avx_vbroadcastsd_ymm_xmm(b, 5, 5) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[3],
                                               BINARY_GP_RAX) ||
      !binary_emit_movq_xmm_reg(b, BINARY_XMM3, BINARY_GP_RAX) ||
      !wcs_avx_vbroadcastsd_ymm_xmm(b, 3, 3) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 3) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vmulpd_ymm(b, 0, 0, 4) ||
      !wcs_avx_vfmadd231pd_ymm(b, 0, 5, 1) ||
      !wcs_avx_vaddpd_ymm(b, 0, 0, 3) ||
      !wcs_avx_vmovups_mem_ymm(b, BINARY_GP_RDX, 0, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movsd_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_movsd_xmm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !binary_emit_mulsd_xmm_xmm(b, BINARY_XMM0, BINARY_XMM4) ||
      !wcs_fmadd231sd(b, 0, 5, 1) ||
      !binary_emit_addsd_xmm_xmm(b, BINARY_XMM0, BINARY_XMM3) ||
      !wcs_movsd_mem_xmm(b, BINARY_GP_RDX, 0, BINARY_XMM0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 8) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 8)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  return wcs_patch_here(b, j_done) && wcs_avx_vzeroupper(b);
}

/* Float32 affine map: dst[i] = a * src[i] + b * dst[i] + c.
 * lhs=src, rhs=dst, arguments[0]=count, [1]=a, [2]=b, [3]=c. */
int code_generator_binary_emit_simd_affine_map_f32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 4 || !instruction->arguments) {
    code_generator_set_error(generator, "Malformed simd_affine_map_f32");
    return 0;
  }
  b = &context->code;

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R8) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[1],
                                               BINARY_GP_RAX) ||
      !wcs_movd_xmm_reg(b, 4, BINARY_GP_RAX) ||
      !wcs_avx_vpbroadcastd_ymm(b, 4, 4) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[2],
                                               BINARY_GP_RAX) ||
      !wcs_movd_xmm_reg(b, 5, BINARY_GP_RAX) ||
      !wcs_avx_vpbroadcastd_ymm(b, 5, 5) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[3],
                                               BINARY_GP_RAX) ||
      !wcs_movd_xmm_reg(b, 3, BINARY_GP_RAX) ||
      !wcs_avx_vpbroadcastd_ymm(b, 3, 3) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vmulps_ymm(b, 0, 0, 4) ||
      !wcs_avx_vfmadd231ps_ymm(b, 0, 5, 1) ||
      !wcs_avx_vaddps_ymm(b, 0, 0, 3) ||
      !wcs_avx_vmovups_mem_ymm(b, BINARY_GP_RDX, 0, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movss_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_movss_xmm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !binary_emit_mulss_xmm_xmm(b, BINARY_XMM0, BINARY_XMM4) ||
      !wcs_fmadd231ss(b, 0, 5, 1) ||
      !binary_emit_addss_xmm_xmm(b, BINARY_XMM0, BINARY_XMM3) ||
      !wcs_movss_mem_xmm(b, BINARY_GP_RDX, 0, BINARY_XMM0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  return wcs_patch_here(b, j_done) && wcs_avx_vzeroupper(b);
}

/* Float32 dot product of a[0..n-1]*b[0..n-1], ADDED to dest's prior value.
 * dest = float32 sum, lhs = a, rhs = b, arguments[0] = element count. */
int code_generator_binary_emit_simd_dot_f32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec2 = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 1 || !instruction->arguments) {
    code_generator_set_error(generator, "Malformed simd_dot_f32");
    return 0;
  }
  b = &context->code;

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !binary_emit_movd_xmm_reg(b, BINARY_XMM3, BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R8) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_avx_vpxor_ymm(b, 2, 2, 2) ||
      !wcs_avx_vpxor_ymm(b, 4, 4, 4)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec2) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  /* Two-accumulator FMA unroll: 16 floats/iter into the independent ymm2/ymm4
   * chains via vfmadd231ps. */
  if (!wcs_patch_here(b, j_vec2) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vfmadd231ps_ymm(b, 2, 0, 1) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 32) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 32) ||
      !wcs_avx_vfmadd231ps_ymm(b, 4, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovups_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovups_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vfmadd231ps_ymm(b, 2, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 32) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 32)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !wcs_movss_xmm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_movss_xmm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_fmadd231ss(b, 3, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done) ||
      !wcs_avx_vaddps_ymm(b, 2, 2, 4) ||
      !wcs_reduce_ps_acc_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}
