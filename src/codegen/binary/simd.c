#include "codegen/binary/internal.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- SSE2 / scalar byte encoders local to the word-count vectorizer ----
 * Kept self-contained so the (verified) instruction encodings live next to
 * the algorithm that depends on them. xmm regs used are 0..6 and GPRs are
 * rax/rcx/rdx + r8..r11, so REX.R/B handling is explicit where r8..r15 or the
 * SSE high regs would need it (here they do not, but the helpers stay
 * general). All return 1 on success, 0 on OOM. */

/* 66 0F <op> /r  — SSE2 packed op, xmm dst, xmm src (regs 0..7). */
int wcs_sse_66(BinaryCodeBuffer *b, unsigned char op,
                      int dst, int src) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, op) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* F3 0F 6F /r — movdqu xmm, [rcx]  (mod=00, rm=001=rcx, no disp). */
int wcs_movdqu_xmm_rcx(BinaryCodeBuffer *b, int xmm) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6F) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((xmm & 7) << 3) | 0x01));
}

/* 66 0F 6E /r — movd xmm, r32 (here src is always a low GPR 0..2,8,9). */
int wcs_movd_xmm_reg(BinaryCodeBuffer *b, int xmm, int gpr) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, xmm >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6E) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((xmm & 7) << 3) | (gpr & 7)));
}

/* 66 0F 70 /r ib — pshufd xmm, xmm, imm8. */
int wcs_pshufd(BinaryCodeBuffer *b, int dst, int src,
                      unsigned char imm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x70) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 66 0F D7 /r — pmovmskb r32, xmm. dst is a GPR (0..15), src xmm 0..7. */
int wcs_pmovmskb(BinaryCodeBuffer *b, int gpr, int xmm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, gpr >> 3, 0, xmm >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xD7) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (xmm & 7)));
}

/* F3 0F B8 /r — popcnt r32, r32. */
int wcs_popcnt(BinaryCodeBuffer *b, int dst, int src) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_emit_rex(b, 0, dst >> 3, 0, src >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xB8) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* 0F B6 /r — movzx r32, byte [rcx]. */
int wcs_movzx_reg_byte_rcx(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, 0) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xB6) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((gpr & 7) << 3) | 0x01));
}

/* C1 /4 ib (shl) or /5 ib (shr) — r32, imm8. */
int wcs_shift_reg_imm(BinaryCodeBuffer *b, int gpr, int is_shr,
                             unsigned char imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0xC1) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((is_shr ? 5 : 4) << 3) |
                                (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 09 /r — or r32, r32  (dst |= src). */
int wcs_or_reg_reg(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x09) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* F7 /2 — not r32. */
int wcs_not_reg(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0xF7) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (2 << 3) | (gpr & 7)));
}

/* 23 /r — and r32, r32 (dst &= src). */
int wcs_and_reg_reg(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, dst >> 3, 0, src >> 3) &&
         binary_code_buffer_append_u8(b, 0x23) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* 89 /r — mov r32, r32 (dst = src). */
int wcs_mov_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x89) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* B8+r id — mov r32, imm32. */
int wcs_mov_reg_imm32(BinaryCodeBuffer *b, int gpr, uint32_t imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xB8 + (gpr & 7))) &&
         binary_code_buffer_append_u32(b, imm);
}

/* 48 01 /r — add r64, r64 (dst += src). */
int wcs_add_reg_reg64(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 1, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x01) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* 48 83 /0 ib — add r64, imm8 ; or /5 for sub. */
int wcs_addsub_reg_imm8(BinaryCodeBuffer *b, int gpr, int is_sub,
                               unsigned char imm) {
  return binary_emit_rex(b, 1, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x83) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((is_sub ? 5 : 0) << 3) |
                                (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 48 83 /7 ib — cmp r64, imm8 (sign-extended). */
int wcs_cmp_reg_imm8(BinaryCodeBuffer *b, int gpr, unsigned char imm) {
  return binary_emit_rex(b, 1, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x83) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (7 << 3) | (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 81 /7 id — cmp r32, imm32 (used for the tail byte compares). */
int wcs_cmp_reg_imm32(BinaryCodeBuffer *b, int gpr, uint32_t imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x81) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (7 << 3) | (gpr & 7))) &&
         binary_code_buffer_append_u32(b, imm);
}

/* 85 /r — test r32, r32. */
int wcs_test_reg_reg32(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x85) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (gpr & 7)));
}

/* 31 /r — xor r32, r32 (zero a reg via self-xor). */
int wcs_xor_self32(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x31) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (gpr & 7)));
}

/* Emit a near jcc/jmp with a 32-bit rel placeholder; record the offset of the
 * displacement field so it can be patched once the target is known. cc==0
 * means an unconditional jmp (E9), otherwise 0F 8x. */
int wcs_jcc(BinaryCodeBuffer *b, unsigned char cc, size_t *disp_off) {
  if (cc == 0) {
    if (!binary_code_buffer_append_u8(b, 0xE9)) return 0;
  } else {
    if (!binary_code_buffer_append_u8(b, 0x0F) ||
        !binary_code_buffer_append_u8(b, cc))
      return 0;
  }
  *disp_off = b->size;
  return binary_code_buffer_append_u32(b, 0);
}

/* Patch a rel32 placeholder so it jumps to the current end of the buffer. */
int wcs_patch_here(BinaryCodeBuffer *b, size_t disp_off) {
  long long delta =
      (long long)b->size - (long long)(disp_off + 4);
  if (delta < INT32_MIN || delta > INT32_MAX) return 0;
  int32_t d = (int32_t)delta;
  memcpy(b->data + disp_off, &d, 4);
  return 1;
}

/* Patch a rel32 placeholder to jump backward to a recorded target offset. */
int wcs_patch_to(BinaryCodeBuffer *b, size_t disp_off,
                        size_t target) {
  long long delta = (long long)target - (long long)(disp_off + 4);
  if (delta < INT32_MIN || delta > INT32_MAX) return 0;
  int32_t d = (int32_t)delta;
  memcpy(b->data + disp_off, &d, 4);
  return 1;
}

