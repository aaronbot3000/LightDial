# MPLAB IDE generated this makefile for use with GNU make.
# Project: 24HJ.mcp
# Date: Thu Dec 01 18:53:12 2011

AS = pic30-as.exe
CC = pic30-gcc.exe
LD = pic30-ld.exe
AR = pic30-ar.exe
HX = pic30-bin2hex.exe
RM = rm

24HJ.hex : 24HJ.cof
	$(HX) "24HJ.cof"

24HJ.cof : lightdial.o
	$(CC) -mcpu=24HJ64GP502 "lightdial.o" -o"24HJ.cof" -Wl,-Tp24HJ64GP502.gld,--defsym=__MPLAB_BUILD=1,-Map="24HJ.map",--report-mem

lightdial.o : ../../../../../../program\ files\ (x86)/microchip/mplab\ c30/include/stddef.h ../../../../../../program\ files\ (x86)/microchip/mplab\ c30/include/string.h ../../../../../../program\ files\ (x86)/microchip/mplab\ c30/include/stdint.h ../../../../../../program\ files\ (x86)/microchip/mplab\ c30/support/PIC24H/h/p24HJ64GP502.h lightdial.c
	$(CC) -mcpu=24HJ64GP502 -x c -c "lightdial.c" -o"lightdial.o" -g -Wall

clean : 
	$(RM) "lightdial.o" "24HJ.cof" "24HJ.hex"

