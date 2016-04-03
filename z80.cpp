/*
 * z80.cpp
 *
 *  Created on: 19 mar 2016
 *      Author: wpk
 */


#include "z80.h"
#include <iostream>
#include <inttypes.h>
constexpr bool z80::parity[];

z80::z80(BusHandler& bh) : bh(bh) {
	reset();
}

void z80::reset() {
	pc = 0;
	imode = 0;
	iff1 = false;
	iff2 = false;
	i = 0;
	r = 0;
	ei_waiting = false;
}

uint32_t z80::readstate(std::istream& in) {
	// AF BC DE HL AF' BC' DE' HL' IX IY SP PC
	uint16_t af;
	in >>  std::hex >> af;
	a = af >> 8;
	f = af & 0xff;
	in >> std::hex >> bc;
	in >> std::hex >> de;
	in >> std::hex >> hl;
	in >> std::hex >> af;
	ap = af >> 8;
	fp = af & 0xff;
	in >> std::hex >> bcp;
	in >> std::hex >> dep;
	in >> std::hex >> hlp;
	in >> std::hex >> ix;
	in >> std::hex >> iy;
	in >> std::hex >> sp;
	in >> std::hex >> pc;
	// I R IFF1 IFF2 IM <halted> <tstates>
	uint16_t v;
	in >> std::hex >> v;
	i = v;
	in >> std::hex >> v;
	r = v;
	in >> iff1;
	in >> iff2;
	in >> v;
	imode = v;
	in >> in_halt;
	uint32_t tst;
	in >> std::dec >>  tst;
	return tst;
}

void z80::writestate(std::ostream& out) {
	char p[1024];
	snprintf(p, 1024, "%02x%02x %04x %04x %04x %02x%02x %04x %04x %04x %04x %04x %04x %04x\n%02x %02x %d %d %d %d %d\n",
		 a,f,bc,de,hl,ap, fp, bcp,dep, hlp, ix, iy, sp, pc,
		 i, r, iff1, iff2, imode, in_halt, cycles);
	out << p;
}

uint64_t z80::tick() {
	if (in_halt) {
		cycles +=4;
		return cycles;
	}
	instrv = bh.readmem(pc);
	bool ei_was_waiting = ei_waiting;
	ei_waiting = false;
	instr = instrv & 0xff;
	cycles += 4; // default and always
	pc++;
	r++;
	(this->*ops[instr])();
	if (ei_was_waiting) {
		iff1 = true;
		iff2 = true;
	}
	return cycles;
}

char* z80::get_trace() {
	static char out[200];
	uint32_t op = bh.readmem(pc); // TODO what about gameboy? IO-mapped PC?
	uint32_t stk = bh.readmem(sp);
	snprintf(out, 200, "%.16" PRIu64 " PC=%04x A=%02x F=%d%d%d%d%d%d I=%02x IFF=%d%d IM=%d BC=%04x DE=%04x HL=%04x AF'=%02x%02x BC'=%04x DE'=%04x HL'=%04x IX=%04x IY=%04x SP=%04x (SP)=%02x%02x%02x%02x OPC=%02x%02x%02x%02x\n",
		 cycles, pc, a,
		 ((f&fS) ? 1 : 0), ((f&fZ) ? 1 : 0), ((f&fH) ? 1 : 0),
		 ((f&fPV) ? 1 : 0), ((f&fN) ? 1 : 0), ((f&fC) ? 1 : 0),
		 i, iff1, iff2, imode, bc, de, hl, ap, fp, bcp, dep, hlp, ix, iy, sp,
		 stk & 0xff, (stk >> 8) & 0xff, (stk >> 16) & 0xff, (stk >> 24) & 0xff,
		 op & 0xff, (op >> 8) & 0xff, (op >> 16) & 0xff, (op >> 24) & 0xff
		 );
	return out;
}

bool z80::interrupt() {
	if (iff1) {
		// push pc
		sp--;
		bh.writemem(sp, pc >> 8);
		sp--;
		bh.writemem(sp, pc & 0xff);
		// set new PC
		switch (imode) {
		case 0:
			throw IllOp(0); // not supported, nobody uses it
		case 1:
			pc = 0x38;
			break;
		case 2: {
			uint16_t addr = i << 8; // TODO read bus? I don't think so...
			pc = bh.readmem(addr);
			break;
		}
		default:
			throw IllOp(pc);
		}
		in_halt = false;
		return true;
	} else {
		return false;
	}
}

uint8_t* z80::regaddr(uint8_t r) {
	r = r & 0b111;
	static uint8_t zero;
	zero = 0;
	switch (r) {
	case 0b000:
		return ((uint8_t*) &bc) + 1 ;
	case 0b001:
		return ((uint8_t*) &bc);
	case 0b010:
		return ((uint8_t*) &de) + 1;
	case 0b011:
		return ((uint8_t*) &de);
	case 0b100:
		return ((uint8_t*) &hl) + 1;
	case 0b101:
		return ((uint8_t*) &hl);
	case 0b110:
		return &zero; // trick for out (c),0 and in (c)
	case 0b111:
		return &a;
	}
	throw IllOp(pc);
}

uint16_t * z80::regaddr16(uint8_t r) {
	r = r & 0b11;
	switch (r) {
	case 0b00:
		return &bc;
	case 0b01:
		return &de;
	case 0b10:
		return &hl;
	case 0b11:
		return &sp;
	}
	throw IllOp(pc);
}

