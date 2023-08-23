#include "FreeRTOS.h"
#include "stdint.h"
#include "stdio.h"
#include "trap.h"
#include "CPM/exbios.h"

uint8_t* z80trap(trapframe_t *frame)
{
	uint8_t *pc = farptr(*frame->pc -1);
#if 1	
	if( *pc++ == CPMEXBIOS && *pc > MINIO && *pc < MAXIO)
		pc = exbioscall(frame);
	else
#endif
	{
		printf("Z80-TRAP Base %02X\n"
			 " AF , BC , DE , HL ,IX IY,PC SP\n"
			 "%04X,%04X,%04X,%04X,%04X ,%04X \n"
			 "%04X,%04X,%04X,%04X,%04X ,%04X \n"
			,(unsigned)pc >> 16
			,frame->af & 0xFFFF	
			,frame->bc & 0xFFFF	
			,frame->de & 0xFFFF	
			,frame->hl & 0xFFFF	
			,frame->ix & 0xFFFF	
			,(unsigned)*frame->pc & 0xFFFF	
			,frame->af_ & 0xFFFF	
			,frame->bc_ & 0xFFFF	
			,frame->de_ & 0xFFFF	
			,frame->hl_ & 0xFFFF	
			,frame->iy & 0xFFFF	
			,(unsigned)frame->sps & 0xFFFF	
		);
		
		asm(" halt");
	}
	return pc;
}

uint8_t* ez80trap(trapframe_t *frame)
{
	printf("eZ80-TRAPd\n"
		 "AF/AF',BC/BC',DE/DE',HL/HL',IX/IY ,SPL/SPS,PC/IFF \n"
		 "%06X,%06X,%06X,%06X,%06X,%06X,%06X \n"
		 "%06X,%06X,%06X,%06X,%06X,%06X,%06X \n"
		,frame->af
		,frame->bc
		,frame->de
		,frame->hl
		,frame->ix
		,(unsigned)frame->spl
		,(unsigned)*frame->pc
		,frame->af_
		,frame->bc_
		,frame->de_
		,frame->hl_
		,frame->iy
		,(unsigned)frame->sps & 0xFFFF
		,(unsigned)frame->iff
	);

	asm(" halt");
	return 0;
}

uint8_t* trap(trapframe_t *frame)
{
	uint8_t trapmode = *frame->spl;
	uint8_t *pret = 0;
	
   	if(trapmode == 2 )
		pret = z80trap(frame);
	else
		pret = ez80trap(frame);
	
	return pret;
}