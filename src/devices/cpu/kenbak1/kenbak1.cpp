// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 CPU
 *
 *****************************************************************************/

#include "emu.h"
#include "kenbak1.h"
#include "kenbak1dasm.h"

DEFINE_DEVICE_TYPE(KENBAK1, kenbak1_cpu_device, "kenbak1", "Kenbak-1")

kenbak1_cpu_device::kenbak1_cpu_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: cpu_device(mconfig, type, tag, owner, clock)
	, m_program_config("program", ENDIANNESS_LITTLE, 8, 8, 0, address_map_constructor(FUNC(kenbak1_cpu_device::register_map), this))
	, m_icount(0)
	, m_halt(true)
	, m_A(0)
	, m_B(0)
	, m_X(0)
	, m_P(0)
	, m_OUT(0)
	, m_OCA(0)
	, m_OCB(0)
	, m_OCX(0)
	, m_IN(0)
	, m_write_out(*this)
	, m_write_halt(*this)
{}

kenbak1_cpu_device::kenbak1_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: kenbak1_cpu_device(mconfig, KENBAK1, tag, owner, clock)
{}

#define REG_MAP(NAME) \
	map(kenbak1_reg::addrs[kenbak1_reg::NAME], kenbak1_reg::addrs[kenbak1_reg::NAME]).rw(FUNC(kenbak1_cpu_device::read_ ## NAME), FUNC(kenbak1_cpu_device::write_ ## NAME));

void kenbak1_cpu_device::register_map(address_map &map)
{
	REG_MAP(A);
	REG_MAP(B);
	REG_MAP(X);
	REG_MAP(P);
	map(0004, 0177).ram();
	REG_MAP(OUT);
	REG_MAP(OCA);
	REG_MAP(OCB);
	REG_MAP(OCX);
	map(0204, 0376).ram();
	REG_MAP(IN);
}

device_memory_interface::space_config_vector kenbak1_cpu_device::memory_space_config() const
{
	return space_config_vector {std::make_pair(AS_PROGRAM, &m_program_config)};
}

std::unique_ptr<util::disasm_interface> kenbak1_cpu_device::create_disassembler()
{
	return std::make_unique<kenbak1_disassembler>();
}

#define STATE_ADD_REG(NAME) state_add(kenbak1_reg::NAME, #NAME, m_##NAME)

void kenbak1_cpu_device::device_start()
{
	space(AS_PROGRAM).specific(m_program);

	// Setup state table
	STATE_ADD_REG(A);
	STATE_ADD_REG(B);
	STATE_ADD_REG(X);
	STATE_ADD_REG(P);
	STATE_ADD_REG(OUT);
	STATE_ADD_REG(OCA);
	STATE_ADD_REG(OCB);
	STATE_ADD_REG(OCX);
	STATE_ADD_REG(IN);

	set_icountptr(m_icount);
}

void kenbak1_cpu_device::device_reset()
{
	m_halt = false;
	m_A = 0;
	m_B = 0;
	m_X = 0;
	m_P = 0;
	m_OUT = 0;
	m_OCA = 0;
	m_OCB = 0;
	m_OCX = 0;
	m_IN = 0;
}

#define REG_RW_DEFS(NAME) \
u8 kenbak1_cpu_device::read_ ## NAME() { return m_ ## NAME; } \
void kenbak1_cpu_device::write_ ## NAME(u8 val) { m_ ## NAME = val; }

REG_RW_DEFS(A);
REG_RW_DEFS(B);
REG_RW_DEFS(X);
REG_RW_DEFS(P);
REG_RW_DEFS(OUT);
REG_RW_DEFS(OCA);
REG_RW_DEFS(OCB);
REG_RW_DEFS(OCX);
REG_RW_DEFS(IN);

void kenbak1_cpu_device::set_halt(bool halt)
{
	m_halt = halt;
	m_write_halt(m_halt);
}

void kenbak1_cpu_device::execute_run()
{
	do {
		debugger_instruction_hook(m_P);

		if (m_halt) {
			m_icount = 0;
			break;
		}
		else {
			execute_one();

			// Right now we'll hard code all instructions to a value which
			// roughly approximates the documented instruction cost
			m_icount -= 2000;

			m_write_out(m_OUT);
			m_write_halt(m_halt);
		}
	} while (m_icount > 0);
}

u8 kenbak1_cpu_device::fetch()
{
	u8 opcode = m_program.read_byte(m_P);
	m_P++;
	return opcode;
}

u8* kenbak1_cpu_device::reg_from_id(kenbak1_reg::id val)
{
	switch (val) {
		case kenbak1_reg::A:   return &m_A;
		case kenbak1_reg::B:   return &m_B;
		case kenbak1_reg::X:   return &m_X;
		case kenbak1_reg::P:   return &m_P;
		case kenbak1_reg::OUT: return &m_OUT;
		case kenbak1_reg::OCA: return &m_OCA;
		case kenbak1_reg::OCB: return &m_OCB;
		case kenbak1_reg::OCX: return &m_OCX;
		case kenbak1_reg::IN:  return &m_IN;
		default:               return NULL;
	}
}

u8 kenbak1_cpu_device::read_param(kenbak1_opcode::param ptype, u8 param)
{
	switch (ptype) {
		case kenbak1_opcode::CONSTANT:
			return param;
		case kenbak1_opcode::MEMORY:
			return m_program.read_byte(param);
		case kenbak1_opcode::INDIRECT:
			return m_program.read_byte(m_program.read_byte(param));
		case kenbak1_opcode::INDEXED:
			return m_program.read_byte(param + m_X);
		case kenbak1_opcode::INDIRECT_INDEXED:
			return m_program.read_byte(m_program.read_byte(param) + m_X);
		default:
			// Shouldn't get here
			assert(false);
			break;
	}
	return 0;
}

void kenbak1_cpu_device::write_param(kenbak1_opcode::param ptype, u8 param, u8 val)
{
	switch (ptype) {
		case kenbak1_opcode::CONSTANT:
			// Write to a constant is legal but what does it actually do?
			// NOOP for now.
			break;
		case kenbak1_opcode::MEMORY:
			m_program.write_byte(param, val);
			break;
		case kenbak1_opcode::INDIRECT:
			m_program.write_byte(m_program.read_byte(param), val);
			break;
		case kenbak1_opcode::INDEXED:
			m_program.write_byte(param + m_X, val);
			break;
		case kenbak1_opcode::INDIRECT_INDEXED:
			m_program.write_byte(m_program.read_byte(param) + m_X, val);
			break;
		default:
			// Shouldn't get here
			assert(false);
			break;
	}
}

void kenbak1_cpu_device::execute_one()
{
	kenbak1_opcode opcode(fetch());
	u8 param = opcode.has_param() ? fetch() : 0;

	switch (opcode.get_func()) {
		case kenbak1_opcode::ADD:
			op_add(opcode, param);
			break;
		case kenbak1_opcode::SUB:
			op_sub(opcode, param);
			break;
		case kenbak1_opcode::LOAD:
			op_load(opcode, param);
			break;
		case kenbak1_opcode::STORE:
			op_store(opcode, param);
			break;
		case kenbak1_opcode::OR:
			op_or(opcode, param);
			break;
		case kenbak1_opcode::AND:
			op_and(opcode, param);
			break;
		case kenbak1_opcode::LNEG:
			op_lneg(opcode, param);
			break;
		case kenbak1_opcode::JPD:
		case kenbak1_opcode::JPI:
		case kenbak1_opcode::JMD:
		case kenbak1_opcode::JMI:
			op_jump(opcode, param);
			break;
		case kenbak1_opcode::SKP_0:
		case kenbak1_opcode::SKP_1:
			op_skp(opcode, param);
			break;
		case kenbak1_opcode::SET_0:
		case kenbak1_opcode::SET_1:
			op_set(opcode, param);
			break;
		case kenbak1_opcode::SFTL:
		case kenbak1_opcode::SFTR:
		case kenbak1_opcode::ROTL:
		case kenbak1_opcode::ROTR:
			op_sft_rot(opcode);
			break;
		case kenbak1_opcode::NOOP:
			// Do nothing
			break;
		case kenbak1_opcode::HALT:
			m_halt = true;
			break;
		default:
			// Shouldn't get here
			assert(false);
			break;
	}
}

void kenbak1_cpu_device::op_add(const kenbak1_opcode &opcode, u8 param)
{
	*reg_from_id(opcode.get_reg()) += read_param(opcode.get_param(), param);
	// TODO: Overflow/Carry
}

void kenbak1_cpu_device::op_sub(const kenbak1_opcode &opcode, u8 param)
{
	*reg_from_id(opcode.get_reg()) -= read_param(opcode.get_param(), param);
	// TODO: Overflow/Carry
}

void kenbak1_cpu_device::op_load(const kenbak1_opcode &opcode, u8 param)
{
	*reg_from_id(opcode.get_reg()) = read_param(opcode.get_param(), param);
}

void kenbak1_cpu_device::op_store(const kenbak1_opcode &opcode, u8 param)
{
	write_param(opcode.get_param(), param, *reg_from_id(opcode.get_reg()));
}

void kenbak1_cpu_device::op_or(const kenbak1_opcode &opcode, u8 param)
{
	*reg_from_id(opcode.get_reg()) |= read_param(opcode.get_param(), param);
}

void kenbak1_cpu_device::op_and(const kenbak1_opcode &opcode, u8 param)
{
	*reg_from_id(opcode.get_reg()) &= read_param(opcode.get_param(), param);
}

void kenbak1_cpu_device::op_lneg(const kenbak1_opcode &opcode, u8 param)
{
	s8 val = read_param(opcode.get_param(), param);
	if (val == -128) {
		// Overflow condition, however it does not alter OCA
		*reg_from_id(opcode.get_reg()) = val;
	}
	else {
		*reg_from_id(opcode.get_reg()) = -val;
	}
}

void kenbak1_cpu_device::op_jump(const kenbak1_opcode &opcode, u8 param)
{
	bool cond = false;
	bool mark = opcode.is_jump_mark();

	switch (opcode.get_cond()) {
		case kenbak1_opcode::COND_NONE:
			cond = true;
			break;
		case kenbak1_opcode::NZERO:
			cond = *reg_from_id(opcode.get_reg()) != 0;
			break;
		case kenbak1_opcode::ZERO:
			cond = *reg_from_id(opcode.get_reg()) == 0;
			break;
		case kenbak1_opcode::NEG:
			cond = (s8)*reg_from_id(opcode.get_reg()) < 0;
			break;
		case kenbak1_opcode::POS:
			cond = (s8)*reg_from_id(opcode.get_reg()) >= 0;
			break;
		case kenbak1_opcode::POS_NZ:
			cond = (s8)*reg_from_id(opcode.get_reg()) > 0;
			break;
		default:
			// Shouldn't get here
			assert(false);
			break;
	}

	if (cond) {
		// Addressing modes for jump have a slightly different connotation since
		// MEMORY is the literal destination and INDIRECT is address to read the destination.
		u8 jump_to = (opcode.get_param() == kenbak1_opcode::INDIRECT) ? m_program.read_byte(param) : param;

		if (mark) {
			m_program.write_byte(jump_to, m_P);
			jump_to++;
		}

		m_P = jump_to;
	}
}

void kenbak1_cpu_device::op_skp(const kenbak1_opcode &opcode, u8 param)
{
	bool skip_if = opcode.get_func() == kenbak1_opcode::SKP_1;
	bool pos = opcode.get_bitpos();
	u8 val = m_program.read_byte(param);

	if (skip_if == !!(val & (1 << pos))) {
		m_P += 2;
	}
}

void kenbak1_cpu_device::op_set(const kenbak1_opcode &opcode, u8 param)
{
	bool set_to = opcode.get_func() == kenbak1_opcode::SET_1;
	bool pos = opcode.get_bitpos();
	u8 val = m_program.read_byte(param);

	if (set_to) {
		val |= 1 << pos;
	}
	else { // clear
		val &= ~(1 << pos);
	}

	m_program.write_byte(param, val);
}

void kenbak1_cpu_device::op_sft_rot(const kenbak1_opcode &opcode)
{
	u8 places = opcode.get_bitpos();
	u8* reg = reg_from_id(opcode.get_reg());
	assert(reg);

	switch (opcode.get_func()) {
		case kenbak1_opcode::SFTL:
			*reg = *reg << places;
			break;
		case kenbak1_opcode::SFTR:
			// Right shift performs sign extension
			*reg = ((signed char)*reg) / 2;
			break;
		case kenbak1_opcode::ROTL:
			*reg = (*reg << places) | (*reg >> (8 - places));
			break;
		case kenbak1_opcode::ROTR:
			*reg = (*reg >> places) | (*reg << (8 - places));
			break;
		default:
			// Shouldn't get here
			assert(false);
			break;
	}
}