void z80::i_setfSZ8(uint8_t v) {
	f &= ~(fZ | fS | fF5 | fF3);
	if (v > 0x7f) {
		f |= fS;
	}
	if (v == 0) {
		f |= fZ;
	}
	if (v & fF3) {
		f |= fF3;
	}
	if (v & fF5) {
		f |= fF5;
	}
}
void z80::i_setfP8(uint8_t v) {
	if (parity[v]) {
		f |= fPV;
	}

}
uint8_t z80::i_add8(uint8_t x, uint8_t y, bool carry) {
	uint8_t oc = ((carry && (f & fC)) ? 1 : 0);
	uint16_t ans = x + y + oc;
	f = 0;
	i_setfSZ8(ans);
	if (ans > 0xff) {
		f |= fC;
	}
	if (((x ^ y) & 0x80) == 0 && ((x ^ ans) & 0x80) != 0) {
		f |= fPV;
	}
	if (((x & 0xf) + (y & 0xf) + oc) & 0x10) {
		f |= fH;
	}
	return ans;
};
uint8_t z80::i_sub8(uint8_t x, uint8_t y, bool carry) {
	uint8_t occ = f&fC;
	uint8_t oc = ((carry && occ) ? 1 : 0);
	uint16_t ans = x - y - oc;
	f = fN; // subtraction
	i_setfSZ8(ans);
	if (ans > 0xff) {
		f |= fC;
	}
	if (((x ^ y) & 0x80) != 0 && ((x ^ ans) & 0x80) != 0) {
		f |= fPV;
	}
	if (((x & 0xf) - (y & 0xf) - oc) & 0x10) {
		f |= fH;
	}
	return ans;
};

uint8_t z80::i_and8(uint8_t x, uint8_t y) {
	uint8_t ans = x & y;
	f = fH;
	i_setfSZ8(ans);
	i_setfP8(ans);
	return ans;
};
uint8_t z80::i_or8(uint8_t x, uint8_t y) {
	uint8_t ans = x | y;
	f = 0;
	i_setfSZ8(ans);
	i_setfP8(ans);
	return ans;
};
uint8_t z80::i_xor8(uint8_t x, uint8_t y) {
	uint8_t ans = x ^ y;
	f = 0;
	i_setfSZ8(ans);
	i_setfP8(ans);
	return ans;
};

uint8_t z80::i_inc8(uint8_t x) {
	uint8_t ans = x+1;
	f = f & fC; // unchanged
	i_setfSZ8(ans);
	if (x == 0x7f) {
		f |= fPV;
	}
	if ((x & 0x8) && !(ans & 0x8)) {
		f |= fH;
	}
	return ans;
};

uint8_t z80::i_dec8(uint8_t x) {
	uint8_t ans = x-1;
	f = f & fC; // unchanged
	i_setfSZ8(ans);
	f |= fN;
	if (x == 0x80) {
		f |= fPV;
	}
	if (!(x & 0x4) && (ans & 0x4)) {
		f |= fH;
	}
	return ans;
};
uint16_t z80::i_add16(uint16_t x, uint16_t y, bool carry) {

	uint8_t prevF = f;
	if (carry) {
		prevF &= fPV;
	} else {
		prevF &= (fZ | fS | fPV);
	}
	uint8_t low = i_add8(x & 0xff, y & 0xff, carry);
	uint8_t hi = i_add8(x >> 8, y >> 8, true);
	uint32_t res = (hi << 8) | low;
	f &= ~(fZ | fS | fPV);
	f |= prevF;
	if (carry) {
		if (res > 0x7fff) {
			f |= fS;
		}
		if (res == 0) {
			f |= fZ;
		}
	}
	return res;
}

uint16_t z80::i_sub16(uint16_t x, uint16_t y, bool carry) {
	uint8_t oc = ((carry && (f & fC)) ? 1 : 0);
	uint32_t res = x - y - oc;
	f = fN;
	if (res > 0xffff) {
		f |= fC;
	}
	if (((x & 0xfff) - (y & 0xfff) - oc) & 0x1000) {
		f |= fH;
	}
	// TODO overflow! - fPV
	if (res == 0) {
		f |= fZ;
	}
	if (res > 0x7fff) {
		f |= fS;
	}
	return res;
}

bool z80::i_checkcc() {
	uint8_t cc = (instr >> 3) & 0x7;
	uint8_t flag = cc >> 1;
	bool b = ((cc & 1) == 0);
	switch (flag) {
	case 0b00:
		return (bool)(f & fZ) ^ b;
	case 0b01:
		return (bool)(f & fC) ^ b;
	case 0b10:
		return (bool)(f & fPV) ^ b;
	case 0b11:
		return (bool)(f & fS) ^ b;
	default:
		throw IllOp(pc);

	}
}

