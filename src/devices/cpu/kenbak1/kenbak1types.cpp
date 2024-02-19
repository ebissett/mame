// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 CPU Types
 *
 *****************************************************************************/

#include "kenbak1types.h"

//static
const offs_t kenbak1_reg::addrs[] = {
	0000, // A
	0001, // B
	0002, // X
	0003, // P
	0200, // OUT
	0201, // OCA
	0202, // OCB
	0203, // OCX
	0377, // IN
};

//static
kenbak1_reg::id kenbak1_reg::is_reg(offs_t addr)
{
	for (int reg = 0; reg < REG_MAX; reg++) {
		if (addr == addrs[reg]) {
			return (id)reg;
		}
	}
	return REG_NONE;
}

//static
const char* kenbak1_reg::to_string(id val)
{
	switch (val) {
		case kenbak1_reg::A:   return "A";
		case kenbak1_reg::B:   return "B";
		case kenbak1_reg::X:   return "X";
		case kenbak1_reg::P:   return "P";
		case kenbak1_reg::OUT: return "OUT";
		case kenbak1_reg::OCA: return "OCA";
		case kenbak1_reg::OCB: return "OCB";
		case kenbak1_reg::OCX: return "OCX";
		case kenbak1_reg::IN:  return "IN";
		default:               return "INVALID_REG";
	}
}

kenbak1_opcode::kenbak1_opcode(u8 opcode)
	: m_opcode(opcode)
	, m_func(FUNC_INVALID)
	, m_param(PARAM_NONE)
	, m_reg(kenbak1_reg::REG_NONE)
	, m_bitpos(0)
	, m_cond(COND_NONE)
{
	decode();
}

// Quick macros for accessing opcode as octal digits
#define m_lo  (m_opcode & 0007)
#define m_mid ((m_opcode & 0070) >> 3)
#define m_hi  ((m_opcode & 0300) >> 6)

void kenbak1_opcode::decode()
{
	switch (m_lo) {
		case 0:
			m_func = (m_hi & 2) ? NOOP : HALT;
			break;
		case 1:
			decode_shift_rotate();
			break;
		case 2:
			decode_set_skip();
			break;
		default:
			if (m_mid >= 4) {
				decode_jump();
			}
			else if (m_hi == 3) {
				decode_bitwise();
			}
			else {
				decode_alu();
			}
			break;
	}
}

void kenbak1_opcode::decode_shift_rotate()
{
	switch (m_hi) {
		case 0: m_func = SFTR; break;
		case 1: m_func = ROTR; break;
		case 2: m_func = SFTL; break;
		case 3: m_func = SFTR; break;
	}
	m_reg = (m_mid & 4) ? kenbak1_reg::B : kenbak1_reg::A;
	m_bitpos = (m_mid & 3) ? (m_mid & 3) : 4;
}

void kenbak1_opcode::decode_set_skip()
{
	switch (m_hi) {
		case 0: m_func = SET_0; break;
		case 1: m_func = SET_1; break;
		case 2: m_func = SKP_0; break;
		case 3: m_func = SKP_1; break;
	}
	m_param = MEMORY;
	m_bitpos = m_mid;
}

void kenbak1_opcode::decode_jump()
{
	decode_reg();

	switch (m_mid & 3) {
		case 0:
			m_func = JPD;
			m_param = MEMORY;
			break;
		case 1:
			m_func = JPI;
			m_param = INDIRECT;
			break;
		case 2:
			m_func = JMD;
			m_param = MEMORY;
			break;
		case 3:
			m_func = JMI;
			m_param = INDIRECT;
			break;
	}

	// No register means unconditional jumps
	if (m_reg == kenbak1_reg::REG_NONE) {
		m_cond = COND_NONE;
	}
	else {
		switch (m_lo) {
			case 3: m_cond = NZERO;  break;
			case 4: m_cond = ZERO;   break;
			case 5: m_cond = NEG;    break;
			case 6: m_cond = POS;    break;
			case 7: m_cond = POS_NZ; break;
		}
	}
}

void kenbak1_opcode::decode_bitwise()
{
	switch (m_mid) {
		case 0: m_func = OR;   break;
		case 1: m_func = NOOP; break;
		case 2: m_func = AND;  break;
		case 3: m_func = LNEG; break;
	}

	if (m_func != NOOP) {
		decode_param();
	}
}

void kenbak1_opcode::decode_alu()
{
	switch (m_mid) {
		case 0: m_func = ADD;   break;
		case 1: m_func = SUB;   break;
		case 2: m_func = LOAD;  break;
		case 3: m_func = STORE; break;
	}
	decode_reg();
	decode_param();
}

void kenbak1_opcode::decode_reg()
{
	switch (m_hi) {
		case 0:  m_reg = kenbak1_reg::A; break;
		case 1:  m_reg = kenbak1_reg::B; break;
		case 2:  m_reg = kenbak1_reg::X; break;
		default: m_reg = kenbak1_reg::REG_NONE; break;
	}
}

void kenbak1_opcode::decode_param()
{
	switch (m_lo) {
		case 3:  m_param = CONSTANT;         break;
		case 4:  m_param = MEMORY;           break;
		case 5:  m_param = INDIRECT;         break;
		case 6:  m_param = INDEXED;          break;
		case 7:  m_param = INDIRECT_INDEXED; break;
	}
}

//static
const char* kenbak1_opcode::get_func_string(func val)
{
	switch (val) {
		case ADD:   return "add";
		case SUB:   return "sub";
		case LOAD:  return "load";
		case STORE: return "store";
		case OR:    return "or";
		case AND:   return "and";
		case LNEG:  return "lneg";
		case JPD:   return "jpd";
		case JPI:   return "jpi";
		case JMD:   return "jmd";
		case JMI:   return "jmi";
		case SKP_0: return "skp 0";
		case SKP_1: return "skp 1";
		case SET_0: return "set 0";
		case SET_1: return "set 1";
		case SFTL:  return "sftl";
		case SFTR:  return "sftr";
		case ROTL:  return "rotl";
		case ROTR:  return "rotr";
		case NOOP:  return "noop";
		case HALT:  return "halt";
		default:    return "INVALID_FUNC";
	}
}

//static
const char* kenbak1_opcode::get_cond_string(cond val)
{
	switch (val) {
		case COND_NONE: return "UNC";
		case NZERO:     return "!=0";
		case ZERO:      return "=0";
		case NEG:       return "<0";
		case POS:       return ">=0";
		case POS_NZ:    return ">0";
		default:        return "INVALID_COND";
	}
}
