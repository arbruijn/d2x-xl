/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _DPMI_H
#define _DPMI_H

#include "pstypes.h"

typedef struct dpmi_real_regs {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved_by_system;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint16_t flags;
    uint16_t es,ds,fs,gs,ip,cs,sp,ss;
} dpmi_real_regs;


// Initializes dpmi. Returns zero if failed.
extern int32_t dpmi_init(int32_t verbose);
// Returns a pointer to a temporary dos memory block. Size must be < 1024 bytes.
extern void *dpmi_get_temp_low_buffer( int32_t size );
extern void *dpmi_real_malloc( int32_t size, uint16_t *selector );
extern void dpmi_real_free( uint16_t selector );
extern void dpmi_real_int386x( uint8_t intno, dpmi_real_regs * rregs );
extern void dpmi_real_call(dpmi_real_regs * rregs);
extern int32_t dpmi_lock_region(void *address, uint32_t length);
extern int32_t dpmi_unlock_region(void *address, uint32_t length);
// returns 0 if failed...
extern int32_t dpmi_allocate_selector( void * address, int32_t size, uint16_t * selector );
extern int32_t dpmi_modify_selector_base( uint16_t selector, void * address );
extern int32_t dpmi_modify_selector_limit( uint16_t selector, int32_t size  );


#if defined(__GNUC__) || defined(_WIN32)
# define _far
#else
# define _far far
#endif
// Sets the PM handler. Returns 0 if succssful
extern int32_t dpmi_set_pm_handler(uint32_t intnum, void _far * isr );

extern uint32_t dpmi_virtual_memory;
extern uint32_t dpmi_available_memory;
extern uint32_t dpmi_physical_memory;
extern uint32_t dpmi_dos_memory;

#endif