uint8_t z80::i_rot(uint8_t v, bool right, bool c, bool update_flags) {
	// t f f
	uint8_t oldc = f & fC ? 1 : 0;
	if (update_flags) {
		f = 0;
	} else {
		f = f & (fS | fZ | fPV);
	}

	if (v & (right ? 0x1 : 0x80)) {
		f |= fC;
	}
	if (right) {
		v >>= 1;
	} else {
		v <<= 1;
	}
	oldc = c ? ( (f & fC) ? 1 : 0 ) : oldc;
	if (oldc) {
		if (right) {
			v+=0x80;
		} else {
			v+=1;
		}
	}
	if (update_flags) {
		i_setfSZ8(v);
		i_setfP8(v);
	}
	f &= ~(fF3 | fF5);
	f |= v & (fF3 | fF5);
	return v;
}

void z80::op_nop() {
}

void z80::op_halt() {
	in_halt = true;
}

void z80::op_ldrr() {
	uint8_t *r1 = regaddr(instr>>3);
	uint8_t *r2 = regaddr(instr);
	*r1 = *r2;
}

void z80::op_ldrn() {
	uint8_t *r = regaddr(instr>>3);
	*r = (uint8_t) (instrv >> 8);
	pc++;
	cycles += 3;
}

void z80::op_ldrm() {
	uint8_t *r = regaddr(instr>>3);
	*r = bh.readmem(hl);
	cycles += 3;
}

void z80::op_ldmr() {
	uint8_t *r = regaddr(instr);
	bh.writemem(hl, *r);
	cycles += 3;
}

void z80::op_ldmn() {
	bh.writemem(hl, (uint8_t) (instrv >> 8));
	pc++;
	cycles += 6;
}

void z80::op_ldarrm() {
	uint16_t *r = regaddr16(instr >> 4);
	a = bh.readmem(*r);
	cycles += 3;
}

void z80::op_ldannm() {
	a = bh.readmem((instrv >> 8) & 0xffff);
	pc += 2;
	cycles += 9;
}

void z80::op_ldrrma() {
	uint16_t *r = regaddr16(instr >> 4);
	bh.writemem(*r, a);
	cycles += 3;
}

void z80::op_ldnnma() {
	bh.writemem((instrv>>8) & 0xffff, a);
	pc += 2;
	cycles += 9;
}

void z80::op_ldrrnn() {
	uint16_t *r = regaddr16(instr >> 4);
	*r = (instrv >> 8) & 0xffff;
	pc += 2;
	cycles += 6;
}

void z80::op_ldhlnnm() {
	hl = bh.readmem((instrv >> 8) & 0xffff);
	pc += 2;
	cycles += 12;
}

void z80::op_ldnnmhl() {
	bh.writemem(((instrv>>8) & 0xffff), hl & 0xff);
	bh.writemem(((instrv>>8) & 0xffff)+1, hl >> 8);
	pc += 2;
	cycles += 12;
}

void z80::op_ldsphl() {
	sp = hl;
	cycles += 2;
}

void z80::op_pushrr() {
	uint16_t *r = regaddr16(instr>>4);
	sp--;
	bh.writemem(sp, (*r) >> 8);
	sp--;
	bh.writemem(sp, (*r) & 0xff);
	cycles += 7;
}

void z80::op_poprr() {
	uint16_t *r = regaddr16(instr>>4);
	*r = bh.readmem(sp);
	sp += 2;
	cycles += 6;
}

void z80::op_pushaf() {
	sp--;
	bh.writemem(sp, a);
	sp--;
	bh.writemem(sp, f);
	cycles += 7;
}

void z80::op_popaf() {
	uint32_t v = bh.readmem(sp);
	f = v;
	a = v >> 8;
	sp += 2;
	cycles += 6;
}

void z80::op_exdehl() {
	hl ^= de;
	de ^= hl;
	hl ^= de;
}

void z80::op_exafaf() {
	ap ^= a;
	a ^= ap;
	ap ^= a;
	fp ^= f;
	f ^= fp;
	fp ^= f;
}

void z80::op_exx() {
	bcp ^= bc;
	bc ^= bcp;
	bcp ^= bc;

	dep ^= de;
	de ^= dep;
	dep ^= de;

	hlp ^= hl;
	hl ^= hlp;
	hlp ^= hl;
}

void z80::op_exsphl() {
	uint16_t tmp = bh.readmem(sp);
	bh.writemem(sp, hl & 0xff);
	bh.writemem(sp+1, hl >> 8);
	hl = tmp;
	cycles += 15;
}

void z80::op_addr() {
	uint8_t *r = regaddr(instr);
	a = i_add8(a, *r, instr & 0x8);
}

void z80::op_addn() {
	a = i_add8(a, (instrv >> 8) & 0xff, instr & 0x8);
	pc++;
	cycles+=3;
}

void z80::op_addm() {
	a = i_add8(a, bh.readmem(hl), instr & 0x8);
	cycles+=3;
}

void z80::op_subr() {
	uint8_t *r = regaddr(instr);
	a = i_sub8(a, *r, instr & 0x8);
}

void z80::op_subn() {
	a = i_sub8(a, (instrv >> 8) & 0xff, instr & 0x8);
	pc++;
	cycles+=3;
}

void z80::op_subm() {
	a = i_sub8(a, bh.readmem(hl), instr & 0x8);
	cycles+=3;
}

void z80::op_andr() {
	uint8_t *r = regaddr(instr);
	a = i_and8(a, *r);
}

void z80::op_andn() {
	a = i_and8(a, (instrv >> 8) & 0xff);
	pc++;
	cycles+=3;
}

