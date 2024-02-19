// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 CPU Types
 *
 *****************************************************************************/

#ifndef MAME_CPU_KENBAK1_KENBAK1TYPES_H
#define MAME_CPU_KENBAK1_KENBAK1TYPES_H

#pragma once

class kenbak1_reg {
public:
	enum id {
		A,
		B,
		X,
		P,   // Program Counter
		OUT, // Lamps
		OCA, // Overflow and Carry for A
		OCB, // " for B
		OCX, // " for X
		IN,  // Bit switches on front panel
		REG_MAX,
		REG_NONE = -1,
	};

	static const offs_t addrs[];
	static id is_reg(offs_t addr);
	static const char* to_string(id val);
};

class kenbak1_opcode
{
public:
	enum func {
		ADD,
		SUB,
		LOAD,
		STORE,
		OR,
		AND,
		LNEG,
		JPD,
		JPI,
		JMD,
		JMI,
		SKP_0,
		SKP_1,
		SET_0,
		SET_1,
		SFTL,
		SFTR,
		ROTL,
		ROTR,
		NOOP,
		HALT,
		FUNC_INVALID = -1,
	};

	enum param {
		PARAM_NONE = 0,
		CONSTANT,
		MEMORY,
		INDIRECT,
		INDEXED,
		INDIRECT_INDEXED,
	};

	enum cond {
		COND_NONE, // Unconditional
		NZERO,     // != 0
		ZERO,      // = 0
		NEG,       // < 0
		POS,       // >= 0
		POS_NZ,    // > 0
	};

	kenbak1_opcode(u8 opcode);

	u8 get_raw() const              { return m_opcode; }
	func get_func() const           { return m_func; }
	bool has_param() const          { return m_param != PARAM_NONE; }
	param get_param() const         { return m_param; }
	kenbak1_reg::id get_reg() const { return m_reg; }
	u8 get_bitpos() const           { return m_bitpos; }
	cond get_cond() const           { return m_cond; }
	bool is_jump_mark() const       { return (m_opcode == JMD) || (m_opcode == JMI); }

	static const char* get_func_string(func val);
	const char* get_func_string() { return get_func_string(m_func); }

	static const char* get_cond_string(cond val);
	const char* get_cond_string() { return get_cond_string(m_cond); }

	const char* get_reg_string() { return kenbak1_reg::to_string(m_reg); }

private:
	u8              m_opcode;
	func            m_func;
	param           m_param;
	kenbak1_reg::id m_reg;
	u8              m_bitpos;
	cond            m_cond;

	void decode();
	void decode_shift_rotate();
	void decode_set_skip();
	void decode_jump();
	void decode_bitwise();
	void decode_alu();
	void decode_reg();
	void decode_param();
};

#endif // MAME_CPU_KENBAK1_KENBAK1TYPES_H
