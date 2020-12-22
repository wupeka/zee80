/*
 * z80.h
 *
 *  Created on: 19 mar 2016
 *      Author: wpk
 */

#ifndef Z80_H_
#define Z80_H_
#include "bushandler.h"
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

struct z80_regs {
  // registers
  uint8_t a;
  uint8_t f;
  //	uint8_t b;
  uint8_t i;
  uint8_t r;
  bool iff1;
  bool iff2;

  uint16_t bc;
  uint16_t de;
  uint16_t hl;

  uint8_t ap;
  uint8_t fp;
  uint16_t bcp;
  uint16_t dep;
  uint16_t hlp;

  uint16_t pc;
  uint16_t sp;
  uint16_t ix;
  uint16_t iy;

  uint8_t imode;
};

class z80 {
public:
  z80(BusHandler &bh);
  void reset();
  uint64_t tick();
  // return true if acked
  bool interrupt(uint8_t data);
  char *get_trace();

  class IllOp : public std::exception {
  public:
    IllOp(uint16_t pc) : pc(pc){};
    uint16_t pc;
    virtual const char *what() const throw() {
      static char x[256];
      snprintf(x, 256, "Illegal opcode @0x%04x", pc);
      return x;
    }
  };
  uint32_t readstate(std::istream &in);
  void writestate(std::ostream &out);
  void addtrap(uint16_t where);
  struct z80_regs get_regs();
  void set_regs(struct z80_regs regs);
  BusHandler &bh;

  static const uint8_t fS = 1 << 7;
  static const uint8_t fZ = 1 << 6;
  static const uint8_t fF5 = 1 << 5;
  static const uint8_t fH = 1 << 4;
  static const uint8_t fF3 = 1 << 3;
  static const uint8_t fPV = 1 << 2;
  static const uint8_t fN = 1 << 1;
  static const uint8_t fC = 1;

// public for traploader
  uint8_t i_sub8(uint8_t a, uint8_t b, bool carry);


private:
  std::vector<uint16_t> traps;

  uint8_t *regaddr(uint8_t r);
  uint16_t *regaddr16(uint8_t r);

  struct z80_regs R;

  bool in_halt = false;
  // used for every tick
  uint64_t cycles = 0; // number of cycles taken
  uint8_t instr;       // current instruction
  uint32_t instrv; // current instruction + 3 next bytes, volatile so make sure
                   // it won't get optimized
  // ei works after the next instruction, sets this flag
  bool ei_waiting;

  typedef void (z80::*op_t)();

