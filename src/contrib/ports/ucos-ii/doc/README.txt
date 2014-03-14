                                                  LwIP uCOS-II Port

Date         : 12.01.2005
Version      : 1.1
LwIP source  : From CVS

put together by  Michael Anburaj
Homepage: http://geocities.com/michaelanburaj/
e-mail  : embeddedeng@yahoo.com


Installation : UNZIP to desired folder (<lwip root>\contrib\ports\ is recommended)

This port was tested on following setups:

MIPS Atlas
----------
Processor : MIPS 4Kc/Atlas
uCOS-II   : ver 2.61
LwIP      : From CVS repository
tool-chain: GNU (mipsisa32-elf)
Interfaces: Ethernet (SAA9730, RLTK8139) & WiFi (Prism II chipset)

LPC2106
-------
Processor : ARM7TDMI-S/LPC2106 IAR Kickstart board
uCOS-II   : ver 2.61
LwIP      : From CVS repository
tool-chain: GNU (arm-elf)
Interfaces: Ethernet (CS8900A)


Important!!!

This is a generic port for uC/OS-II. Hence it should run without or minmal change on any processor platform running uC/OS-II.

This port can be modified very easily to make it work on other RTOSes too. Please feel free to get in touch with me for further help.


Cheers,
Michael Anburaj.