void z80::op_andm() {
	a = i_and8(a, bh.readmem(hl));
	cycles+=3;
}

void z80::op_xorr() {
	uint8_t *r = regaddr(instr);
	a = i_xor8(a, *r);
}

void z80::op_xorn() {
	a = i_xor8(a, (instrv >> 8) & 0xff);
	pc++;
	cycles+=3;
}

void z80::op_xorm() {
	a = i_xor8(a, bh.readmem(hl));
	cycles+=3;
}

void z80::op_orr() {
	uint8_t *r = regaddr(instr);
	a = i_or8(a, *r);
}

void z80::op_orn() {
	a = i_or8(a, (instrv >> 8) & 0xff);
	pc++;
	cycles+=3;
}

void z80::op_orm() {
	a = i_or8(a, bh.readmem(hl));
	cycles+=3;
}

void z80::op_cpr() {
	uint8_t *r = regaddr(instr);
	i_sub8(a, *r, false);
	f &= ~(fF3 | fF5);
	f |= *r & (fF3 | fF5);
}

void z80::op_cpn() {
	i_sub8(a, (instrv >> 8) & 0xff, false);
	f &= ~(fF3 | fF5);
	f |= (instrv >> 8) & (fF3 | fF5);
	pc++;
	cycles+=3;
}

void z80::op_cpm() {
	uint8_t v = bh.readmem(hl);
	i_sub8(a, bh.readmem(hl), false);
	f &= ~(fF3 | fF5);
	f |= v & (fF3 | fF5);
	cycles+=3;
}

void z80::op_incr() {
	uint8_t *r = regaddr(instr>>3);
	*r = i_inc8(*r);
}

void z80::op_incm() {
	cycles += 7;
	uint8_t t = bh.readmem(hl);
	t = i_inc8(t);
	bh.writemem(hl, t);
}

void z80::op_decr() {
	uint8_t *r = regaddr(instr>>3);
	*r = i_dec8(*r);
}

void z80::op_decm() {
	cycles += 7;
	uint8_t t = bh.readmem(hl);
	t = i_dec8(t);
	bh.writemem(hl, t);
}

void z80::op_daa() {
//	N . C . high   . H . low    . # added . C after
//	  .   . nibble .   . nibble . to A    . execution
//	.................................................
//	add, adc, inc operands:
//	0   0   0~9      0   0~9      00        0
//	    0   0~8      0   a~f      06        0
//	    0   0~9      1   0~3      06        0
//	    0   a~f      0   0~9      60        1
//	    0   9~f      0    a~f      66        1
//	    0   a~f      1   0~3      66        1
//	    1   0~2      0   0~9      60        1
//	    1   0~2      0   a~f      66        1
//	    1   0~3      1   0~3      66        1
//	.................................................
//	sub, sbc, dec, neg operands:
//	1   0   0~9      0   0~9      00        0
//	    0   0~8      1   6~f      fa        0
//	    1   7~f      0   0~9      a0        1
//	    1   6~f      1   6~f      9a        1



	bool prevc = f & fC;
	bool prevh = f & fH;
	bool prevn = f & fN;
	uint8_t lo = a & 0xf;
	uint8_t hi = a >> 4;
	f = f & fN;

	uint8_t diff = 0;

	if (!prevc && hi < 10 && !prevh && lo < 10) {
		diff = 0;
	} else if (!prevc && hi < 10 && prevh && lo < 10) {
		diff = 0x06;
	} else if (!prevc && hi < 9 && lo > 9) {
		diff = 0x06;
	} else if (!prevc && hi > 9 && !prevh && lo < 10) {
		diff = 0x60;
	} else if (prevc && !prevh && lo < 10) {
		diff = 0x60;
	} else {
		diff = 0x66;
	}

	if ((prevc) || (hi > 8 && lo > 9) || (hi > 9)) {
		f |= fC;
	}
	if ((!prevn && lo > 9) || (prevn && prevh && lo < 6)) {
		f |= fH;
	}
	if (prevn) {
		a -= diff;
	} else {
		a += diff;
	}

	f |= (a & (fF3 | fF5));
}

void z80::op_addhlrr() {
	hl = i_add16(hl, *regaddr16(instr >> 4), false);
	cycles += 7;
}

void z80::op_incrr() {
	(*regaddr16(instr >> 4))++;
	cycles += 2;
}

void z80::op_decrr() {
	(*regaddr16(instr >> 4))--;
	cycles +=2;
}


void z80::op_ret() {
	pc = bh.readmem(sp);
	sp += 2;
	cycles+=6;
}

void z80::op_retcc() {
	cycles++; // 11 or 5 cycles
	if (i_checkcc()) {
		op_ret();
	}
}

void z80::op_call() {
	pc+=2;
	sp--;
	bh.writemem(sp, pc >> 8);
	sp--;
	bh.writemem(sp, pc & 0xff);
	pc = (instrv >> 8) & 0xffff;
	cycles += 13;
}
void z80::op_callcc() {
	if (i_checkcc()) {
		op_call();
	} else {
		pc += 2;
		cycles += 6;
	}
}

void z80::op_rst() {
	sp--;
	bh.writemem(sp, pc >> 8);
	sp--;
	bh.writemem(sp, pc & 0xff);
	pc = instr & 0b00111000;
	cycles += 7;
}

