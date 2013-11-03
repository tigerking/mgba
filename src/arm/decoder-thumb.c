#include "decoder.h"

#include "arm.h"
#include "decoder-inlines.h"
#include "emitter-thumb.h"
#include "isa-inlines.h"

#include <stdio.h>
#include <string.h>

#define DEFINE_THUMB_DECODER(NAME, MNEMONIC, BODY) \
	static void _ThumbDecode ## NAME (uint16_t opcode, struct ThumbInstructionInfo* info) { \
		UNUSED(opcode); \
		info->mnemonic = THUMB_MN_ ## MNEMONIC; \
		BODY; \
	}

#define DEFINE_IMMEDIATE_5_DECODER_DATA_THUMB(NAME, IMMEDIATE, MNEMONIC) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op3.immediate = IMMEDIATE; \
		info->op1.reg = opcode & 0x0007; \
		info->op2.reg = (opcode >> 3) & 0x0007; \
		info->affectsCPSR = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_REGISTER_2 | \
			ARM_OPERAND_IMMEDIATE_3;)

#define DEFINE_IMMEDIATE_5_DECODER_MEM_THUMB(NAME, IMMEDIATE, MNEMONIC, CYCLES) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = opcode & 0x0007; \
		info->memory.baseReg = (opcode >> 3) & 0x0007; \
		info->memory.offset.immediate = IMMEDIATE << 2; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_MEMORY_2; \
		info->memory.format = ARM_MEMORY_REGISTER_BASE | \
			ARM_MEMORY_IMMEDIATE_OFFSET; \
		CYCLES)

#define DEFINE_IMMEDIATE_5_DECODER_MEM_LOAD_THUMB(NAME, IMMEDIATE, MNEMONIC) \
	DEFINE_IMMEDIATE_5_DECODER_MEM_THUMB(NAME, IMMEDIATE, MNEMONIC, LOAD_CYCLES)

#define DEFINE_IMMEDIATE_5_DECODER_MEM_STORE_THUMB(NAME, IMMEDIATE, MNEMONIC) \
	DEFINE_IMMEDIATE_5_DECODER_MEM_THUMB(NAME, IMMEDIATE, MNEMONIC, STORE_CYCLES)

#define DEFINE_IMMEDIATE_5_DECODER_THUMB(NAME, MNEMONIC, TYPE) \
	COUNT_5(DEFINE_IMMEDIATE_5_DECODER_ ## TYPE ## _THUMB, NAME ## _, MNEMONIC)

DEFINE_IMMEDIATE_5_DECODER_THUMB(LSL1, LSL, DATA)
DEFINE_IMMEDIATE_5_DECODER_THUMB(LSR1, LSR, DATA)
DEFINE_IMMEDIATE_5_DECODER_THUMB(ASR1, ASR, DATA)
DEFINE_IMMEDIATE_5_DECODER_THUMB(LDR1, LDR, MEM_LOAD)
DEFINE_IMMEDIATE_5_DECODER_THUMB(LDRB1, LDRB, MEM_LOAD)
DEFINE_IMMEDIATE_5_DECODER_THUMB(LDRH1, LDRH, MEM_LOAD)
DEFINE_IMMEDIATE_5_DECODER_THUMB(STR1, STR, MEM_STORE)
DEFINE_IMMEDIATE_5_DECODER_THUMB(STRB1, STRB, MEM_STORE)
DEFINE_IMMEDIATE_5_DECODER_THUMB(STRH1, STRH, MEM_STORE)

#define DEFINE_DATA_FORM_1_DECODER_EX_THUMB(NAME, RM, MNEMONIC) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = opcode & 0x0007; \
		info->op2.reg = (opcode >> 3) & 0x0007; \
		info->op3.reg = RM; \
		info->affectsCPSR = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_REGISTER_2 | \
			ARM_OPERAND_REGISTER_3;)

#define DEFINE_DATA_FORM_1_DECODER_THUMB(NAME) \
	COUNT_3(DEFINE_DATA_FORM_1_DECODER_EX_THUMB, NAME ## 3_R, NAME)

DEFINE_DATA_FORM_1_DECODER_THUMB(ADD)
DEFINE_DATA_FORM_1_DECODER_THUMB(SUB)

#define DEFINE_DATA_FORM_2_DECODER_EX_THUMB(NAME, IMMEDIATE, MNEMONIC) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = opcode & 0x0007; \
		info->op2.reg = (opcode >> 3) & 0x0007; \
		info->op3.immediate = IMMEDIATE; \
		info->affectsCPSR = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_REGISTER_2 | \
			ARM_OPERAND_IMMEDIATE_3;)

