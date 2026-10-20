#include "Cpu6502.h"

int Cpu6502::step() {
    uint8_t op = read(PC++);
    switch (op) {
        // ===== LDA =====
        case 0xA9: A = read(resolve(IMM));  setZN(A); return 2;
        case 0xA5: A = read(resolve(ZP));   setZN(A); return 3;
        case 0xB5: A = read(resolve(ZPX));  setZN(A); return 4;
        case 0xAD: A = read(resolve(ABS));  setZN(A); return 4;
        case 0xBD: A = read(resolve(ABSX)); setZN(A); return 4;
        case 0xB9: A = read(resolve(ABSY)); setZN(A); return 4;
        case 0xA1: A = read(resolve(INDX)); setZN(A); return 6;
        case 0xB1: A = read(resolve(INDY)); setZN(A); return 5;
        // ===== LDX =====
        case 0xA2: X = read(resolve(IMM));  setZN(X); return 2;
        case 0xA6: X = read(resolve(ZP));   setZN(X); return 3;
        case 0xB6: X = read(resolve(ZPY));  setZN(X); return 4;
        case 0xAE: X = read(resolve(ABS));  setZN(X); return 4;
        case 0xBE: X = read(resolve(ABSY)); setZN(X); return 4;
        // ===== LDY =====
        case 0xA0: Y = read(resolve(IMM));  setZN(Y); return 2;
        case 0xA4: Y = read(resolve(ZP));   setZN(Y); return 3;
        case 0xB4: Y = read(resolve(ZPX));  setZN(Y); return 4;
        case 0xAC: Y = read(resolve(ABS));  setZN(Y); return 4;
        case 0xBC: Y = read(resolve(ABSX)); setZN(Y); return 4;
        // ===== STA =====
        case 0x85: write(resolve(ZP),   A); return 3;
        case 0x95: write(resolve(ZPX),  A); return 4;
        case 0x8D: write(resolve(ABS),  A); return 4;
        case 0x9D: write(resolve(ABSX), A); return 5;
        case 0x99: write(resolve(ABSY), A); return 5;
        case 0x81: write(resolve(INDX), A); return 6;
        case 0x91: write(resolve(INDY), A); return 6;
        // ===== STX / STY =====
        case 0x86: write(resolve(ZP),  X); return 3;
        case 0x96: write(resolve(ZPY), X); return 4;
        case 0x8E: write(resolve(ABS), X); return 4;
        case 0x84: write(resolve(ZP),  Y); return 3;
        case 0x94: write(resolve(ZPX), Y); return 4;
        case 0x8C: write(resolve(ABS), Y); return 4;
        // ===== Transfer =====
        case 0xAA: X = A; setZN(X); return 2;
        case 0xA8: Y = A; setZN(Y); return 2;
        case 0x8A: A = X; setZN(A); return 2;
        case 0x98: A = Y; setZN(A); return 2;
        case 0xBA: X = SP; setZN(X); return 2;
        case 0x9A: SP = X; return 2;
        // ===== ADC =====
        case 0x69: { uint8_t v=read(resolve(IMM));  uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 2; }
        case 0x65: { uint8_t v=read(resolve(ZP));   uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 3; }
        case 0x75: { uint8_t v=read(resolve(ZPX));  uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 4; }
        case 0x6D: { uint8_t v=read(resolve(ABS));  uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 4; }
        case 0x7D: { uint8_t v=read(resolve(ABSX)); uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 4; }
        case 0x79: { uint8_t v=read(resolve(ABSY)); uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 4; }
        case 0x61: { uint8_t v=read(resolve(INDX)); uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 6; }
        case 0x71: { uint8_t v=read(resolve(INDY)); uint16_t s=A+v+(P&FC?1:0); setC(s>0xFF); A=uint8_t(s); setZN(A); return 5; }
        // ===== SBC =====
        case 0xE9: { uint8_t v=read(resolve(IMM));  int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 2; }
        case 0xE5: { uint8_t v=read(resolve(ZP));   int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 3; }
        case 0xF5: { uint8_t v=read(resolve(ZPX));  int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 4; }
        case 0xED: { uint8_t v=read(resolve(ABS));  int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 4; }
        case 0xFD: { uint8_t v=read(resolve(ABSX)); int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 4; }
        case 0xF9: { uint8_t v=read(resolve(ABSY)); int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 4; }
        case 0xE1: { uint8_t v=read(resolve(INDX)); int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 6; }
        case 0xF1: { uint8_t v=read(resolve(INDY)); int s=A-v-(P&FC?0:1); setC(s>=0); A=uint8_t(s); setZN(A); return 5; }
        // ===== AND =====
        case 0x29: A &= read(resolve(IMM));  setZN(A); return 2;
        case 0x25: A &= read(resolve(ZP));   setZN(A); return 3;
        case 0x35: A &= read(resolve(ZPX));  setZN(A); return 4;
        case 0x2D: A &= read(resolve(ABS));  setZN(A); return 4;
        case 0x3D: A &= read(resolve(ABSX)); setZN(A); return 4;
        case 0x39: A &= read(resolve(ABSY)); setZN(A); return 4;
        case 0x21: A &= read(resolve(INDX)); setZN(A); return 6;
        case 0x31: A &= read(resolve(INDY)); setZN(A); return 5;
        // ===== ORA =====
        case 0x09: A |= read(resolve(IMM));  setZN(A); return 2;
        case 0x05: A |= read(resolve(ZP));   setZN(A); return 3;
        case 0x15: A |= read(resolve(ZPX));  setZN(A); return 4;
        case 0x0D: A |= read(resolve(ABS));  setZN(A); return 4;
        case 0x1D: A |= read(resolve(ABSX)); setZN(A); return 4;
        case 0x19: A |= read(resolve(ABSY)); setZN(A); return 4;
        case 0x01: A |= read(resolve(INDX)); setZN(A); return 6;
        case 0x11: A |= read(resolve(INDY)); setZN(A); return 5;
        // ===== EOR =====
        case 0x49: A ^= read(resolve(IMM));  setZN(A); return 2;
        case 0x45: A ^= read(resolve(ZP));   setZN(A); return 3;
        case 0x55: A ^= read(resolve(ZPX));  setZN(A); return 4;
        case 0x4D: A ^= read(resolve(ABS));  setZN(A); return 4;
        case 0x5D: A ^= read(resolve(ABSX)); setZN(A); return 4;
        case 0x59: A ^= read(resolve(ABSY)); setZN(A); return 4;
        case 0x41: A ^= read(resolve(INDX)); setZN(A); return 6;
        case 0x51: A ^= read(resolve(INDY)); setZN(A); return 5;
        // ===== CMP =====
        case 0xC9: { uint8_t v=read(resolve(IMM));  setC(A>=v); setZN(uint8_t(A-v)); return 2; }
        case 0xC5: { uint8_t v=read(resolve(ZP));   setC(A>=v); setZN(uint8_t(A-v)); return 3; }
        case 0xD5: { uint8_t v=read(resolve(ZPX));  setC(A>=v); setZN(uint8_t(A-v)); return 4; }
        case 0xCD: { uint8_t v=read(resolve(ABS));  setC(A>=v); setZN(uint8_t(A-v)); return 4; }
        case 0xDD: { uint8_t v=read(resolve(ABSX)); setC(A>=v); setZN(uint8_t(A-v)); return 4; }
        case 0xD9: { uint8_t v=read(resolve(ABSY)); setC(A>=v); setZN(uint8_t(A-v)); return 4; }
        case 0xC1: { uint8_t v=read(resolve(INDX)); setC(A>=v); setZN(uint8_t(A-v)); return 6; }
        case 0xD1: { uint8_t v=read(resolve(INDY)); setC(A>=v); setZN(uint8_t(A-v)); return 5; }
        case 0xE0: { uint8_t v=read(resolve(IMM));  setC(X>=v); setZN(uint8_t(X-v)); return 2; }
        case 0xE4: { uint8_t v=read(resolve(ZP));   setC(X>=v); setZN(uint8_t(X-v)); return 3; }
        case 0xEC: { uint8_t v=read(resolve(ABS));  setC(X>=v); setZN(uint8_t(X-v)); return 4; }
        case 0xC0: { uint8_t v=read(resolve(IMM));  setC(Y>=v); setZN(uint8_t(Y-v)); return 2; }
        case 0xC4: { uint8_t v=read(resolve(ZP));   setC(Y>=v); setZN(uint8_t(Y-v)); return 3; }
        case 0xCC: { uint8_t v=read(resolve(ABS));  setC(Y>=v); setZN(uint8_t(Y-v)); return 4; }
        // ===== INC / DEC =====
        case 0xE6: { uint16_t a=resolve(ZP);   uint8_t v=read(a)+1; write(a,v); setZN(v); return 5; }
        case 0xF6: { uint16_t a=resolve(ZPX);  uint8_t v=read(a)+1; write(a,v); setZN(v); return 6; }
        case 0xEE: { uint16_t a=resolve(ABS);  uint8_t v=read(a)+1; write(a,v); setZN(v); return 6; }
        case 0xFE: { uint16_t a=resolve(ABSX); uint8_t v=read(a)+1; write(a,v); setZN(v); return 7; }
        case 0xC6: { uint16_t a=resolve(ZP);   uint8_t v=read(a)-1; write(a,v); setZN(v); return 5; }
        case 0xD6: { uint16_t a=resolve(ZPX);  uint8_t v=read(a)-1; write(a,v); setZN(v); return 6; }
        case 0xCE: { uint16_t a=resolve(ABS);  uint8_t v=read(a)-1; write(a,v); setZN(v); return 6; }
        case 0xDE: { uint16_t a=resolve(ABSX); uint8_t v=read(a)-1; write(a,v); setZN(v); return 7; }
        case 0xE8: X++; setZN(X); return 2;
        case 0xCA: X--; setZN(X); return 2;
        case 0xC8: Y++; setZN(Y); return 2;
        case 0x88: Y--; setZN(Y); return 2;
        // ===== ASL =====
        case 0x0A: setC(A & 0x80); A <<= 1; setZN(A); return 2;
        case 0x06: { uint16_t a=resolve(ZP);   uint8_t v=read(a); setC(v&0x80); v<<=1; write(a,v); setZN(v); return 5; }
        case 0x16: { uint16_t a=resolve(ZPX);  uint8_t v=read(a); setC(v&0x80); v<<=1; write(a,v); setZN(v); return 6; }
        case 0x0E: { uint16_t a=resolve(ABS);  uint8_t v=read(a); setC(v&0x80); v<<=1; write(a,v); setZN(v); return 6; }
        case 0x1E: { uint16_t a=resolve(ABSX); uint8_t v=read(a); setC(v&0x80); v<<=1; write(a,v); setZN(v); return 7; }
        // ===== LSR =====
        case 0x4A: setC(A & 0x01); A >>= 1; setZN(A); return 2;
        case 0x46: { uint16_t a=resolve(ZP);   uint8_t v=read(a); setC(v&0x01); v>>=1; write(a,v); setZN(v); return 5; }
        case 0x56: { uint16_t a=resolve(ZPX);  uint8_t v=read(a); setC(v&0x01); v>>=1; write(a,v); setZN(v); return 6; }
        case 0x4E: { uint16_t a=resolve(ABS);  uint8_t v=read(a); setC(v&0x01); v>>=1; write(a,v); setZN(v); return 6; }
        case 0x5E: { uint16_t a=resolve(ABSX); uint8_t v=read(a); setC(v&0x01); v>>=1; write(a,v); setZN(v); return 7; }
        // ===== ROL =====
        case 0x2A: { uint8_t c=(P&FC)?1:0; setC(A&0x80); A=(A<<1)|c; setZN(A); return 2; }
        case 0x26: { uint16_t a=resolve(ZP);   uint8_t v=read(a); uint8_t c=(P&FC)?1:0; setC(v&0x80); v=(v<<1)|c; write(a,v); setZN(v); return 5; }
        case 0x36: { uint16_t a=resolve(ZPX);  uint8_t v=read(a); uint8_t c=(P&FC)?1:0; setC(v&0x80); v=(v<<1)|c; write(a,v); setZN(v); return 6; }
        case 0x2E: { uint16_t a=resolve(ABS);  uint8_t v=read(a); uint8_t c=(P&FC)?1:0; setC(v&0x80); v=(v<<1)|c; write(a,v); setZN(v); return 6; }
        case 0x3E: { uint16_t a=resolve(ABSX); uint8_t v=read(a); uint8_t c=(P&FC)?1:0; setC(v&0x80); v=(v<<1)|c; write(a,v); setZN(v); return 7; }
        // ===== ROR =====
        case 0x6A: { uint8_t c=(P&FC)?0x80:0; setC(A&0x01); A=(A>>1)|c; setZN(A); return 2; }
        case 0x66: { uint16_t a=resolve(ZP);   uint8_t v=read(a); uint8_t c=(P&FC)?0x80:0; setC(v&1); v=(v>>1)|c; write(a,v); setZN(v); return 5; }
        case 0x76: { uint16_t a=resolve(ZPX);  uint8_t v=read(a); uint8_t c=(P&FC)?0x80:0; setC(v&1); v=(v>>1)|c; write(a,v); setZN(v); return 6; }
        case 0x6E: { uint16_t a=resolve(ABS);  uint8_t v=read(a); uint8_t c=(P&FC)?0x80:0; setC(v&1); v=(v>>1)|c; write(a,v); setZN(v); return 6; }
        case 0x7E: { uint16_t a=resolve(ABSX); uint8_t v=read(a); uint8_t c=(P&FC)?0x80:0; setC(v&1); v=(v>>1)|c; write(a,v); setZN(v); return 7; }
        // ===== Branches =====
        case 0xF0: { int8_t r=int8_t(read(PC++)); if (P&FZ) { PC+=r; return 3; } return 2; }
        case 0xD0: { int8_t r=int8_t(read(PC++)); if (!(P&FZ)) { PC+=r; return 3; } return 2; }
        case 0x90: { int8_t r=int8_t(read(PC++)); if (!(P&FC)) { PC+=r; return 3; } return 2; }
        case 0xB0: { int8_t r=int8_t(read(PC++)); if (P&FC) { PC+=r; return 3; } return 2; }
        case 0x10: { int8_t r=int8_t(read(PC++)); if (!(P&FN)) { PC+=r; return 3; } return 2; }
        case 0x30: { int8_t r=int8_t(read(PC++)); if (P&FN) { PC+=r; return 3; } return 2; }
        case 0x50: { int8_t r=int8_t(read(PC++)); if (!(P&FV)) { PC+=r; return 3; } return 2; }
        case 0x70: { int8_t r=int8_t(read(PC++)); if (P&FV) { PC+=r; return 3; } return 2; }
        // ===== Jumps / Calls =====
        case 0x4C: PC = resolve(ABS); return 3;
        case 0x6C: PC = resolve(IND); return 5;
        case 0x20: {
            uint16_t tgt = resolve(ABS);
            uint16_t ret = PC - 1;
            ram[0x100 + SP] = (ret >> 8) & 0xFF; SP--;
            ram[0x100 + SP] = ret & 0xFF;        SP--;
            PC = tgt;
            return 6;
        }
        case 0x60: {
            uint8_t lo = pull();
            uint8_t hi = pull();
            PC = ((hi << 8) | lo) + 1;
            return 6;
        }
        case 0x40: {
            P = pull() | FU;
            uint8_t lo = pull();
            uint8_t hi = pull();
            PC = (hi << 8) | lo;
            return 6;
        }
        // ===== Stack =====
        case 0x48: push(A); return 3;
        case 0x68: A = pull(); setZN(A); return 4;
        case 0x08: push(P | FB | FU); return 3;
        case 0x28: P = pull() | FU; return 4;
        // ===== Flags =====
        case 0x18: P &= ~FC; return 2;
        case 0x38: P |=  FC; return 2;
        case 0x58: P &= ~FI; return 2;
        case 0x78: P |=  FI; return 2;
        case 0xD8: P &= ~FD; return 2;
        case 0xF8: P |=  FD; return 2;
        case 0xB8: P &= ~FV; return 2;
        // ===== BIT =====
        case 0x24: { uint8_t v=read(resolve(ZP));  if (!(A&v)) P|=FZ; else P&=~FZ; P=(P&~FN)|(v&FN); P=(P&~FV)|(v&0x40); return 3; }
        case 0x2C: { uint8_t v=read(resolve(ABS)); if (!(A&v)) P|=FZ; else P&=~FZ; P=(P&~FN)|(v&FN); P=(P&~FV)|(v&0x40); return 4; }
        // ===== NOP =====
        case 0xEA: return 2;

        // ===== BRK — явный halt =====
        case 0x00: return 0;

        // ===== Неизвестный/нелегальный опкод =====
        // Трактируем как 2-байтовый NOP: пропускаем 1 байт операнда.
        // Это лечит нелегальные NOP-варианты (1A, 3A, 5A, 7A, DA, FA и др.),
        // но НЕ лечит реальные "прыжки в пустоту" (RAM[PC]=0 уже поймано case 0x00).
        default:
            PC += 1;
            return 2;
    }
}
