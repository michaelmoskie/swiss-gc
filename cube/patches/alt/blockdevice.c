/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"
#include "../base/common.h"
#include "../base/exi.h"
#include "../base/os.h"

bool exi_probe(int32_t chan)
{
	if (chan == EXI_CHANNEL_2)
		return false;
	if (chan == *VAR_EXI_SLOT)
		return false;
	return true;
}

bool exi_trylock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	if (chan == *VAR_EXI_SLOT && dev == EXI_DEVICE_0)
		return false;
	if (chan == *VAR_EXI_SLOT)
		end_read();
	return true;
}

void di_update_interrupts(void);
void di_complete_transfer(void);

void schedule_read(void *address, uint32_t length, uint32_t offset, OSTick ticks)
{
	*(uint32_t *)VAR_LAST_OFFSET = offset;
	*(uint32_t *)VAR_TMP2 = length;
	*(uint8_t **)VAR_TMP1 = address;

	if (length) {
		timer1_start(ticks);
		return;
	}

	di_complete_transfer();
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	schedule_read(OSPhysicalToUncached(address), length, offset, OSMicrosecondsToTicks(300));
}

void trickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		OSTick start = OSGetTick();
		data_size = read_frag(data, remainder, position);
		OSTick end = OSGetTick();

		position  += data_size;
		remainder -= data_size;

		schedule_read(data + data_size, remainder, position, OSDiffTick(end, start));
	}
}

void change_disc(void)
{
	*(uint32_t *)VAR_CURRENT_DISC = !*(uint32_t *)VAR_CURRENT_DISC;

	(*DI_EMU)[1] &= ~0b001;
	(*DI_EMU)[1] |=  0b100;
	di_update_interrupts();
}
