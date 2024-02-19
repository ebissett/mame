// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 disassembler
 *
 * Syntax style is based off the wikipedia page (https://en.wikipedia.org/wiki/Kenbak-1)
 * for the device.  The offical Programmer's Reference has a slightly different
 * style.
 *
 * All numerical addresses are displayed in octal unless they specify a register
 * location in which case the register name is displayed.
 *
 *****************************************************************************/

#include "emu.h"
#include "kenbak1dasm.h"

u32 kenbak1_disassembler::opcode_alignment() const
{
	return 1;
}

offs_t kenbak1_disassembler::disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params)
{
	offs_t flags = 0;
	u8 length = 1;
	u8 param = 0;

	// Don't disassemble memory mapped register addresses
	if (kenbak1_reg::is_reg(pc) != kenbak1_reg::REG_NONE) {
		util::stream_format(stream, "REG<%s>", kenbak1_reg::to_string(kenbak1_reg::is_reg(pc)));

		return length | SUPPORTED;
	}

	kenbak1_opcode opcode(opcodes.r8(pc));
	if (opcode.has_param()) {
		length++;
		param = opcodes.r8(pc + 1);
	}

	// Write out mnemonic
	util::stream_format(stream, "%s ", opcode.get_func_string());

	// Write out parameters
	switch (opcode.get_func()) {
		case kenbak1_opcode::ADD:
		case kenbak1_opcode::SUB:
		case kenbak1_opcode::LOAD:
			util::stream_format(stream, "%s ", opcode.get_reg_string());
			stream_param(stream, opcode.get_param(), param);
			break;

		case kenbak1_opcode::STORE:
			stream_param(stream, opcode.get_param(), param);
			util::stream_format(stream, " %s", opcode.get_reg_string());
			break;

		case kenbak1_opcode::OR:
		case kenbak1_opcode::AND:
		case kenbak1_opcode::LNEG:
			stream_param(stream, opcode.get_param(), param);
			break;

		case kenbak1_opcode::JPD:
		case kenbak1_opcode::JPI:
		case kenbak1_opcode::JMD:
		case kenbak1_opcode::JMI:
			if (opcode.get_cond() != kenbak1_opcode::COND_NONE) {
				util::stream_format(stream, "%s ", opcode.get_reg_string());
			}
			util::stream_format(stream, "%s ", opcode.get_cond_string());
			stream_param(stream, opcode.get_param(), param);
			break;

		case kenbak1_opcode::SKP_0:
		case kenbak1_opcode::SKP_1:
		case kenbak1_opcode::SET_0:
		case kenbak1_opcode::SET_1:
			util::stream_format(stream, "b%d ", opcode.get_bitpos());
			stream_param(stream, opcode.get_param(), param);
			break;

		case kenbak1_opcode::SFTL:
		case kenbak1_opcode::SFTR:
		case kenbak1_opcode::ROTL:
		case kenbak1_opcode::ROTR:
			util::stream_format(stream, "%s%d", opcode.get_reg_string(), opcode.get_bitpos());
			break;

		case kenbak1_opcode::NOOP:
		case kenbak1_opcode::HALT:
		default:
			break;
	}

	return length | flags | SUPPORTED;
}

void kenbak1_disassembler::stream_param(std::ostream &stream, kenbak1_opcode::param ptype, u8 param)
{
	switch (ptype) {
		case kenbak1_opcode::CONSTANT:
			util::stream_format(stream, "#%03o", param);
			break;
		case kenbak1_opcode::MEMORY:
			{
				kenbak1_reg::id reg = kenbak1_reg::is_reg(param);
				if (reg != kenbak1_reg::REG_NONE) {
					util::stream_format(stream, "%s", kenbak1_reg::to_string(reg));
				}
				else {
					util::stream_format(stream, "%03o", param);
				}
			}
			break;
		case kenbak1_opcode::INDIRECT:
			util::stream_format(stream, "(%03o)", param);
			break;
		case kenbak1_opcode::INDEXED:
			util::stream_format(stream, "%03o, %s", param, kenbak1_reg::to_string(kenbak1_reg::X));
			break;
		case kenbak1_opcode::INDIRECT_INDEXED:
			util::stream_format(stream, "(%03o), %s", param, kenbak1_reg::to_string(kenbak1_reg::X));
			break;
		default:
			util::stream_format(stream, "INVALID_PARAM", param);
			break;
	}
}
