/* Test originally written by refraction */
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <debug.h>

/* Some common types */
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long long u_int64;

/* Registers we need */
#define INTCSTAT (*(volatile u_int*)0x1000F000)
#define INTCMASK (*(volatile u_int*)0x1000F010)
#define T0COUNT (*(volatile u_int*)0x10000000)
#define T0MODE 	(*(volatile u_int*)0x10000010)
#define T1COUNT (*(volatile u_int*)0x10000800)
#define T1MODE 	(*(volatile u_int*)0x10000810)

void start_test()
{
	SetGsCrt(1, 2, 1);
	u_int overflow = 0;
	u_int vsync_on_time = 0;
	u_int vsync_off_time = 0;
	u_int hblank_to_on_vblank = 0;
	u_int hblank_to_off_vblank = 0;
	u_int start_time = 0;
	u_int current_time = 0;
	u_int64 of_count = 0;
	
	INTCMASK &= 0xFFF3;
	INTCSTAT = 0xC;
	T1MODE = 0xC02;
	T0MODE = 0xC03;
	T0COUNT = 0;	
	T1COUNT = 0;
	
	/* Wait for VBLANK END to happen, our start line */
	while (!(INTCSTAT & 0x8)) {}
	
	/* Set timer 1 to BUSCLK / 256 and timer 0 to HBLANK */
	T0COUNT = 0;
	T1COUNT = 0;
	T1MODE = 0x82;
	T0MODE = 0x83;
	/* Clear vblank interrupts. */
	INTCSTAT = 0xC;
	
	/* Wait for VBLANK START */
	while (!(INTCSTAT & 0x4))
	{
		/* The EE timers are 16bit so they may overflow. Check when that happens */
		if (T1MODE & 0x800)
		{
			of_count++;
			T1MODE &= 0x7FF;
		}
	}

	/* Current time is BUSCLK / 256 cycles from vblank end */
	current_time = of_count * 0xffff + T1COUNT;
	/* How many hblanks passed from vblank end? */
	hblank_to_on_vblank = T0COUNT;
	
	/* Store the recorded time */
	vsync_off_time = current_time;
	start_time = current_time;
	
	/* Clear vblank interrupts. */
	INTCSTAT = 0xC;
	of_count = 0;
	
	/* Wait for VB end */
	while (!(INTCSTAT & 0x8))
	{
		if (T1MODE & 0x800)
		{
			of_count++;
			T1MODE &= 0x7FF;
		}
	}
	
	/* Current time is BUSCLK / 256 cycles since the last vblank end interrupt */
	current_time = of_count * 0xffff + T1COUNT;
	/* How many hblanks does vblank last */
	hblank_to_off_vblank = T0COUNT - hblank_to_on_vblank;
	/* How many BUSCLK / 256 cycles does vblank last */
	vsync_on_time = current_time - start_time;
	
	/* (BUSCLK / 256) * 512 = EE cycles */
	vsync_off_time = vsync_off_time * 512;
	vsync_on_time = vsync_on_time * 512;

	/* Print results */
	printf("VSYNC OFF for Approx %d CPU Cycles (give or take 256) %d HSYNCs passed\n", vsync_off_time, hblank_to_on_vblank);
	printf("VSYNC ON for Approx %d CPU Cycles (give or take 256) %d HSYNCs passed\n", vsync_on_time, hblank_to_off_vblank);
	scr_printf("VSYNC OFF for Approx %d CPU Cycles (give or take 256) %d HSYNCs passed\n", vsync_off_time, hblank_to_on_vblank);
	scr_printf("VSYNC ON for Approx %d CPU Cycles (give or take 256) %d HSYNCs passed\n", vsync_on_time, hblank_to_off_vblank);
}

int main()
{
	/* Init the display */ 
	SifInitRpc(0);
	init_scr();

	/* Reset timers and enable vblank interrupts */ 
	T0COUNT = 0;
	T1COUNT = 0;
	INTCMASK &= 0xFFF3; /* Clear vblank interrupts */
	INTCSTAT = 0xC;

	start_test();
	sleep(20);

	return 0;
}