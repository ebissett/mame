// license:GPL-2.0+
// copyright-holders:Ethan Bissett
/*****************************************************************************
 *
 *   Kenbak-1
 *
 * TODO:
 *   - Front panel functions
 *   - Input lock/unlock switch (blocks Store button)
 *   - CPU accurate timing
 *   - Check ProgRef for accurate state behavior
 *   - Original HW in run state used dim lamps to show OUT if IN is set to non-zero
 *
 * BUGS:
 *   - CPU writes to IN should be blocked?
 *
 * ERRATA:
 *   - Hard reset segfaults.  Stock main does same thing.  QT problem of some kind.
 *
 *****************************************************************************/

#include "emu.h"

#include "cpu/kenbak1/kenbak1.h"
#include "video/pwm.h"

#include "kenbak1.lh"

namespace {

#define LAMP_BITS_MASK         (0x0FF)
#define LAMP_CTRL_INPUT_MASK   (0x100)
#define LAMP_CTRL_ADDRESS_MASK (0x200)
#define LAMP_CTRL_MEMORY_MASK  (0x400)
#define LAMP_CTRL_RUN_MASK     (0x800)

class kenbak1_state : public driver_device
{
public:
	kenbak1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_display(*this, "display")
		, m_data_input(*this, "data")
		, m_ctrl_input(*this, "control")
		, m_lamps(0)
	{}

	void kenbak1(machine_config &config);

	enum buttons {
		BUTTON_BIT0 = 0,
		BUTTON_BIT1,
		BUTTON_BIT2,
		BUTTON_BIT3,
		BUTTON_BIT4,
		BUTTON_BIT5,
		BUTTON_BIT6,
		BUTTON_BIT7,
		BUTTON_CLEAR,
		BUTTON_DISPLAY,
		BUTTON_SET,
		BUTTON_READ,
		BUTTON_STORE,
		BUTTON_START,
		BUTTON_STOP,
	};

	DECLARE_INPUT_CHANGED_MEMBER(push_button);

private:
	void mem_map(address_map &map);
	void update_lamps();
	void set_lamp(u16 mask, bool val);

	// CPU callbacks
	void cpu_update_bit_lamps(u8 data);
	void cpu_update_halt(u8 data);

	required_device<kenbak1_cpu_device> m_maincpu;
	required_device<pwm_display_device> m_display;
	required_ioport m_data_input;
	required_ioport m_ctrl_input;

	u16 m_lamps;
};

void kenbak1_state::mem_map(address_map &map)
{
	map.unmap_value_high();
	map(0x00, 0xff).ram().share("mainram");
}

#define KENBAK_BUTTON(VAL) PORT_CHANGED_MEMBER(DEVICE_SELF, kenbak1_state, push_button, kenbak1_state::VAL)

static INPUT_PORTS_START( kenbak1 )
	PORT_START("data")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )  PORT_NAME("Bit 0")   PORT_CODE(KEYCODE_0)         KENBAK_BUTTON(BUTTON_BIT0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )  PORT_NAME("Bit 1")   PORT_CODE(KEYCODE_1)         KENBAK_BUTTON(BUTTON_BIT1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )  PORT_NAME("Bit 2")   PORT_CODE(KEYCODE_2)         KENBAK_BUTTON(BUTTON_BIT2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )  PORT_NAME("Bit 3")   PORT_CODE(KEYCODE_3)         KENBAK_BUTTON(BUTTON_BIT3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 )  PORT_NAME("Bit 4")   PORT_CODE(KEYCODE_4)         KENBAK_BUTTON(BUTTON_BIT4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 )  PORT_NAME("Bit 5")   PORT_CODE(KEYCODE_5)         KENBAK_BUTTON(BUTTON_BIT5)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 )  PORT_NAME("Bit 6")   PORT_CODE(KEYCODE_6)         KENBAK_BUTTON(BUTTON_BIT6)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON8 )  PORT_NAME("Bit 7")   PORT_CODE(KEYCODE_7)         KENBAK_BUTTON(BUTTON_BIT7)
	PORT_START("control")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 )  PORT_NAME("Clear")   PORT_CODE(KEYCODE_BACKSPACE) KENBAK_BUTTON(BUTTON_CLEAR)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Display") PORT_CODE(KEYCODE_D)         KENBAK_BUTTON(BUTTON_DISPLAY)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON11 ) PORT_NAME("Set")     PORT_CODE(KEYCODE_S)         KENBAK_BUTTON(BUTTON_SET)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Read")    PORT_CODE(KEYCODE_R)         KENBAK_BUTTON(BUTTON_READ)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Store")   PORT_CODE(KEYCODE_ENTER)     KENBAK_BUTTON(BUTTON_STORE)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Start")   PORT_CODE(KEYCODE_SLASH)     KENBAK_BUTTON(BUTTON_START)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON15 ) PORT_NAME("Stop")    PORT_CODE(KEYCODE_STOP)      KENBAK_BUTTON(BUTTON_STOP)
	// TODO: Input lock switch