void z80::op_jr() {
	pc++;
	int8_t r = (instrv >> 8);
	pc += r;
	cycles += 8;
}

void z80::op_jrc() {
	if (!((bool)(f & fC) ^ (bool)(instr & 0x8))) {
		op_jr();
	} else {
		pc++;
		cycles +=3;
	}
}

void z80::op_jrz() {
	if (!((bool)(f & fZ) ^ (bool)(instr & 0x8))) {
		op_jr();
	} else {
		pc++;
		cycles +=3;
	}
}

void z80::op_djnz() {
	pc++;
	// dec 'b' in 'bc'
	uint8_t newb =((bc >> 8)-1);
	bc = (newb << 8) | (bc & 0xff);
	if (newb != 0) {
		pc += (int8_t)(instrv >> 8); // signed!
		cycles += 9;
	} else {
		cycles += 4;
	}
}

void z80::op_jpnn() {
	pc = (instrv >> 8) & 0xffff;
	cycles += 6;
}

void z80::op_jpccnn() {
	if (i_checkcc()) {
		pc = (instrv >> 8) & 0xffff;
	} else {
		pc += 2;
	}
	cycles += 6;
}

void z80::op_jphlm() {
	pc = hl;
}

void z80::op_ccf() {
	f &= ~(fH | fF3 | fF5 | fN);
	f ^= fC;
	if (!(f & fC)) {
		f |= fH;
	}
	f |= (a & (fF3 | fF5));
}

void z80::op_scf() {
	f &= ~(fH | fN | fF3 | fF5);
	f |= fC;
	f |= (a & (fF3 | fF5));
}

void z80::op_cpl() {
	a ^= 0xff;
	f |= fH;
	f |= fN;
	f |= (a & (fF3 | fF5));
}

void z80::op_di() {
	iff1 = false;
	iff2 = false;
}

void z80::op_ei() {
	// interrupts are enabled one instruction -after- ei
	ei_waiting = true;
}

void z80::op_outn() {
	pc++;
	bh.writeio((a << 8) | ( (instrv >> 8) & 0xff), a);
	cycles += 7;
}

void z80::op_inn() {
	pc++;
	a = bh.readio((a << 8) | ((instrv >> 8) & 0xff));
	cycles += 7;
}

void z80::op_rlca() {
	a = i_rot(a, false, true, false);
}

void z80::op_rrca() {
	a = i_rot(a, true, true, false);
}

void z80::op_rla() {
	a = i_rot(a, false, false, false);
}

void z80::op_rra() {
	a = i_rot(a, true, false, false);
}

void z80::op_BITS() {
	cycles += 4;
	pc++;
	r++;
	uint8_t op = instrv >> 8;
	uint8_t hop = op >> 4;

	if ((hop & 0b1110) == 0b0000) {
		if ((op & 0b111) == 0b110) {
			cycles += 7;
			op_bits_rotm(op & 0b1000, !(hop & 1), hl);
		} else {
			op_bits_rotr(op & 0b1000, !(hop & 1), op);
		}
	} else if ((hop & 0b1110) == 0b0010) {
		if ((op & 0b111) == 0b110) {
			cycles += 7;
			op_bits_shtm(op & 0b1000, hop & 1, hl);
		} else {
			op_bits_shtr(op & 0b1000, hop & 1, op);
		}
	} else if ((hop & 0b1100) == 0b0100) {
		if ((op & 0b0111) == 0b0110) {
			cycles += 4;
			i_bits_bitm(op, hl);
		} else {
			i_bits_bitr(op);
		}
	} else if ((hop & 0b1000) == 0b1000) {
		if ((op & 0b0111) == 0b0110) {
			cycles += 7;
			i_bits_setresm(op, hl);
		} else {
			i_bits_setresr(op);
		}
	}
}

void z80::op_bits_rotm(bool right, bool c, uint16_t addr) {
	uint8_t v = bh.readmem(addr);
	v = i_rot(v, right, c, true);
	bh.writemem(addr, v);
}

void z80::op_bits_rotr(bool right, bool c, uint8_t r) {
	uint8_t *v = regaddr(r);
	*v = i_rot(*v, right, c, true);
}

void z80::op_bits_shtm(bool right, bool c, uint16_t addr) {
	uint8_t v = bh.readmem(addr);
	f = 0;
	if (right) {
		if (v & 1) {
			f |= fC;
		}
		v >>=1;
		if (!c) {
			v |= (v<<1) & 0x80;
		}
	} else {
		if (v & 0x80) {
			f |= fC;
		}
		v <<=1;
		if (c) {
			v |= 1;
		}
	}
	i_setfSZ8(v);
	i_setfP8(v);
	bh.writemem(addr, v);

}

void z80::op_bits_shtr(bool right, bool c, uint8_t r) {
	uint8_t * v = regaddr(r);
	f = 0;
	if (right) {
		if (*v & 1) {
			f |= fC;
		}
		*v >>=1;
		if (!c) {
			*v |= (*v<<1) & 0x80;
		}
	} else {
		if (*v & 0x80) {
			f |= fC;
		}
		*v <<=1;
		if (c) {
			*v |= 1;
		}
	}
	i_setfSZ8(*v);
	i_setfP8(*v);
}

