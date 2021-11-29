#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define ANSI_CLRS			"\x1b[2J"
#define ANSI_SCUR			"\x1b""7"
#define ANSI_RCUR			"\x1b""8"
#define ANSI_COFF			"\x1b[?25l"
#define ANSI_CON			"\x1b[?25h"
#define ANSI_GXY(x,y)		"\x1b["#y";"#x"f"
#define ANSI_SATT(m,f,b)	"\x1b["#m";"#f";"#b"m"
#define ANSI_NORM			"\x1b[2;37;40m"
#define ANSI_DEOL			"\x1b[K"

BaseType_t initConsole();
int console_putch(int c);
int console_getch();

#endif  /* _CONSOLE_H_ */