#define DEFINE_DATA_FORM_2_DECODER_THUMB(NAME) \
	COUNT_3(DEFINE_DATA_FORM_2_DECODER_EX_THUMB, NAME ## 1_, NAME)

DEFINE_DATA_FORM_2_DECODER_THUMB(ADD)
DEFINE_DATA_FORM_2_DECODER_THUMB(SUB)

#define DEFINE_DATA_FORM_3_DECODER_EX_THUMB(NAME, RD, MNEMONIC, AFFECTED) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = RD; \
		info->op2.immediate = opcode & 0x00FF; \
		info->affectsCPSR = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			AFFECTED | \
			ARM_OPERAND_IMMEDIATE_2;)

#define DEFINE_DATA_FORM_3_DECODER_THUMB(NAME, MNEMONIC, AFFECTED) \
	COUNT_3(DEFINE_DATA_FORM_3_DECODER_EX_THUMB, NAME ## _R, MNEMONIC, AFFECTED)

DEFINE_DATA_FORM_3_DECODER_THUMB(ADD2, ADD, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_3_DECODER_THUMB(CMP1, CMP, ARM_OPERAND_NONE)
DEFINE_DATA_FORM_3_DECODER_THUMB(MOV1, MOV, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_3_DECODER_THUMB(SUB2, SUB, ARM_OPERAND_AFFECTED_1)

#define DEFINE_DATA_FORM_5_DECODER_THUMB(NAME, MNEMONIC, AFFECTED) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = opcode & 0x0007; \
		info->op2.reg = (opcode >> 3) & 0x0007; \
		info->affectsCPSR = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			AFFECTED | \
			ARM_OPERAND_REGISTER_2;)

DEFINE_DATA_FORM_5_DECODER_THUMB(AND, AND, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(EOR, EOR, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(LSL2, LSL, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(LSR2, LSR, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(ASR2, ASR, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(ADC, ADC, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(SBC, SBC, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(ROR, ROR, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(TST, TST, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(NEG, NEG, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(CMP2, CMP, ARM_OPERAND_NONE)
DEFINE_DATA_FORM_5_DECODER_THUMB(CMN, CMN, ARM_OPERAND_NONE)
DEFINE_DATA_FORM_5_DECODER_THUMB(ORR, ORR, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(MUL, MUL, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(BIC, BIC, ARM_OPERAND_AFFECTED_1)
DEFINE_DATA_FORM_5_DECODER_THUMB(MVN, MVN, ARM_OPERAND_AFFECTED_1)

#define DEFINE_DECODER_WITH_HIGH_EX_THUMB(NAME, H1, H2, MNEMONIC, AFFECTED, CPSR) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = (opcode & 0x0007) | H1; \
		info->op2.reg = ((opcode >> 3) & 0x0007) | H2; \
		info->accessesSpecialRegisters = info->op1.reg > 12 || info->op2.reg > 12; \
		info->branches = info->op1.reg == ARM_PC; \
		info->affectsCPSR = CPSR; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			AFFECTED | \
			ARM_OPERAND_REGISTER_2;)


#define DEFINE_DECODER_WITH_HIGH_THUMB(NAME, MNEMONIC, AFFECTED, CPSR) \
	DEFINE_DECODER_WITH_HIGH_EX_THUMB(NAME ## 00, 0, 0, MNEMONIC, AFFECTED, CPSR) \
	DEFINE_DECODER_WITH_HIGH_EX_THUMB(NAME ## 01, 0, 8, MNEMONIC, AFFECTED, CPSR) \
	DEFINE_DECODER_WITH_HIGH_EX_THUMB(NAME ## 10, 8, 0, MNEMONIC, AFFECTED, CPSR) \
	DEFINE_DECODER_WITH_HIGH_EX_THUMB(NAME ## 11, 8, 8, MNEMONIC, AFFECTED, CPSR)

DEFINE_DECODER_WITH_HIGH_THUMB(ADD4, ADD, ARM_OPERAND_AFFECTED_1, 0)
DEFINE_DECODER_WITH_HIGH_THUMB(CMP3, CMP, ARM_OPERAND_NONE, 1)
DEFINE_DECODER_WITH_HIGH_THUMB(MOV3, MOV, ARM_OPERAND_AFFECTED_1, 0)

#define DEFINE_IMMEDIATE_WITH_REGISTER_DATA_THUMB(NAME, RD, MNEMONIC, REG) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = RD; \
		info->op2.reg = REG; \
		info->op3.immediate = (opcode & 0x00FF) << 2; \
		info->accessesSpecialRegisters = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_REGISTER_2 | \
			ARM_OPERAND_IMMEDIATE_3;)

#define DEFINE_IMMEDIATE_WITH_REGISTER_MEM_THUMB(NAME, RD, MNEMONIC, REG, CYCLES) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = RD; \
		info->memory.baseReg = REG; \
		info->memory.offset.immediate = (opcode & 0x00FF) << 2; \
		info->accessesSpecialRegisters = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_MEMORY_2; \
		info->memory.format = ARM_MEMORY_REGISTER_BASE | \
			ARM_MEMORY_IMMEDIATE_OFFSET; \
		CYCLES;)

#define DEFINE_IMMEDIATE_WITH_REGISTER_MEM_LOAD_THUMB(NAME, RD, MNEMONIC, REG) \
	DEFINE_IMMEDIATE_WITH_REGISTER_MEM_THUMB(NAME, RD, MNEMONIC, REG, LOAD_CYCLES)

#define DEFINE_IMMEDIATE_WITH_REGISTER_MEM_STORE_THUMB(NAME, RD, MNEMONIC, REG) \
	DEFINE_IMMEDIATE_WITH_REGISTER_MEM_THUMB(NAME, RD, MNEMONIC, REG, STORE_CYCLES)

#define DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(NAME, MNEMONIC, TYPE, REG) \
	COUNT_3(DEFINE_IMMEDIATE_WITH_REGISTER_ ## TYPE ## _THUMB, NAME ## _R, MNEMONIC, REG)

DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(LDR3, LDR, MEM_LOAD, ARM_PC)
DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(LDR4, LDR, MEM_LOAD, ARM_SP)
DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(STR3, STR, MEM_STORE, ARM_SP)

DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(ADD5, ADD, DATA, ARM_PC)
DEFINE_IMMEDIATE_WITH_REGISTER_THUMB(ADD6, ADD, DATA, ARM_SP)

#define DEFINE_LOAD_STORE_WITH_REGISTER_EX_THUMB(NAME, RM, MNEMONIC, CYCLES) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->memory.offset.reg = RM; \
		info->op1.reg = opcode & 0x0007; \
		info->memory.baseReg = (opcode >> 3) & 0x0007; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_MEMORY_2; \
		info->memory.format = ARM_MEMORY_REGISTER_BASE | \
			ARM_MEMORY_REGISTER_OFFSET; \
		CYCLES;)

#define DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(NAME, MNEMONIC, CYCLES) \
	COUNT_3(DEFINE_LOAD_STORE_WITH_REGISTER_EX_THUMB, NAME ## _R, MNEMONIC, CYCLES)

DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(LDR2, LDR, LOAD_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(LDRB2, LDRB, LOAD_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(LDRH2, LDRH, LOAD_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(LDRSB, LDRSB, LOAD_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(LDRSH, LDRSH, LOAD_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(STR2, STR, STORE_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(STRB2, STRB, STORE_CYCLES)
DEFINE_LOAD_STORE_WITH_REGISTER_THUMB(STRH2, STRH, STORE_CYCLES)

#define DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB(NAME, RN, MNEMONIC, SPECIAL_REG, ADDITIONAL_REG) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->memory.baseReg = RN; \
		info->accessesSpecialRegisters = SPECIAL_REG; \
		info->op1.immediate = (opcode & 0xFF) | ADDITIONAL_REG; \
		info->branches = info->op1.immediate & (1 << ARM_PC); \
		info->operandFormat = ARM_OPERAND_MEMORY_1; \
		info->memory.format = ARM_MEMORY_REGISTER_BASE | \
			ARM_MEMORY_POST_INCREMENT;)

#define DEFINE_LOAD_STORE_MULTIPLE_THUMB(NAME) \
	COUNT_3(DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB, NAME ## _R, NAME, 0, 0)

DEFINE_LOAD_STORE_MULTIPLE_THUMB(LDMIA)
DEFINE_LOAD_STORE_MULTIPLE_THUMB(STMIA)

#define DEFINE_CONDITIONAL_BRANCH_THUMB(COND) \
	DEFINE_THUMB_DECODER(B ## COND, B, \
		int8_t immediate = opcode; \
		info->op1.immediate = immediate << 1; \
		info->branches = 1; \
		info->condition = ARM_CONDITION_ ## COND; \
		info->operandFormat = ARM_OPERAND_IMMEDIATE_1;)

DEFINE_CONDITIONAL_BRANCH_THUMB(EQ)
DEFINE_CONDITIONAL_BRANCH_THUMB(NE)
DEFINE_CONDITIONAL_BRANCH_THUMB(CS)
DEFINE_CONDITIONAL_BRANCH_THUMB(CC)
DEFINE_CONDITIONAL_BRANCH_THUMB(MI)
DEFINE_CONDITIONAL_BRANCH_THUMB(PL)
DEFINE_CONDITIONAL_BRANCH_THUMB(VS)
DEFINE_CONDITIONAL_BRANCH_THUMB(VC)
DEFINE_CONDITIONAL_BRANCH_THUMB(LS)
DEFINE_CONDITIONAL_BRANCH_THUMB(HI)
DEFINE_CONDITIONAL_BRANCH_THUMB(GE)
DEFINE_CONDITIONAL_BRANCH_THUMB(LT)
DEFINE_CONDITIONAL_BRANCH_THUMB(GT)
DEFINE_CONDITIONAL_BRANCH_THUMB(LE)

#define DEFINE_SP_MODIFY_THUMB(NAME, MNEMONIC) \
	DEFINE_THUMB_DECODER(NAME, MNEMONIC, \
		info->op1.reg = ARM_SP; \
		info->op2.immediate = (opcode & 0x7F) << 2; \
		info->accessesSpecialRegisters = 1; \
		info->operandFormat = ARM_OPERAND_REGISTER_1 | \
			ARM_OPERAND_AFFECTED_1 | \
			ARM_OPERAND_IMMEDIATE_2;)

DEFINE_SP_MODIFY_THUMB(ADD7, ADD)
DEFINE_SP_MODIFY_THUMB(SUB4, SUB)

DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB(POP, ARM_SP, POP, 1, 0)
DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB(POPR, ARM_SP, POP, 1, 1 << ARM_PC)
DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB(PUSH, ARM_SP, PUSH, 1, 0)
DEFINE_LOAD_STORE_MULTIPLE_EX_THUMB(PUSHR, ARM_SP, PUSH, 1, 1 << ARM_LR)

DEFINE_THUMB_DECODER(ILL, ILL, info->traps = 1;)
DEFINE_THUMB_DECODER(BKPT, BKPT, info->traps = 1;)

DEFINE_THUMB_DECODER(B, B,
	int16_t immediate = (opcode & 0x07FF) << 5;
	info->op1.immediate = (((int32_t) immediate) >> 4);
	info->operandFormat = ARM_OPERAND_IMMEDIATE_1;
	info->branches = 1;)

DEFINE_THUMB_DECODER(BL1, BLH,
	int16_t immediate = (opcode & 0x07FF) << 5;
	info->op1.immediate = (((int32_t) immediate) << 7);
	info->operandFormat = ARM_OPERAND_IMMEDIATE_1;
	info->accessesSpecialRegisters = 1;)

DEFINE_THUMB_DECODER(BL2, BL,
	info->op1.immediate = (opcode & 0x07FF) << 1;
	info->operandFormat = ARM_OPERAND_IMMEDIATE_1;
	info->accessesSpecialRegisters = 1;
	info->branches = 1;)

DEFINE_THUMB_DECODER(BX, BX,
	info->op1.reg = (opcode >> 3) & 0xF;
	info->operandFormat = ARM_OPERAND_REGISTER_1;
	info->branches = 1;)

DEFINE_THUMB_DECODER(SWI, SWI,
	info->op1.immediate = opcode & 0xFF;
	info->operandFormat = ARM_OPERAND_IMMEDIATE_1;
	info->traps = 1;)

typedef void (*ThumbDecoder)(uint16_t opcode, struct ThumbInstructionInfo* info);

static const ThumbDecoder _thumbDecoderTable[0x400] = {
	DECLARE_THUMB_EMITTER_BLOCK(_ThumbDecode)
};

void ARMDecodeThumb(uint16_t opcode, struct ThumbInstructionInfo* info) {
	info->opcode = opcode;
	info->branches = 0;
	info->traps = 0;
	info->accessesSpecialRegisters = 0;
	info->affectsCPSR = 0;
	info->condition = ARM_CONDITION_AL;
	info->sDataCycles = 0;
	info->nDataCycles = 0;
	info->sInstructionCycles = 1;
	info->nInstructionCycles = 0;
	info->iCycles = 0;
	info->cCycles = 0;
	ThumbDecoder decoder = _thumbDecoderTable[opcode >> 6];
	decoder(opcode, info);
}

const char* thumbMnemonicStrings[] = {
	"ill",
	"adc",
	"add",
	"and",
	"asr",
	"b",
	"bic",
	"bkpt",
	"bl",
	"blh",
	"bx",
	"cmn",
	"cmp",
	"eor",
	"ldmia",
	"ldr",
	"ldrb",
	"ldrh",
	"ldrsb",
	"ldrsh",
	"lsl",
	"lsr",
	"mov",
	"mul",
	"mvn",
	"neg",
	"orr",
	"pop",
	"push",
	"ror",
    "sbc",
    "stmia",
	"str",
	"strb",
	"strh",
	"sub",
	"swi",
	"tst"
};

int ARMDisassembleThumb(uint16_t opcode, uint32_t pc, char* buffer, int blen) {
	struct ThumbInstructionInfo info;
	ARMDecodeThumb(opcode, &info);
	const char* mnemonic = thumbMnemonicStrings[info.mnemonic];
	int written;
	int total = 0;
	const char* cond = "";
	if (info.condition != ARM_CONDITION_AL && info.condition < ARM_CONDITION_NV) {
		cond = _armConditions[info.condition];
	}
	written = snprintf(buffer, blen, "%s%s ", mnemonic, cond);
	ADVANCE(written);

	switch (info.mnemonic) {
	case THUMB_MN_LDMIA:
	case THUMB_MN_STMIA:
		written = _decodeRegister(info.memory.baseReg, buffer, blen);
		ADVANCE(written);
		strncpy(buffer, "!, ", blen);
		ADVANCE(3);
		// Fall through
	case THUMB_MN_POP:
	case THUMB_MN_PUSH:
		written = _decodeRegisterList(info.op1.immediate, buffer, blen);
		ADVANCE(written);
		break;
	case THUMB_MN_B:
		written = _decodePCRelative(info.op1.immediate, pc, buffer, blen);
		ADVANCE(written);
		break;
	default:
		if (info.operandFormat & ARM_OPERAND_IMMEDIATE_1) {
			written = snprintf(buffer, blen, "#%i", info.op1.immediate);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_MEMORY_1) {
			written = _decodeMemory(info.memory, pc, buffer, blen);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_REGISTER_1) {
			written = _decodeRegister(info.op1.reg, buffer, blen);
			ADVANCE(written);
		}
		if (info.operandFormat & ARM_OPERAND_2) {
			strncpy(buffer, ", ", blen);
			ADVANCE(2);
		}
		if (info.operandFormat & ARM_OPERAND_IMMEDIATE_2) {
			written = snprintf(buffer, blen, "#%i", info.op2.immediate);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_MEMORY_2) {
			written = _decodeMemory(info.memory, pc, buffer, blen);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_REGISTER_2) {
			written = _decodeRegister(info.op2.reg, buffer, blen);
			ADVANCE(written);
		}
		if (info.operandFormat & ARM_OPERAND_3) {
			strncpy(buffer, ", ", blen);
			ADVANCE(2);
		}
		if (info.operandFormat & ARM_OPERAND_IMMEDIATE_3) {
			written = snprintf(buffer, blen, "#%i", info.op3.immediate);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_MEMORY_3) {
			written = _decodeMemory(info.memory, pc, buffer, blen);
			ADVANCE(written);
		} else if (info.operandFormat & ARM_OPERAND_REGISTER_3) {
			written = _decodeRegister(info.op1.reg, buffer, blen);
			ADVANCE(written);
		}
		break;
	}
	buffer[total] = '\0';
	return total;
}