/* 39 /r — cmp r32, r32 */
int wcs_cmp_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x39) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* movdqu xmm, [gpr+0] */
int wcs_movdqu_xmm_mem(BinaryCodeBuffer *b, int xmm, int gpr) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6F) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((xmm & 7) << 3) | (gpr & 7)));
}

int simd_emit_prefixed_xmm_mem_disp(BinaryCodeBuffer *b, unsigned char prefix,
                                    unsigned char opcode, int xmm, int gpr,
                                    int displacement) {
  if (!b) {
    return 0;
  }

  int use_disp8 = displacement >= -128 && displacement <= 127;
  unsigned char rm = (unsigned char)(gpr & 7);
  int needs_sib = rm == (BINARY_GP_RSP & 7);
  int needs_base_disp = rm == (BINARY_GP_RBP & 7);
  unsigned char mod =
      (displacement == 0 && !needs_base_disp) ? 0 : (use_disp8 ? 1 : 2);
  unsigned char modrm =
      (unsigned char)((mod << 6) | ((xmm & 7) << 3) |
                      (needs_sib ? 4 : rm));

  if (!binary_code_buffer_append_u8(b, prefix) ||
      !binary_emit_rex(b, 0, xmm >> 3, 0, gpr >> 3) ||
      !binary_code_buffer_append_u8(b, 0x0F) ||
      !binary_code_buffer_append_u8(b, opcode) ||
      !binary_code_buffer_append_u8(b, modrm)) {
    return 0;
  }
  if (needs_sib) {
    unsigned char sib = (unsigned char)((0 << 6) | (4 << 3) | (gpr & 7));
    if (!binary_code_buffer_append_u8(b, sib)) {
      return 0;
    }
  }
  if (mod == 1) {
    return binary_code_buffer_append_u8(b, (unsigned char)(int8_t)displacement);
  }
  if (mod == 2) {
    return binary_code_buffer_append_u32(b, (uint32_t)(int32_t)displacement);
  }
  return 1;
}

int simd_emit_xmm_mem_disp(BinaryCodeBuffer *b, unsigned char opcode,
                                  int xmm, int gpr, int displacement) {
  return simd_emit_prefixed_xmm_mem_disp(b, 0xF3, opcode, xmm, gpr,
                                         displacement);
}

int simd_movdqu_xmm_mem_disp(BinaryCodeBuffer *b, int xmm, int gpr,
                                    int displacement) {
  return simd_emit_xmm_mem_disp(b, 0x6F, xmm, gpr, displacement);
}

int simd_movdqu_mem_xmm_disp(BinaryCodeBuffer *b, int gpr,
                                    int displacement, int xmm) {
  return simd_emit_xmm_mem_disp(b, 0x7F, xmm, gpr, displacement);
}

int simd_movd_xmm_mem32_disp(BinaryCodeBuffer *b, int xmm, int gpr,
                             int displacement) {
  return simd_emit_prefixed_xmm_mem_disp(b, 0x66, 0x6E, xmm, gpr,
                                         displacement);
}

/* 66 0F 7E /r - movd r32, xmm */
int wcs_movd_reg_xmm(BinaryCodeBuffer *b, int gpr, int xmm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, xmm >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x7E) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((xmm & 7) << 3) | (gpr & 7)));
}

/* 29 /r — sub r32, r32 (dst -= src) */
int wcs_sub_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x29) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* Lower IR_OP_SIMD_SUM_I32: add sum of base[0..len-1] int32s into dest. */
int code_generator_binary_emit_simd_sum_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  /* rax=sum, rcx=base, edx=i, r8d=len */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R8)) {
    return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_RDX)) return 0;

  loop_top = b->size;
  if (!wcs_cmp_reg_reg32(b, BINARY_GP_RDX, BINARY_GP_R8)) return 0;
  if (!wcs_jcc(b, 0x83 /* jae */, &j_done)) return 0;

  /* len - i >= 4 ? */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_sub_reg_reg32(b, BINARY_GP_R9, BINARY_GP_RDX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 4) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec)) {
    return 0;
  }
  if (!wcs_jcc(b, 0, &j_scalar)) return 0;

  if (!wcs_patch_here(b, j_vec)) return 0;
  if (!wcs_movdqu_xmm_mem(b, 0, BINARY_GP_RCX)) return 0;
  for (int lane = 0; lane < 4; lane++) {
    if (lane > 0 && !wcs_pshufd(b, 0, 0, (unsigned char)lane)) {
      return 0;
    }
    if (!wcs_movd_reg_xmm(b, BINARY_GP_R10, 0) ||
        !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
        !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10)) {
      return 0;
    }
  }
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 16) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar)) return 0;
  if (!binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 1)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done)) return 0;

  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* 66 0F F4 /r — pmuludq xmm, xmm */