void z80::i_bits_setresm(uint8_t op, uint16_t addr) {
	uint8_t bit = (op >> 3) & 7;
	uint8_t v = bh.readmem(addr);
	if (op & 0x40) {
		v |= (1 << bit);
	} else {
		v &= ~(1 << bit);
	}
	bh.writemem(addr, v);
}

void z80::i_bits_setresr(uint8_t op) {
	uint8_t bit = (op >> 3) & 7;
	uint8_t * reg = regaddr(op & 7);
	if (op & 0x40) {
		*reg |= (1 << bit);
	} else {
		*reg &= ~(1 << bit);
	}
}

void z80::i_bits_bitm(uint8_t op, uint16_t addr) {
	uint8_t bit = (op >> 3) & 7;
	uint8_t v = bh.readmem(addr);
	f |= fH;
	f &= ~(fN | fZ | fS | fPV);
	// *Z513*0-  PV as Z, S set only if n=7 and b7 of r set
	if (!(v & (1 << bit))) {
		f |= fZ;
		f |= fPV;
	}
	if ((bit == 7) && !(f&fZ)) {
		f |= fS;
	}
	f &= ~(fF3 | fF5);
	f |= (v & (fF3 | fF5));
}

void z80::i_bits_bitr(uint8_t op) {
	uint8_t bit = (op >> 3) & 7;
	uint8_t v = *regaddr(op & 7);
	f |= fH;
	f &= ~(fN | fZ | fS | fPV);
	if (!(v & (1 << bit))) {
		f |= fZ;
		f |= fPV;
	}
	if (bit == 7 && !(f & fZ)) {
		f |= fS;
	}
	f &= ~(fF3 | fF5);
	f |= (v & (fF3 | fF5));
}

void z80::op_EXTD() {
	cycles += 4;
	pc++;
	r++;
	uint8_t op = instrv >> 8;
	if (op < 0x4 || op > 0xbf) {
		throw IllOp(pc); // undefined
	}
	if (op & 0x80) { // ldi, cpi, etc.
		int8_t s = (op & 0b1000) ? -1 : 1; // ldI vs ldD
		bool r = (op & 0b10000);
		switch (op & 0b111) {
		case 0: { // ldi, ldd
			cycles += 8;
			f &= ~(fN | fH | fPV);
			uint8_t v = bh.readmem(hl);
			bh.writemem(de, v);
			de += s;
			hl += s;
			bc--;
			if (r && (bc != 0)) {
				cycles += 5;
				pc -= 2; // loop
			}
			if (bc) {
				f |= fPV;
			}
			v+= a;
			f &= ~(fF3 | fF5);
			if (v & 2) {
				f |= fF5;
			}
			f |= (v & fF3);
			break;
		}
		case 1: {// cpi, cpd
			cycles +=8;
			uint8_t v = bh.readmem(hl);
			uint8_t c = f&fC;
			uint8_t h = i_sub8(a, v, false);
			hl+=s;
			bc--;
			if (r && (bc != 0 && h != 0)) {
				cycles += 5;
				pc -= 2; // loop
			}
			f &= ~fPV;
			if (bc) {
				f |= fPV;
			}
			if (f & fH) {
				h--;
			}
			f |= fN;
			f &= ~(fF3 | fF5 | fC);
			if (h & 2) {
				f |= fF5;
			}
			f |= (h & fF3);
			f |= c;
			break;
		}
		case 2: { // ini, ind, inir...
			cycles += 8;
			uint8_t v = bh.readio(bc & 0xff);
			bh.writemem(hl, v);
			hl += s;
			bc = (( (bc >> 8) -1) << 8) | (bc & 0xff);
			if (r && ((bc >> 8) != 0)) {
				cycles += 5;
				pc -= 2;
			}
			break;
		}
		case 3: { // outi, outd, otir
			cycles +=8;
			uint8_t v = bh.readmem(hl);
			bh.writeio(bc & 0xff, v);
			hl += s;
			bc = (( (bc >> 8) -1) << 8) | (bc & 0xff);
			if (r && ((bc >> 8) != 0)) {
				cycles += 5;
				pc -= 2;
			}
			break;
		}
		default:
			throw IllOp(pc);

		}
	} else {
		switch (op & 0xf) {
		case 0x0:
		case 0x8: {// in r,(c)
			cycles += 4;
			uint8_t * v = regaddr(op >> 3);
			*v = bh.readio(bc);
			f &= ~(fN | fH);
			i_setfSZ8(*v);
			i_setfP8(*v);
			break;
		}
		case 0x1:
		case 0x9: { // out (c), r
			cycles += 4;
			uint8_t * v = regaddr(op >> 3);
			bh.writeio(bc, *v);
			break;
		}
		case 0x2: { // sbc hl, rr
			cycles += 7;
			uint16_t *v = regaddr16(op >> 4);
			hl = i_sub16(hl, *v, true);
			break;
		}
		case 0xa: { // adc hl, rr
			cycles += 7;
			uint16_t * v = regaddr16(op >> 4);
			hl = i_add16(hl, *v, true);
			break;
		}
		case 0x3: // ld (nn), rr
		{
			uint16_t * r = regaddr16(op >> 4);
			uint16_t addr = instrv >> 16;
			bh.writemem(addr, *r);
			bh.writemem(addr+1, *r >> 8);
			pc +=2;
			cycles +=12;
			break;
		}
		case 0xb: // ld rr, (nn)
		{
			uint16_t *r = regaddr16(op >> 4);
			uint16_t addr = instrv >> 16;
			*r = bh.readmem(addr);
			pc+=2;
			cycles += 12;
			break;
		}

		case 0x4: // neg
		case 0xc: // neg
			a = i_sub8(0, a, false);
			break;
		case 0x5: // retn
		case 0xd: // reti/retn
			if (op != 0x4d) { //retn
				iff1=iff2;
			}
			op_ret();
			break;
		case 0x6: // im 0/1
		case 0xe: // im0/1/2
			if (!(op & 0x10)) {
				imode = 0;
			} else {
				imode = (op & 0b1000) ? 2 : 1;
			}
			break;
		case 0x7: // ld i,a a,i rrd
			switch (op >> 4) {
			case 4:
				cycles++;
				i = a;
				break;
			case 5:
				cycles++;
				a = i;
				f &= fC;
				i_setfSZ8(a);
				if (iff2) {
					f |= fPV;
				}
				break;
			case 6: { /// rrd
//				Reverse of rld
				uint8_t hlx = bh.readmem(hl);
				uint8_t pa = a;
				a &= 0xf0;
				a |= (hlx & 0x0f);
				hlx >>= 4;
				hlx |= (pa<<4);
				i_setfSZ8(a);
				i_setfP8(a);
				bh.writemem(hl, hlx);
				cycles += 10;
				break;
			}
			default:
				throw IllOp(pc);
			}
			break;
		case 0xf: // ld r,a a,r rld
			if (op == 0x4f) {
				cycles++;
				r = a;
				break;
			} else if (op == 0x5f) {
				cycles++;
				a = r;
				f &= fC;
				i_setfSZ8(a);
				break;
			} else if (op == 0x6f) { // rld
//				The contents of the low order four bits (bits 3, 2, 1, and 0) of the memory
//				location (HL) are copied to the high order four bits (7, 6, 5, and 4) of that
//				same memory location; the previous contents of those high order four bits
//				are copied to the low order four bits of the Accumulator (register A); and
//				the previous contents of the low order four bits of the Accumulator are
//				copied to the low order four bits of memory location (HL). The contents of
//				the high order bits of the Accumulator are unaffected.
				uint8_t hlx = bh.readmem(hl);
				uint8_t pa = a;
				a &= 0xf0;
				a |= (hlx >> 4);
				hlx <<= 4;
				hlx |= pa & 0xf;
				i_setfSZ8(a);
				i_setfP8(a);
				bh.writemem(hl, hlx);
				cycles += 10;
				break;
			} else {
				throw IllOp(pc);
			}
		default:
			throw IllOp(pc);
		}
	}
}