  const op_t ops[256] = {
      &z80::op_nop,    &z80::op_ldrrnn,  &z80::op_ldrrma,  &z80::op_incrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_rlca,
      &z80::op_exafaf, &z80::op_addhlrr, &z80::op_ldarrm,  &z80::op_decrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_rrca,
      &z80::op_djnz,   &z80::op_ldrrnn,  &z80::op_ldrrma,  &z80::op_incrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_rla,
      &z80::op_jr,     &z80::op_addhlrr, &z80::op_ldarrm,  &z80::op_decrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_rra,
      &z80::op_jrz,    &z80::op_ldrrnn,  &z80::op_ldnnmhl, &z80::op_incrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_daa,
      &z80::op_jrz,    &z80::op_addhlrr, &z80::op_ldhlnnm, &z80::op_decrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_cpl,
      &z80::op_jrc,    &z80::op_ldrrnn,  &z80::op_ldnnma,  &z80::op_incrr,
      &z80::op_incm,   &z80::op_decm,    &z80::op_ldmn,    &z80::op_scf,
      &z80::op_jrc,    &z80::op_addhlrr, &z80::op_ldannm,  &z80::op_decrr,
      &z80::op_incr,   &z80::op_decr,    &z80::op_ldrn,    &z80::op_ccf,

      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,
      &z80::op_ldmr,   &z80::op_ldmr,    &z80::op_ldmr,    &z80::op_ldmr,
      &z80::op_ldmr,   &z80::op_ldmr,    &z80::op_halt,    &z80::op_ldmr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrr,    &z80::op_ldrr,
      &z80::op_ldrr,   &z80::op_ldrr,    &z80::op_ldrm,    &z80::op_ldrr,

      &z80::op_addr,   &z80::op_addr,    &z80::op_addr,    &z80::op_addr,
      &z80::op_addr,   &z80::op_addr,    &z80::op_addm,    &z80::op_addr,
      &z80::op_addr,   &z80::op_addr,    &z80::op_addr,    &z80::op_addr,
      &z80::op_addr,   &z80::op_addr,    &z80::op_addm,    &z80::op_addr,
      &z80::op_subr,   &z80::op_subr,    &z80::op_subr,    &z80::op_subr,
      &z80::op_subr,   &z80::op_subr,    &z80::op_subm,    &z80::op_subr,
      &z80::op_subr,   &z80::op_subr,    &z80::op_subr,    &z80::op_subr,
      &z80::op_subr,   &z80::op_subr,    &z80::op_subm,    &z80::op_subr,
      &z80::op_andr,   &z80::op_andr,    &z80::op_andr,    &z80::op_andr,
      &z80::op_andr,   &z80::op_andr,    &z80::op_andm,    &z80::op_andr,
      &z80::op_xorr,   &z80::op_xorr,    &z80::op_xorr,    &z80::op_xorr,
      &z80::op_xorr,   &z80::op_xorr,    &z80::op_xorm,    &z80::op_xorr,
      &z80::op_orr,    &z80::op_orr,     &z80::op_orr,     &z80::op_orr,
      &z80::op_orr,    &z80::op_orr,     &z80::op_orm,     &z80::op_orr,
      &z80::op_cpr,    &z80::op_cpr,     &z80::op_cpr,     &z80::op_cpr,
      &z80::op_cpr,    &z80::op_cpr,     &z80::op_cpm,     &z80::op_cpr,

      &z80::op_retcc,  &z80::op_poprr,   &z80::op_jpccnn,  &z80::op_jpnn,
      &z80::op_callcc, &z80::op_pushrr,  &z80::op_addn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_ret,     &z80::op_jpccnn,  &z80::op_BITS,
      &z80::op_callcc, &z80::op_call,    &z80::op_addn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_poprr,   &z80::op_jpccnn,  &z80::op_outn,
      &z80::op_callcc, &z80::op_pushrr,  &z80::op_subn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_exx,     &z80::op_jpccnn,  &z80::op_inn,
      &z80::op_callcc, &z80::op_II,      &z80::op_subn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_poprr,   &z80::op_jpccnn,  &z80::op_exsphl,
      &z80::op_callcc, &z80::op_pushrr,  &z80::op_andn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_jphlm,   &z80::op_jpccnn,  &z80::op_exdehl,
      &z80::op_callcc, &z80::op_EXTD,    &z80::op_xorn,    &z80::op_rst,
      &z80::op_retcc,  &z80::op_popaf,   &z80::op_jpccnn,  &z80::op_di,
      &z80::op_callcc, &z80::op_pushaf,  &z80::op_orn,     &z80::op_rst,
      &z80::op_retcc,  &z80::op_ldsphl,  &z80::op_jpccnn,  &z80::op_ei,
      &z80::op_callcc, &z80::op_II,      &z80::op_cpn,     &z80::op_rst};

  // helper functions
  void i_setfSZ8(uint8_t v);
  void i_setf358(uint8_t v);
  void i_setfP8(uint8_t v);
  uint8_t i_add8(uint8_t a, uint8_t b, bool carry);
// public for traploader
// uint8_t i_sub8(uint8_t a, uint8_t b, bool carry);
  uint8_t i_and8(uint8_t a, uint8_t b);
  uint8_t i_xor8(uint8_t a, uint8_t b);
  uint8_t i_or8(uint8_t a, uint8_t b);
  uint8_t i_inc8(uint8_t a);
  uint8_t i_dec8(uint8_t a);
  uint16_t i_add16(uint16_t a, uint16_t b, bool carry);
  uint16_t i_sub16(uint16_t a, uint16_t b, bool carry);
  bool i_checkcc();
  uint8_t i_rot(uint8_t v, bool right, bool c, bool update_flags);

