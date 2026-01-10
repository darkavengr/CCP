;  CCP Version 0.0.1
;    (C) Matthew Boote 2020-2023
;
;    This file is part of CCP.
;
;    CCP is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    CCP is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with CCP.  If not, see <https://www.gnu.org/licenses/>.
;

;
; SCI interrupt handler
;

extern PowerButtonMode

void sci_interrupt_handler:
pusha						; save registers

mov	eax,[PowerButtonMode]			; get power button mode

test	eax,eax					; shutdown
jz	sci_shutdown

cmp	eax,1					; restart
jz	sci_restart

cmp	eax,2					; sleep
jz	sci_sleep

jmp	sci_return

sci_shutdown:
call	ACPIShutdown
jmp	sci_return

sci_restart:
call	ACPIRestart
jmp	sci_return

sci_sleep:
call	ACPISleep
jmp	sci_return

sci_return:
popa
ret

