// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1 CPU
 *
 *****************************************************************************/

#ifndef MAME_CPU_KENBAK1_KENBAK1_H
#define MAME_CPU_KENBAK1_KENBAK1_H

#pragma once

#include "kenbak1types.h"

class kenbak1_opcode;

class kenbak1_cpu_device : public cpu_device {
public:
	// construction/destruction
	kenbak1_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	// front panel callbacks
	auto write_out()  { return m_write_out.bind(); }
	auto write_halt() { return m_write_halt.bind(); }

	// Control functions
	void set_input_bit(u8 bit) { m_IN |= 1 << bit; }
	void clear_input()         { m_IN = 0; }
	u8 get_input()             { return m_IN; }

	void set_address()         { m_address = m_IN; }
	void display_address()     { m_OUT = m_address; }
	void store_value()         { m_program.write_byte(m_address++, m_IN); }
	void read_value()          { m_OUT = m_program.read_byte(m_address); }

	void set_halt(bool halt);
	bool get_halt() { return m_halt; }

protected:
	// construction/destruction
	kenbak1_cpu_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_execute_interface overrides
	virtual bool cpu_is_interruptible() const override { return false; }
	virtual void execute_run() override;

	// device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;

	// device_disasm_interface overrides
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() override;

	// internal register handlers
#define REG_RW_PROTOS(NAME) \
	u8 read_ ## NAME();     \
	void write_ ## NAME(u8 val)

	REG_RW_PROTOS(A);
	REG_RW_PROTOS(B);
	REG_RW_PROTOS(X);
	REG_RW_PROTOS(P);
	REG_RW_PROTOS(OUT);
	REG_RW_PROTOS(OCA);
	REG_RW_PROTOS(OCB);
	REG_RW_PROTOS(OCX);
	REG_RW_PROTOS(IN);

	// execution operations
	u8 fetch();
	void execute_one();

    void set_oc_reg(kenbak1_reg::id reg_id, bool carry, bool overflow);

	void op_add(const kenbak1_opcode &opcode, u8 param);
	void op_sub(const kenbak1_opcode &opcode, u8 param);
	void op_load(const kenbak1_opcode &opcode, u8 param);
	void op_store(const kenbak1_opcode &opcode, u8 param);
	void op_or(const kenbak1_opcode &opcode, u8 param);
	void op_and(const kenbak1_opcode &opcode, u8 param);
	void op_lneg(const kenbak1_opcode &opcode, u8 param);
	void op_jump(const kenbak1_opcode &opcode, u8 param);
	void op_skp(const kenbak1_opcode &opcode, u8 param);
	void op_set(const kenbak1_opcode &opcode, u8 param);
	void op_sft_rot(const kenbak1_opcode &opcode);
	void op_halt();

	// helpers
	u8 *reg_from_id(kenbak1_reg::id id);
	u8 read_param(kenbak1_opcode::param ptype, u8 param);
	void write_param(kenbak1_opcode::param ptype, u8 param, u8 val);

	address_space_config m_program_config;
	int                  m_icount;
	bool                 m_halt;

	u8 m_A, m_B, m_X, m_P, m_OUT, m_OCA, m_OCB, m_OCX, m_IN;

	u8 m_address;

	memory_access<8, 0, 0, ENDIANNESS_LITTLE>::specific m_program;

	devcb_write8 m_write_out;
	devcb_write8 m_write_halt;

private:
	void register_map(address_map &map);
};

DECLARE_DEVICE_TYPE(KENBAK1, kenbak1_cpu_device)

#endif // MAME_CPU_KENBAK1_KENBAK1_H