/* In-place int32 insertion sort with SSE2 chunk scan and 16-byte shifts. */
int code_generator_binary_emit_simd_insertion_sort_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t outer_loop = 0;
  size_t inner_loop = 0;
  size_t j_insert_from_bound = 0;
  size_t j_insert_from_le = 0;
  size_t j_done = 0;

  if (!generator || !context || !instruction ||
      instruction->dest.kind == IR_OPERAND_NONE ||
      instruction->rhs.kind == IR_OPERAND_NONE) {
    code_generator_set_error(generator, "Malformed simd_insertion_sort_i32");
    return 0;
  }
  b = &context->code;

  /* rcx=base, r8d=len, edx=i, r9d=key, r10=scan_ptr, r11d=current */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R8) ||
      !wcs_xor_self32(b, BINARY_GP_RDX) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 1)) {
    return 0;
  }

  outer_loop = b->size;
  if (!wcs_cmp_reg_reg32(b, BINARY_GP_RDX, BINARY_GP_R8) ||
      !wcs_jcc(b, 0x8D /* jge */, &j_done)) {
    return 0;
  }

  if (!binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_RCX, BINARY_GP_RDX, 4, 0) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R9, BINARY_GP_RAX, 0) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R10, BINARY_GP_RAX) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R10, 1, 4)) {
    return 0;
  }

  inner_loop = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_R10, BINARY_GP_RCX) ||
      !wcs_jcc(b, 0x82 /* jb */, &j_insert_from_bound) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R11, BINARY_GP_R10, 0) ||
      !wcs_cmp_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x8E /* jle */, &j_insert_from_le) ||
      !binary_emit_mov_mem_reg32(b, BINARY_GP_R10, 4, BINARY_GP_R11) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R10, 1, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, inner_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_insert_from_bound) ||
      !wcs_patch_here(b, j_insert_from_le) ||
      !binary_emit_mov_mem_reg32(b, BINARY_GP_R10, 4, BINARY_GP_R9) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 1)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, outer_loop)) {
      return 0;
    }
  }

  return wcs_patch_here(b, j_done);
}

int code_generator_binary_emit_lower_bound_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_back = 0;

  if (!generator || !context || !instruction || instruction->argument_count < 1) {
    return 0;
  }
  b = &context->code;

  /* r10=lo, r11=hi, rsi=arr, r9d=key, rax=mid, rcx=arr[mid] */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R11) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R9) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RSI)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_R10, BINARY_GP_R11) ||
      !wcs_jcc(b, 0x8D /* jge */, &j_done) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_RAX, BINARY_GP_R11) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_RAX, BINARY_GP_R10) ||
      !binary_emit_shift_reg_imm8(b, 7, BINARY_GP_RAX, 1) ||
      !binary_emit_alu_reg_reg(b, 0x01, BINARY_GP_RAX, BINARY_GP_R10) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RCX, BINARY_GP_RSI, BINARY_GP_RAX, 4, 0) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_RCX, BINARY_GP_RCX, 0) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_RDX, BINARY_GP_RAX) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R8, BINARY_GP_RAX) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R8, 0, 1) ||
      !wcs_cmp_reg_reg32(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !binary_emit_cmovcc_reg_reg(b, 0x4C /* cmovl */, BINARY_GP_R10,
                                  BINARY_GP_R8) ||
      !binary_emit_cmovcc_reg_reg(b, 0x4D /* cmovge */, BINARY_GP_R11,
                                  BINARY_GP_RDX) ||
      !wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
    return 0;
  }
  j_back = 0;
  if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top) ||
      !wcs_patch_here(b, j_done) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_R10)) {
    return 0;
  }
  return 1;
}

int wcs_pmuludq(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66(b, 0xF4, dst, src);
}

/* 66 0F FE /r — paddd xmm, xmm */
int wcs_paddd(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66(b, 0xFE, dst, src);
}

/* 66 0F 38 xx /r — SSE4.1 packed xmm ops. */
int wcs_sse_66_38(BinaryCodeBuffer *b, unsigned char op, int dst,
                         int src) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x38) &&
         binary_code_buffer_append_u8(b, op) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

int wcs_pmulld(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66_38(b, 0x40, dst, src);
}

/* 66 0F 38 39 /r — pminsd xmm, xmm (SSE4.1) */
int wcs_pminsd(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66_38(b, 0x39, dst, src);
}

/* 66 0F 38 3D /r — pmaxsd xmm, xmm (SSE4.1) */
int wcs_pmaxsd(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66_38(b, 0x3D, dst, src);
}

int wcs_broadcast_i32_to_xmm(BinaryCodeBuffer *b, int xmm, int gpr) {
  return wcs_movd_xmm_reg(b, xmm, gpr) && wcs_pshufd(b, xmm, xmm, 0x00);
}

int wcs_accumulate_xmm0_i32_to_rax(BinaryCodeBuffer *b) {
  if (!wcs_sse_66(b, 0x6F, 1, 0) || !wcs_pshufd(b, 1, 0, 0xEE) ||
      !wcs_paddd(b, 0, 1) || !wcs_pshufd(b, 1, 0, 0x01) ||
      !wcs_paddd(b, 0, 1) || !wcs_movd_reg_xmm(b, BINARY_GP_R10, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10)) {
    return 0;
  }
  return 1;
}

int wcs_fold_xmm6_i32_sum_to_rax(BinaryCodeBuffer *b) {
  if (!wcs_sse_66(b, 0x6F, 0, 6) || !wcs_accumulate_xmm0_i32_to_rax(b)) {
    return 0;
  }
  return 1;
}

int code_generator_binary_emit_simd_scale_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;
  int32_t mul_imm = 0;
  int32_t add_imm = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 3) {
    return 0;
  }
  b = &context->code;
  mul_imm = (int32_t)instruction->arguments[1].int_value;
  add_imm = (int32_t)instruction->arguments[2].int_value;

  /* rax=sum, rcx=src, rdx=dst, r8=src_end, xmm4/xmm5=mul/add */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R11) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_R11) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R8, BINARY_GP_RCX) ||
      !wcs_mov_reg_imm32(b, BINARY_GP_R9, (uint32_t)mul_imm) ||
      !wcs_broadcast_i32_to_xmm(b, 4, BINARY_GP_R9) ||
      !wcs_mov_reg_imm32(b, BINARY_GP_R9, (uint32_t)add_imm) ||
      !wcs_broadcast_i32_to_xmm(b, 5, BINARY_GP_R9) ||
      !wcs_sse_66(b, 0xEF, 6, 6)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R8) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_pmulld(b, 0, 4) ||
      !wcs_paddd(b, 0, 5) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 0, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 16) ||
      !wcs_pmulld(b, 0, 4) ||
      !wcs_paddd(b, 0, 5) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 16, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 32) ||
      !wcs_pmulld(b, 0, 4) ||
      !wcs_paddd(b, 0, 5) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 32, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 48) ||
      !wcs_pmulld(b, 0, 4) ||
      !wcs_paddd(b, 0, 5) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 48, 0) ||
      !wcs_paddd(b, 6, 0) ||
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

  if (!wcs_patch_here(b, j_scalar) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0) ||
      !binary_emit_imul_reg_reg_imm32(b, BINARY_GP_R10, BINARY_GP_R10,
                                      (uint32_t)mul_imm) ||
      !binary_emit_add_reg_imm32(b, BINARY_GP_R10, (uint32_t)add_imm) ||
      !binary_emit_mov_mem_reg32(b, BINARY_GP_RDX, 0, BINARY_GP_R10) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
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
      !wcs_fold_xmm6_i32_sum_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

