; HP-PA  __gmpn_lshift --

; Copyright (C) 1992, 1994, 2000 Free Software Foundation, Inc.

; This file is part of the GNU MP Library.

; The GNU MP Library is free software; you can redistribute it and/or modify
; it under the terms of the GNU Library General Public License as published by
; the Free Software Foundation; either version 2 of the License, or (at your
; option) any later version.

; The GNU MP Library is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
; License for more details.

; You should have received a copy of the GNU Library General Public License
; along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
; the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
; MA 02111-1307, USA.


; INPUT PARAMETERS
; res_ptr	gr26
; s_ptr		gr25
; size		gr24
; cnt		gr23

	.code
	.export		__gmpn_lshift
__gmpn_lshift
	.proc
	.callinfo	frame=64,no_calls
	.entry

	sh2add		%r24,%r25,%r25
	sh2add		%r24,%r26,%r26
	ldws,mb		-4(0,%r25),%r22
	subi		32,%r23,%r1
	mtsar		%r1
	addib,=		-1,%r24,L$0004
	vshd		%r0,%r22,%r28		; compute carry out limb
	ldws,mb		-4(0,%r25),%r29
	addib,=		-1,%r24,L$0002
	vshd		%r22,%r29,%r20

L$loop	ldws,mb		-4(0,%r25),%r22
	stws,mb		%r20,-4(0,%r26)
	addib,=		-1,%r24,L$0003
	vshd		%r29,%r22,%r20
	ldws,mb		-4(0,%r25),%r29
	stws,mb		%r20,-4(0,%r26)
	addib,<>	-1,%r24,L$loop
	vshd		%r22,%r29,%r20

L$0002	stws,mb		%r20,-4(0,%r26)
	vshd		%r29,%r0,%r20
	bv		0(%r2)
	stw		%r20,-4(0,%r26)
L$0003	stws,mb		%r20,-4(0,%r26)
L$0004	vshd		%r22,%r0,%r20
	bv		0(%r2)
	stw		%r20,-4(0,%r26)

	.exit
	.procend
