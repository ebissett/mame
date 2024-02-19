// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 disassembler
 *
 *****************************************************************************/

#ifndef MAME_CPU_KENBAK1_KENBAK1DASM_H
#define MAME_CPU_KENBAK1_KENBAK1DASM_H

#pragma once

#include "kenbak1types.h"

class kenbak1_disassembler : public util::disasm_interface
{
public:
	kenbak1_disassembler() = default;
	virtual ~kenbak1_disassembler() = default;

	virtual u32 opcode_alignment() const override;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) override;

private:
	void stream_param(std::ostream &stream, kenbak1_opcode::param ptype, u8 param);
};

#endif // MAME_CPU_KENBAK1_KENBAK1DASM_H