int code_generator_binary_emit_simd_reverse_copy_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 1) {
    return 0;
  }
  b = &context->code;

  /* rax=sum, rcx=src (last elem), rdx=dst walk, r8=dst_end, r10=src_base */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_R10, BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R11) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R11) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R9, 1, 1) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_R11) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R8, BINARY_GP_RDX)) {
    return 0;
  }
  if (!wcs_sse_66(b, 0xEF, 6, 6)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RDX, BINARY_GP_R8) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_RDX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 64) ||
      !wcs_jcc(b, 0x82 /* jb */, &j_scalar) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_RCX) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_R10) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 60) ||
      !wcs_jcc(b, 0x82 /* jb */, &j_scalar) ||
      !wcs_jcc(b, 0, &j_vec)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, -12) ||
      !wcs_pshufd(b, 0, 0, 0x1B) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 0, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, -28) ||
      !wcs_pshufd(b, 0, 0, 0x1B) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 16, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, -44) ||
      !wcs_pshufd(b, 0, 0, 0x1B) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 32, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, -60) ||
      !wcs_pshufd(b, 0, 0, 0x1B) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 48, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 1, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R9, BINARY_GP_RCX, 0) ||
      !binary_emit_mov_mem_reg32(b, BINARY_GP_RDX, 0, BINARY_GP_R9) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R9) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R9) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 1, 4) ||
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
      !wcs_fold_xmm6_i32_sum_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

int code_generator_binary_emit_simd_clamp_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;
  int32_t lo = 0;
  int32_t hi = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 3) {
    return 0;
  }
  b = &context->code;
  lo = (int32_t)instruction->arguments[1].int_value;
  hi = (int32_t)instruction->arguments[2].int_value;
  if (lo > hi) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R11) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_R11) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R8, BINARY_GP_RCX) ||
      !wcs_mov_reg_imm32(b, BINARY_GP_R9, (uint32_t)lo) ||
      !wcs_broadcast_i32_to_xmm(b, 4, BINARY_GP_R9) ||
      !wcs_mov_reg_imm32(b, BINARY_GP_R9, (uint32_t)hi) ||
      !wcs_broadcast_i32_to_xmm(b, 5, BINARY_GP_R9) ||
      !wcs_sse_66(b, 0xEF, 6, 6)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R8) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 16) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_movdqu_xmm_mem(b, 0, BINARY_GP_RCX) ||
      !wcs_pmaxsd(b, 0, 4) ||
      !wcs_pminsd(b, 0, 5) ||
      !simd_movdqu_mem_xmm_disp(b, BINARY_GP_RDX, 0, 0) ||
      !wcs_paddd(b, 6, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 16) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 16)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0)) {
    return 0;
  }
  {
    size_t j_not_lo = 0;
    size_t j_not_hi = 0;
    size_t j_clamp_done = 0;
    if (!wcs_cmp_reg_imm32(b, BINARY_GP_R10, lo) ||
        !wcs_jcc(b, 0x8D /* jge */, &j_not_lo) ||
        !wcs_mov_reg_imm32(b, BINARY_GP_R10, (uint32_t)lo) ||
        !wcs_jcc(b, 0, &j_clamp_done)) {
      return 0;
    }
    if (!wcs_patch_here(b, j_not_lo) ||
        !wcs_cmp_reg_imm32(b, BINARY_GP_R10, hi) ||
        !wcs_jcc(b, 0x8E /* jle */, &j_not_hi) ||
        !wcs_mov_reg_imm32(b, BINARY_GP_R10, (uint32_t)hi)) {
      return 0;
    }
    if (!wcs_patch_here(b, j_not_hi) ||
        !wcs_patch_here(b, j_clamp_done) ||
        !binary_emit_mov_mem_reg32(b, BINARY_GP_RDX, 0, BINARY_GP_R10) ||
        !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
        !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
        !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
        !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 4)) {
      return 0;
    }
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done) ||
      !wcs_fold_xmm6_i32_sum_to_rax(b)) {
    return 0;
  }
  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* 66 0F 73 /2 ib — psrlq xmm, imm8 */
