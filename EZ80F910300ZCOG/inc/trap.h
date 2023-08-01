#ifndef _TRAP_H_
#define _TRAP_H_

typedef struct
{
	uint8_t			*sps;
	uint24_t		hl_;
	uint24_t		de_;
	uint24_t		bc_;
	uint24_t		af_;
	uint24_t		iy;
	uint24_t		ix;
	uint8_t			**pc;
	uint24_t		hl;
	uint24_t		de;
	uint24_t		bc;
	uint24_t		iff;
	uint24_t		af;
	uint8_t			*spl;
} trapframe_t;

#endif /* _TRAP_H_ */ 