INPUT_PORTS_END

void kenbak1_state::update_lamps()
{
	m_display->matrix(1, m_lamps);
}

void kenbak1_state::set_lamp(u16 mask, bool val)
{
	if (val) {
		m_lamps &= ~(mask);
	}
	else {
		m_lamps |= mask;
	}
}

void kenbak1_state::cpu_update_bit_lamps(u8 data)
{
	m_lamps &= ~(LAMP_BITS_MASK);
	m_lamps |= data;
	update_lamps();
}

void kenbak1_state::cpu_update_halt(u8 data)
{
	set_lamp(LAMP_CTRL_RUN_MASK, data);
	update_lamps();
}

INPUT_CHANGED_MEMBER(kenbak1_state::push_button)
{
	if (newval) {
		switch (param) {
			case BUTTON_BIT0:
			case BUTTON_BIT1:
			case BUTTON_BIT2:
			case BUTTON_BIT3:
			case BUTTON_BIT4:
			case BUTTON_BIT5:
			case BUTTON_BIT6:
			case BUTTON_BIT7:
				printf("%s:%d: BIT%d\n", __FUNCTION__, __LINE__, param);
				m_maincpu.target()->set_input_bit(param);
				break;

			case BUTTON_CLEAR:
				printf("%s:%d: CLEAR\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->clear_input();
				break;

			case BUTTON_DISPLAY:
				printf("%s:%d: DISPLAY\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->display_address();
				break;

			case BUTTON_SET:
				printf("%s:%d: SET\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->set_address();
				break;

			case BUTTON_READ:
				printf("%s:%d: READ\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->read_value();
				break;

			case BUTTON_STORE:
				printf("%s:%d: STORE\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->store_value();
				break;

			case BUTTON_START:
				printf("%s:%d: START\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->set_halt(false);
				break;

			case BUTTON_STOP:
				printf("%s:%d: STOP\n", __FUNCTION__, __LINE__);
				m_maincpu.target()->set_halt(true);
				break;
		}
	}
}

void kenbak1_state::kenbak1(machine_config &config)
{
	// Basic machine hardware
	KENBAK1(config, m_maincpu, 1_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &kenbak1_state::mem_map);
	m_maincpu->write_out().set(FUNC(kenbak1_state::cpu_update_bit_lamps));
	m_maincpu->write_halt().set(FUNC(kenbak1_state::cpu_update_halt));

	// Front panel
	// Refresh/Interpolation set to defeat PWM behavior (I want flicker)
	PWM_DISPLAY(config, m_display).set_size(1, 12);
	m_display->set_refresh(attotime::from_hz(500));
	m_display->set_interpolation(1);
	config.set_default_layout(layout_kenbak1);
}

// ROM Definitions
ROM_START( kenbak1 )
	// Machine had no ROM
ROM_END

} // anonymous namespace

//    YEAR  NAME     PARENT COMPAT MACHINE  INPUT    CLASS          INIT        COMPANY               FULLNAME    FLAGS
COMP( 1971, kenbak1, 0,     0,     kenbak1, kenbak1, kenbak1_state, empty_init, "Kenbak Corporation", "Kenbak-1", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW | MACHINE_IMPERFECT_TIMING)