int wcs_psrlq_imm(BinaryCodeBuffer *b, int xmm, unsigned char imm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x73) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (2 << 3) | (xmm & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* Fixed 32x32 int32 matrix multiply — SSE2 4-column kernel, N=32. */
/* 66 0F 73 /3 ib - psrldq xmm, imm8 */
int wcs_psrldq_imm(BinaryCodeBuffer *b, int xmm, unsigned char imm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x73) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (3 << 3) | (xmm & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 66 0F D4 /r - paddq xmm, xmm */
int wcs_paddq(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66(b, 0xD4, dst, src);
}

/* 66 0F 38 28 /r - pmuldq xmm, xmm (signed dword -> qword). */
int wcs_pmuldq(BinaryCodeBuffer *b, int dst, int src) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, dst >> 3, 0, src >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x38) &&
         binary_code_buffer_append_u8(b, 0x28) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

static int wcs_vex3(BinaryCodeBuffer *b, int map, int pp, int len256, int w,
                    int reg, int rm, int vvvv) {
  unsigned char b2 = (unsigned char)((((~(reg >> 3)) & 1) << 7) |
                                     (1 << 6) |
                                     (((~(rm >> 3)) & 1) << 5) |
                                     (map & 0x1F));
  unsigned char b3 = (unsigned char)(((w & 1) << 7) |
                                     (((~vvvv) & 0x0F) << 3) |
                                     ((len256 & 1) << 2) | (pp & 3));
  return binary_code_buffer_append_u8(b, 0xC4) &&
         binary_code_buffer_append_u8(b, b2) &&
         binary_code_buffer_append_u8(b, b3);
}

static int wcs_avx_modrm_mem_disp(BinaryCodeBuffer *b, int reg, int base,
                                  int displacement) {
  int use_disp8 = displacement >= -128 && displacement <= 127;
  unsigned char base_low = (unsigned char)(base & 7);
  unsigned char mod = 0;
  unsigned char rm = base_low;
  if (displacement != 0 || base_low == 5) {
    mod = use_disp8 ? 1 : 2;
  }
  if (base_low == 4) {
    rm = 4;
  }
  if (!binary_code_buffer_append_u8(
          b, (unsigned char)((mod << 6) | ((reg & 7) << 3) | rm))) {
    return 0;
  }
  if (base_low == 4 &&
      !binary_code_buffer_append_u8(
          b, (unsigned char)((0 << 6) | (4 << 3) | base_low))) {
    return 0;
  }
  if (mod == 1) {
    return binary_code_buffer_append_u8(b,
                                        (unsigned char)(int8_t)displacement);
  }
  if (mod == 2 || (mod == 0 && base_low == 5)) {
    return binary_code_buffer_append_u32(b, (uint32_t)(int32_t)displacement);
  }
  return 1;
}

static int wcs_avx_vmovdqu_ymm_mem(BinaryCodeBuffer *b, int dst, int base,
                                   int displacement) {
  return wcs_vex3(b, 1, 2, 1, 0, dst, base, 0) &&
         binary_code_buffer_append_u8(b, 0x6F) &&
         wcs_avx_modrm_mem_disp(b, dst, base, displacement);
}

static int wcs_avx_vpaddq_ymm(BinaryCodeBuffer *b, int dst, int src1,
                              int src2) {
  return wcs_vex3(b, 1, 1, 1, 0, dst, src2, src1) &&
         binary_code_buffer_append_u8(b, 0xD4) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src2 & 7)));
}

static int wcs_avx_vpxor_ymm(BinaryCodeBuffer *b, int dst, int src1,
                             int src2) {
  return wcs_vex3(b, 1, 1, 1, 0, dst, src2, src1) &&
         binary_code_buffer_append_u8(b, 0xEF) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src2 & 7)));
}

static int wcs_avx_vpmuldq_ymm(BinaryCodeBuffer *b, int dst, int src1,
                               int src2) {
  return wcs_vex3(b, 2, 1, 1, 0, dst, src2, src1) &&
         binary_code_buffer_append_u8(b, 0x28) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src2 & 7)));
}

static int wcs_avx_vpsrlq_ymm_imm(BinaryCodeBuffer *b, int dst, int src,
                                  unsigned char imm) {
  return wcs_vex3(b, 1, 1, 1, 0, 2, dst, src) &&
         binary_code_buffer_append_u8(b, 0x73) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (2 << 3) | (dst & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

static int wcs_avx_vextracti128(BinaryCodeBuffer *b, int dst_xmm, int src_ymm,
                                unsigned char lane) {
  return wcs_vex3(b, 3, 1, 1, 0, src_ymm, dst_xmm, 0) &&
         binary_code_buffer_append_u8(b, 0x39) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src_ymm & 7) << 3) |
                                (dst_xmm & 7))) &&
         binary_code_buffer_append_u8(b, lane);
}

static int wcs_avx_vzeroupper(BinaryCodeBuffer *b) {
  return binary_code_buffer_append_u8(b, 0xC5) &&
         binary_code_buffer_append_u8(b, 0xF8) &&
         binary_code_buffer_append_u8(b, 0x77);
}

int code_generator_binary_emit_simd_dot_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count != 1 || !instruction->arguments) {
    code_generator_set_error(generator, "Malformed simd_dot_i32");
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
      !binary_emit_mov_reg_imm64(b, BINARY_GP_RAX, 0) ||
      !wcs_avx_vpxor_ymm(b, 2, 2, 2) ||
      !wcs_avx_vpxor_ymm(b, 5, 5, 5) ||
      !binary_emit_mov_reg_reg(b, BINARY_GP_R11, BINARY_GP_R8) ||
      !binary_emit_shift_reg_imm8(b, 4, BINARY_GP_R11, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R11, BINARY_GP_RCX)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R11) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_R11) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 128) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 1, BINARY_GP_RDX, 0) ||
      !wcs_avx_vpmuldq_ymm(b, 3, 0, 1) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 0, 0, 32) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 1, 1, 32) ||
      !wcs_avx_vpmuldq_ymm(b, 4, 0, 1) ||
      !wcs_avx_vpaddq_ymm(b, 2, 2, 3) ||
      !wcs_avx_vpaddq_ymm(b, 5, 5, 4) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 0, BINARY_GP_RCX, 32) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 1, BINARY_GP_RDX, 32) ||
      !wcs_avx_vpmuldq_ymm(b, 3, 0, 1) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 0, 0, 32) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 1, 1, 32) ||
      !wcs_avx_vpmuldq_ymm(b, 4, 0, 1) ||
      !wcs_avx_vpaddq_ymm(b, 2, 2, 3) ||
      !wcs_avx_vpaddq_ymm(b, 5, 5, 4) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 0, BINARY_GP_RCX, 64) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 1, BINARY_GP_RDX, 64) ||
      !wcs_avx_vpmuldq_ymm(b, 3, 0, 1) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 0, 0, 32) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 1, 1, 32) ||
      !wcs_avx_vpmuldq_ymm(b, 4, 0, 1) ||
      !wcs_avx_vpaddq_ymm(b, 2, 2, 3) ||
      !wcs_avx_vpaddq_ymm(b, 5, 5, 4) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 0, BINARY_GP_RCX, 96) ||
      !wcs_avx_vmovdqu_ymm_mem(b, 1, BINARY_GP_RDX, 96) ||
      !wcs_avx_vpmuldq_ymm(b, 3, 0, 1) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 0, 0, 32) ||
      !wcs_avx_vpsrlq_ymm_imm(b, 1, 1, 32) ||
      !wcs_avx_vpmuldq_ymm(b, 4, 0, 1) ||
      !wcs_avx_vpaddq_ymm(b, 2, 2, 3) ||
      !wcs_avx_vpaddq_ymm(b, 5, 5, 4) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 64) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R9, BINARY_GP_RDX, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R9) ||
      !binary_emit_imul_reg_reg(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
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
      !wcs_avx_vpaddq_ymm(b, 2, 2, 5) ||
      !wcs_avx_vextracti128(b, 3, 2, 1) ||
      !wcs_avx_vzeroupper(b) ||
      !wcs_paddq(b, 2, 3) ||
      !binary_emit_movq_reg_xmm(b, BINARY_GP_R10, BINARY_XMM2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
      !wcs_pshufd(b, 3, 2, 0xEE) ||
      !binary_emit_movq_reg_xmm(b, BINARY_GP_R10, BINARY_XMM3) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10)) {
    return 0;
  }

  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