  // ops
  void op_nop();

  // arithmetics
  void op_addm();
  void op_addn();
  void op_addr();
  void op_subm();
  void op_subn();
  void op_subr();
  void op_andm();
  void op_andn();
  void op_andr();
  void op_xorm();
  void op_xorn();
  void op_xorr();
  void op_orm();
  void op_orn();
  void op_orr();
  void op_cpl();
  void op_cpm();
  void op_cpn();
  void op_cpr();

  void op_incm();
  void op_incr();
  void op_decm();
  void op_decr();

  void op_addhlrr();
  void op_incrr();
  void op_decrr();

  void op_callcc();
  void op_call();
  void op_retcc();
  void op_ret();
  void op_djnz();
  void op_jpccnn();
  void op_jphlm();
  void op_jpnn();
  void op_jr();
  void op_jrc();
  void op_jrz();

  void op_daa();

  void op_di();
  void op_ei();
  void op_exafaf();
  void op_exdehl();
  void op_exsphl();
  void op_exx();
  void op_halt();

  void op_inn();
  void op_outn();

  void op_ldannm();
  void op_ldarrm();
  void op_ldhlnnm();
  void op_ldmn();
  void op_ldmr();
  void op_ldnnma();
  void op_ldnnmhl();
  void op_ldrm();
  void op_ldrn();
  void op_ldrr();
  void op_ldrrma();
  void op_ldrrnn();
  void op_ldsphl();
  void op_pushrr();
  void op_poprr();
  void op_pushaf();
  void op_popaf();

  void op_rlca();
  void op_rla();
  void op_rrca();
  void op_rra();

  void op_rst();

  void op_ccf();
  void op_scf();

  void op_BITS();
  void op_EXTD();
  void op_II();

  uint8_t op_bits_rotm(bool right, bool c, uint16_t addr);
  uint8_t op_bits_rotr(bool right, bool c, uint8_t r);

  uint8_t op_bits_shtm(bool right, bool c, uint16_t addr);
  uint8_t op_bits_shtr(bool right, bool c, uint8_t r);

  void i_bits_bitm(uint8_t op, uint16_t addr);
  void i_bits_bitr(uint8_t op);

  uint8_t i_bits_setresm(uint8_t op, uint16_t addr);
  uint8_t i_bits_setresr(uint8_t op);

  static constexpr bool parity[256] = {
      true,  false, false, true,  false, true,  true,  false, false, true,
      true,  false, true,  false, false, true,  false, true,  true,  false,
      true,  false, false, true,  true,  false, false, true,  false, true,
      true,  false, false, true,  true,  false, true,  false, false, true,
      true,  false, false, true,  false, true,  true,  false, true,  false,
      false, true,  false, true,  true,  false, false, true,  true,  false,
      true,  false, false, true,  false, true,  true,  false, true,  false,
      false, true,  true,  false, false, true,  false, true,  true,  false,
      true,  false, false, true,  false, true,  true,  false, false, true,
      true,  false, true,  false, false, true,  true,  false, false, true,
      false, true,  true,  false, false, true,  true,  false, true,  false,
      false, true,  false, true,  true,  false, true,  false, false, true,
      true,  false, false, true,  false, true,  true,  false, false, true,
      true,  false, true,  false, false, true,  true,  false, false, true,
      false, true,  true,  false, true,  false, false, true,  false, true,
      true,  false, false, true,  true,  false, true,  false, false, true,
      true,  false, false, true,  false, true,  true,  false, false, true,
      true,  false, true,  false, false, true,  false, true,  true,  false,
      true,  false, false, true,  true,  false, false, true,  false, true,
      true,  false, true,  false, false, true,  false, true,  true,  false,
      false, true,  true,  false, true,  false, false, true,  false, true,
      true,  false, true,  false, false, true,  true,  false, false, true,
      false, true,  true,  false, false, true,  true,  false, true,  false,
      false, true,  true,  false, false, true,  false, true,  true,  false,
      true,  false, false, true,  false, true,  true,  false, false, true,
      true,  false, true,  false, false, true};
};

#endif /* Z80_H_ */