void z80::op_II() {
	cycles += 4;
	pc += 1;
	r++;
	uint16_t * iir;
	if (instr & 0x20) {
		iir = &iy;
	} else {
		iir = &ix;
	}
	uint8_t op = (instrv >> 8) & 0xff;
	uint16_t addr = *iir + ((int8_t) (instrv >> 16));
	uint8_t * hr;
	if (op & 0x8) {
		hr = ((uint8_t*) iir);
	} else {
		hr = ((uint8_t*) iir) +1;
	}

	switch (op) {
	// ld iih, iih
	case 0x64:
		break;
	// ld iil, iih:
	case 0x6c:
		*hr = (*iir) >> 8;
		break;
	// ld iih, iil
	case 0x65:
	// ld iil, iil
	case 0x6d: {
		*hr = *iir & 0xff;
		break;
	}

	// just get the register and perform the op, all of those are undocumented
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6a:
	case 0x6b:
	case 0x6f: {
		uint8_t * r = regaddr(op);
		*hr = *r;
		break;
	}

	// ld (ii+*), r
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77: {
		pc++;
		cycles+=11;
		uint8_t *r = regaddr(op);
		bh.writemem(addr, *r);
		break;
	}

	// ld a, iih, undocumented
	case 0x7c: {
		a = *iir >> 8;
		break;
	}
	// ld a, iil, undocumented
	case 0x7d: {
		a = *iir & 0xff;
		break;
	}
	// add ii, bc
	case 0x09: {
		cycles += 7;
		*iir = i_add16(*iir, bc, false);
		break;
	}
	// add ii, de
	case 0x19: {
		cycles += 7;
		*iir = i_add16(*iir, de, false);
		break;
	}
	// add ii, ii
	case 0x29: {
		cycles += 7;
		*iir = i_add16(*iir, *iir, false);
		break;
	}
	// add ii, sp
	case 0x39: {
		cycles += 7;
		*iir = i_add16(*iir, sp, false);
		break;
	}
	// ld ii, **
	case 0x21: {
		*iir = instrv >> 16;
		pc += 2;
		cycles +=6;
		break;
	}
	// ld (**), ii
	case 0x22: {
		bh.writemem(((instrv>>16) & 0xffff), (*iir) & 0xff);
		bh.writemem(((instrv>>16) & 0xffff)+1, (*iir) >> 8);
		pc += 2;
		cycles += 12;
		break;
	}
	// inc ii
	case 0x23: {
		(*iir)++;
		cycles += 2;
		break;
	}
	// dec ii
	case 0x2b: {
		(*iir)--;
		cycles += 2;
		break;
	}
	// ld iih, *
	case 0x26: {
		(*iir) = (((instrv >> 16) << 8) & 0xff00) | ((*iir) & 0xff);
		cycles += 3;
		pc++;
		break;
	}
	// ld ii, (**)
	case 0x2a: {
		*iir = bh.readmem(instrv >> 16);
		pc += 2;
		cycles +=12;
		break;
	}
	// inc (ii+*)
	case 0x34: {
		pc += 1;
		cycles += 15;
		uint8_t v = bh.readmem(addr);
		v = i_inc8(v);
		bh.writemem(addr, v);
		break;
	}
	// dec (ii+*)
	case 0x35: {
		pc += 1;
		cycles += 15;
		uint8_t v = bh.readmem(addr);
		v = i_dec8(v);
		bh.writemem(addr, v);
		break;
	}
	// ld (..), *
	case 0x36:{
		pc += 2;
		cycles += 11;
		bh.writemem(addr, instrv >> 24);
		break;
	}
	// ld b, iih
	case 0x44:
		bc = (*iir & 0xff00) | (bc & 0x00ff);
		break;
	// ld c, iil
	case 0x4d:
		bc = (bc & 0xff00) | (*iir & 0x00ff);
		break;
	// ld d, iih
	case 0x54:
		de = (*iir & 0xff00) | (de & 0x00ff);
		break;
	// ld e, iih
	case 0x5c:
		de = (de & 0xff00) | (*iir >> 8);
		break;
	// ld e, ixl
	case 0x5d:
		de = (de & 0xff00) | (*iir & 0x00ff);
		break;

	// ld r, (ii+a)
	case 0x46:
	case 0x56:
	case 0x66:
	case 0x4e:
	case 0x5e:
	case 0x6e:
	case 0x7e: {
		uint8_t *r = regaddr(op >> 3);
		*r = bh.readmem(addr);
		pc++;
		cycles+=11;
		break;
	}

	// add a, (ii+n)
	case 0x86:
		pc++;
		cycles+=11;
		a = i_add8(a, bh.readmem(addr), false);
		break;
	// sub a, (ii+n)
	case 0x96:
		pc++;
		cycles+=11;
		a = i_sub8(a, bh.readmem(addr), false);
		break;
	// and a, (ii+n)
	case 0xa6:
		pc++;
		cycles+=11;
		a = i_and8(a, bh.readmem(addr));
		break;
	// or a, (ii+n)
	case 0xb6:
		pc++;
		cycles+=11;
		a = i_or8(a, bh.readmem(addr));
		break;
	// adc a, (ii+n)
	case 0x8e:
		pc++;
		cycles+=11;
		a = i_add8(a, bh.readmem(addr), true);
		break;
	 // sbc a, (ii+n)
	case 0x9e:
		pc++;
		cycles+=11;
		a = i_sub8(a, bh.readmem(addr), true);
		break;
	// xor a, (ii+n)
	case 0xae:
		pc++;
		cycles+=11;
		a = i_xor8(a, bh.readmem(addr));
		break;
	// cp a, (ii+n)
	case 0xbe: {
		pc++;
		cycles+=11;
		uint8_t v = bh.readmem(addr);
		i_sub8(a, v, false);
		f &= ~(fF3 | fF5);
		f |= v & (fF3 | fF5);
		break;
	}
	// or iil
	case 0xb5:
		a = i_or8(a, *iir & 0xff);
		break;
	// pop ii
	case 0xe1:
		*iir = bh.readmem(sp);
		sp += 2;
		cycles += 6;
		break;
	// ex (sp), ii
	case 0xe3: {
		uint16_t tmp = bh.readmem(sp);
		bh.writemem(sp, *iir & 0xff);
		bh.writemem(sp+1, *iir >> 8);
		*iir = tmp;
		cycles += 15;
	}
	// push ii
	case 0xe5:
		sp--;
		bh.writemem(sp, (*iir) >> 8);
		sp--;
		bh.writemem(sp, (*iir) & 0xff);
		cycles += 7;
		break;
	// jp (ii)
	case 0xe9:
		pc = *iir;
		break;
	case 0xcb: { // bitops
		pc+=2;
		cycles += 12;
		uint8_t sop = instrv >> 24;
		if ((sop & 0b111) == 0b110) { // rest is undocumented
			switch (sop >> 4) {
			case 0:
				cycles += 3;
				op_bits_rotm(sop & 0x8, true, addr);
				break;
			case 1:
				cycles += 3;
				op_bits_rotm(sop & 0x8, false, addr);
				break;
			case 2:
				cycles += 3;
				op_bits_shtm(sop & 0x8, true, addr);
				break;
			case 3:
				cycles += 3;
				op_bits_shtm(sop & 0x8, false, addr);
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				i_bits_bitm(sop, addr);
				f &= ~(fF3 | fF5);
				f |= ((addr>>8) & (fF3 | fF5));
				break;
			default: // rest
				cycles += 3;
				i_bits_setresm(sop, addr);
			}
		} else {
			throw IllOp(pc);
		}
		break;
	}
	default:
		// let's try doing it differently...
		//throw IllOp(pc);
		// just ignore the prefix and move to the next instruction
		pc -= 1;
		cycles -= 4;
		break;
	}

}