int code_generator_binary_emit_simd_matmul_n32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t row_loop = 0;
  size_t row_done = 0;
  size_t col_loop = 0;
  size_t col_done = 0;
  size_t k_loop = 0;
  size_t k_done = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  /* This kernel uses r12-r15 and rbx as scratch; all are callee-saved under
   * Win64. Because the matmul op is frequently inlined into its caller (where
   * those registers may hold live promoted values), the kernel is made
   * self-contained: save them on entry and restore on every exit path. No
   * call/aligned-stack-spill happens between, so the odd push count is fine. */
  /* Volatile-only leaf kernel: rcx=a, rdx=b, r8=c, r9d=row, r10d=col,
   * r11d=k, rax=address/index. */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_R8)) {
    return 0;
  }

  if (!wcs_xor_self32(b, BINARY_GP_R9)) return 0; /* row */

  row_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_R9, 32) ||
      !wcs_jcc(b, 0x83, &row_done)) return 0;

  if (!wcs_xor_self32(b, BINARY_GP_R10)) return 0; /* col */

  col_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_R10, 32) ||
      !wcs_jcc(b, 0x83, &col_done)) return 0;

  if (!wcs_sse_66(b, 0xEF, 3, 3)) return 0; /* pxor xmm3, xmm3 */

  if (!wcs_xor_self32(b, BINARY_GP_R11)) return 0; /* k */

  k_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_R11, 32) ||
      !wcs_jcc(b, 0x83, &k_done)) return 0;

  /* av = a[r12 + r9*4] — r11 holds row_base until clobbered for av load */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_RAX, BINARY_GP_R9) ||
      !wcs_shift_reg_imm(b, BINARY_GP_RAX, 0, 5) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R11) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_RCX, BINARY_GP_RAX, 4, 0) ||
      !simd_movd_xmm_mem32_disp(b, 4, BINARY_GP_RAX, 0) ||
      !wcs_pshufd(b, 4, 4, 0x00)) {
    return 0;
  }

  /* b row at r13 + r10*4, 16 bytes */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_RAX, BINARY_GP_R11) ||
      !wcs_shift_reg_imm(b, BINARY_GP_RAX, 0, 5) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_RDX, BINARY_GP_RAX, 4, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 2, BINARY_GP_RAX, 0)) {
    return 0;
  }

  /* xmm2 = b[k][col..col+3] (4 packed int32); xmm4 = a[r][k] broadcast.
   * Lane-wise int32 multiply (low 32 bits = int32 wraparound semantics) and
   * accumulate into xmm3. pmulld keeps a clean 4-wide dword lane mapping; the
   * earlier pmuludq pair produced 64-bit products that paddd then folded into
   * the wrong columns. */
  if (!wcs_sse_66(b, 0x6F, 0, 2) || /* movdqa xmm0, xmm2 */
      !wcs_pmulld(b, 0, 4) ||
      !wcs_paddd(b, 3, 0)) return 0;

  if (!wcs_addsub_reg_imm8(b, BINARY_GP_R11, 0, 1)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, k_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, k_done)) return 0;

  /* store xmm3 lanes to c[row*32+col..+3] */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_RAX, BINARY_GP_R9) ||
      !wcs_shift_reg_imm(b, BINARY_GP_RAX, 0, 5) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_R8, BINARY_GP_RAX, 4, 0)) {
    return 0;
  }
  /* Store the 4 accumulated lanes of xmm3 to c[row*32+col..+3]. Extract each
   * lane by shuffling it to dword0 of a scratch register (xmm0) read FRESH from
   * xmm3 every iteration — shuffling xmm3 in place would clobber the lanes not
   * yet stored (pshufd writes all four dwords). */
  for (int lane = 0; lane < 4; lane++) {
    int src_xmm = 3;
    if (lane > 0) {
      if (!wcs_pshufd(b, 0, 3, (unsigned char)lane)) return 0;
      src_xmm = 0;
    }
    if (!wcs_movd_reg_xmm(b, BINARY_GP_R11, src_xmm) ||
        !binary_emit_mov_mem_reg32(b, BINARY_GP_RAX, (int)(lane * 4),
                                   BINARY_GP_R11)) {
      return 0;
    }
  }

  if (!wcs_addsub_reg_imm8(b, BINARY_GP_R10, 0, 4)) return 0;
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, col_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, col_done)) return 0;
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_R9, 0, 1)) return 0;
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, row_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, row_done)) return 0;

  return 1;
}

/* Lower IR_OP_COUNT_WORD_STARTS: count maximal non-whitespace runs in
 * buf[0..len-1] (whitespace = 0x20/0x09/0x0A/0x0D), 16 bytes/iter via SSE2,
 * plus a scalar tail. Result is ADDED to count's prior value (the recognizer
 * only fires when the source set count=0 before the loop). Same algorithm as
 * the text backend's code_generator_emit_ir_count_word_starts. */
int code_generator_binary_emit_count_word_starts(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  if (!generator || !context || !instruction ||
      instruction->dest.kind != IR_OPERAND_SYMBOL ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL ||
      instruction->rhs.kind != IR_OPERAND_SYMBOL) {
    code_generator_set_error(generator, "Malformed count_word_starts in '%s'",
                             context ? context->function_name : "?");
    return 0;
  }

  BinaryCodeBuffer *b = &context->code;

  /* rcx <- buf ; rdx <- len ; rax <- count ; r8d <- 0 (carry). */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX)) {
    return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_R8)) return 0;

  /* Broadcast 0x20/0x09/0x0A/0x0D into xmm1..xmm4 (via r9d + movd + pshufd). */
  static const struct { unsigned int pat; int xmm; } CONSTS[4] = {
      {0x20202020u, 1}, {0x09090909u, 2}, {0x0A0A0A0Au, 3}, {0x0D0D0D0Du, 4}};
  for (int i = 0; i < 4; i++) {
    if (!wcs_mov_reg_imm32(b, BINARY_GP_R9, CONSTS[i].pat) ||
        !wcs_movd_xmm_reg(b, CONSTS[i].xmm, BINARY_GP_R9) ||
        !wcs_pshufd(b, CONSTS[i].xmm, CONSTS[i].xmm, 0x00)) {
      return 0;
    }
  }

  /* ---- vector loop: while (rdx >= 16) ---- */
  size_t loop_top = b->size;
  /* cmp rdx, 16 ; jb tail */
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 16)) return 0;
  size_t j_to_tail;
  if (!wcs_jcc(b, 0x82 /* jb */, &j_to_tail)) return 0;

  /* xmm0 = chunk ; xmm5 = copy ; xmm0 = (==sp) | (==tab) | (==lf) | (==cr) */
  if (!wcs_movdqu_xmm_rcx(b, 0) ||
      !wcs_sse_66(b, 0x6F, 5, 0) ||           /* movdqa xmm5, xmm0 */
      !wcs_sse_66(b, 0x74, 0, 1) ||           /* pcmpeqb xmm0, xmm1 */
      !wcs_sse_66(b, 0x6F, 6, 5) ||           /* movdqa xmm6, xmm5 */
      !wcs_sse_66(b, 0x74, 6, 2) ||           /* pcmpeqb xmm6, xmm2 */
      !wcs_sse_66(b, 0xEB, 0, 6) ||           /* por xmm0, xmm6 */
      !wcs_sse_66(b, 0x6F, 6, 5) ||
      !wcs_sse_66(b, 0x74, 6, 3) ||           /* pcmpeqb xmm6, xmm3 */
      !wcs_sse_66(b, 0xEB, 0, 6) ||
      !wcs_sse_66(b, 0x6F, 6, 5) ||
      !wcs_sse_66(b, 0x74, 6, 4) ||           /* pcmpeqb xmm6, xmm4 */
      !wcs_sse_66(b, 0xEB, 0, 6)) {
    return 0;
  }
  /* r9d = ws bitmask ; r10d = nw = ~ws & 0xFFFF */
  if (!wcs_pmovmskb(b, BINARY_GP_R9, 0) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !wcs_not_reg(b, BINARY_GP_R10) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R10, 0xFFFF)) {
    return 0;
  }
  /* r11d = prev = (nw<<1) | carry */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R10) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R11, 0, 1) ||
      !wcs_or_reg_reg(b, BINARY_GP_R11, BINARY_GP_R8)) {
    return 0;
  }
  /* new carry r8d = nw bit15 */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_R10) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 1, 15) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R8, 1)) {
    return 0;
  }
  /* starts = nw & ~prev ; count += popcount(starts) */
  if (!wcs_not_reg(b, BINARY_GP_R11) ||
      !wcs_and_reg_reg(b, BINARY_GP_R11, BINARY_GP_R10) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R11, 0xFFFF) ||
      !wcs_popcnt(b, BINARY_GP_R11, BINARY_GP_R11) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R11)) {
    return 0;
  }
  /* rcx += 16 ; rdx -= 16 ; jmp loop_top */
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 16) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 1, 16)) {
    return 0;
  }
  size_t j_back;
  if (!wcs_jcc(b, 0, &j_back)) return 0;
  if (!wcs_patch_to(b, j_back, loop_top)) {
    code_generator_set_error(generator, "wcs back-jump out of range");
    return 0;
  }

  /* ---- scalar tail ---- */
  if (!wcs_patch_here(b, j_to_tail)) return 0; /* jb -> here */
  /* if (rdx == 0) goto done */
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 0)) return 0;
  size_t j_done_early;
  if (!wcs_jcc(b, 0x84 /* je */, &j_done_early)) return 0;

  size_t tail_top = b->size;
  /* r9d = (uint8)[rcx] */
  if (!wcs_movzx_reg_byte_rcx(b, BINARY_GP_R9)) return 0;
  /* four "is whitespace?" tests -> collect je placeholders */
  size_t j_ws[4];
  static const unsigned int WS[4] = {32u, 9u, 10u, 13u};
  for (int i = 0; i < 4; i++) {
    if (!wcs_cmp_reg_imm32(b, BINARY_GP_R9, WS[i]) ||
        !wcs_jcc(b, 0x84 /* je */, &j_ws[i])) {
      return 0;
    }
  }
  /* non-whitespace: if (carry==0) count++ ; carry=1 */
  if (!wcs_test_reg_reg32(b, BINARY_GP_R8)) return 0;
  size_t j_skip_inc;
  if (!wcs_jcc(b, 0x85 /* jne */, &j_skip_inc)) return 0;
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RAX, 0, 1)) return 0; /* rax += 1 */
  if (!wcs_patch_here(b, j_skip_inc)) return 0;
  if (!wcs_mov_reg_imm32(b, BINARY_GP_R8, 1)) return 0; /* carry = 1 */
  size_t j_after_class;
  if (!wcs_jcc(b, 0, &j_after_class)) return 0;
  /* whitespace target: carry = 0 */
  for (int i = 0; i < 4; i++) {
    if (!wcs_patch_here(b, j_ws[i])) return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_R8)) return 0; /* carry = 0 */
  if (!wcs_patch_here(b, j_after_class)) return 0;
  /* rcx++ ; rdx-- ; if (rdx != 0) goto tail_top */
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 1, 1)) {
    return 0;
  }
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 0)) return 0;
  size_t j_tail_back;
  if (!wcs_jcc(b, 0x85 /* jne */, &j_tail_back)) return 0;
  if (!wcs_patch_to(b, j_tail_back, tail_top)) {
    code_generator_set_error(generator, "wcs tail-jump out of range");
    return 0;
  }

  if (!wcs_patch_here(b, j_done_early)) return 0;

  /* count = rax */
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  return 1;
}

static int wcs_horizontal_pminsd_to_reg(BinaryCodeBuffer *b, int xmm, int gpr) {
  return wcs_pshufd(b, 1, xmm, 0xEE) &&
         wcs_pminsd(b, xmm, 1) &&
         wcs_pshufd(b, 1, xmm, 0x01) &&
         wcs_pminsd(b, xmm, 1) &&
         wcs_movd_reg_xmm(b, BINARY_GP_R9, xmm) &&
         binary_emit_cmp_reg_reg(b, BINARY_GP_R9, gpr) &&
         binary_emit_cmovcc_reg_reg(b, 0x4C /* cmovl */, gpr, BINARY_GP_R9);
}

static int wcs_horizontal_pmaxsd_to_reg(BinaryCodeBuffer *b, int xmm, int gpr) {
  return wcs_pshufd(b, 1, xmm, 0xEE) &&
         wcs_pmaxsd(b, xmm, 1) &&
         wcs_pshufd(b, 1, xmm, 0x01) &&
         wcs_pmaxsd(b, xmm, 1) &&
         wcs_movd_reg_xmm(b, BINARY_GP_R9, xmm) &&
         binary_emit_cmp_reg_reg(b, BINARY_GP_R9, gpr) &&
         binary_emit_cmovcc_reg_reg(b, 0x4F /* cmovg */, gpr, BINARY_GP_R9);
}

int code_generator_binary_emit_prefix_sum_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 1) {
    return 0;
  }
  b = &context->code;

  /* r8=sum, rcx=src, rdx=dst, r9=end */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_R8) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->arguments[0],
                                               BINARY_GP_R11) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R11) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R9, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R9, BINARY_GP_RCX)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R9) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R8, BINARY_GP_R10) ||
      !binary_emit_mov_mem_reg32(b, BINARY_GP_RDX, 0, BINARY_GP_R8) ||
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
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_R8)) {
    return 0;
  }
  return 1;
}

int code_generator_binary_emit_simd_minmax_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction ||
      instruction->argument_count < 1) {
    return 0;
  }
  b = &context->code;

  /* r10=min, r11=max, rcx=walk, r8=end, xmm4/xmm5 extrema */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(
          generator, context, &instruction->arguments[0], BINARY_GP_R11) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_RDX) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 0, 2) ||
      !wcs_add_reg_reg64(b, BINARY_GP_R8, BINARY_GP_RCX) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
      !wcs_broadcast_i32_to_xmm(b, 4, BINARY_GP_R10) ||
      !wcs_broadcast_i32_to_xmm(b, 5, BINARY_GP_R11)) {
    return 0;
  }

  loop_top = b->size;
  if (!binary_emit_cmp_reg_reg(b, BINARY_GP_RCX, BINARY_GP_R8) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_done)) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !binary_emit_alu_reg_reg(b, 0x29, BINARY_GP_R9, BINARY_GP_RCX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 64) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec) ||
      !wcs_jcc(b, 0, &j_scalar)) {
    return 0;
  }

  if (!wcs_patch_here(b, j_vec) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 0) ||
      !wcs_pminsd(b, 4, 0) ||
      !wcs_pmaxsd(b, 5, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 16) ||
      !wcs_pminsd(b, 4, 0) ||
      !wcs_pmaxsd(b, 5, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 32) ||
      !wcs_pminsd(b, 4, 0) ||
      !wcs_pmaxsd(b, 5, 0) ||
      !simd_movdqu_xmm_mem_disp(b, 0, BINARY_GP_RCX, 48) ||
      !wcs_pminsd(b, 4, 0) ||
      !wcs_pmaxsd(b, 5, 0) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 64)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R14, BINARY_GP_RCX, 0) ||
      !binary_emit_cmp_reg_reg(b, BINARY_GP_R14, BINARY_GP_R10) ||
      !binary_emit_cmovcc_reg_reg(b, 0x4C /* cmovl */, BINARY_GP_R10,
                                  BINARY_GP_R14) ||
      !binary_emit_cmp_reg_reg(b, BINARY_GP_R14, BINARY_GP_R11) ||
      !binary_emit_cmovcc_reg_reg(b, 0x4F /* cmovg */, BINARY_GP_R11,
                                  BINARY_GP_R14) ||
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
      !wcs_horizontal_pminsd_to_reg(b, 4, BINARY_GP_R10) ||
      !wcs_horizontal_pmaxsd_to_reg(b, 5, BINARY_GP_R11) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_R10) ||
      !code_generator_binary_emit_destination_store(
          generator, context, &instruction->arguments[0], BINARY_GP_R11)) {
    return 0;
  }
  return 1;
}
