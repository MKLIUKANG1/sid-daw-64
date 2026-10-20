#pragma once
#include <cstdint>
#include <functional>
#include <cmath>

// Минимальный 6502-эмулятор для запуска SID-плееров.
// Покрывает официальные опкоды 6502. Нелегальные — halt.
// Точность по циклам приблизительная, но для VBI-плееров достаточная.

class Cpu6502 {
public:
    uint8_t  ram[65536] = {0};
    uint8_t  A = 0, X = 0, Y = 0, SP = 0xFD, P = 0x24;
    uint16_t PC = 0;
    uint64_t cycles = 0;

    // Колбеки внешней шины (SID, VIC, CIA). Если nullptr — RAM.
    std::function<void(uint16_t, uint8_t)>* onWrite = nullptr;
    std::function<uint8_t(uint16_t)>*      onRead  = nullptr;

    int currentFrame = 0;

    enum Flag {
        FC = 0x01, FZ = 0x02, FI = 0x04, FD = 0x08,
        FB = 0x10, FU = 0x20, FV = 0x40, FN = 0x80
    };

    uint8_t read(uint16_t addr) {
        if (onRead && *onRead) return (*onRead)(addr);
        return ram[addr];
    }
    void write(uint16_t addr, uint8_t v) {
        if (onWrite && *onWrite) (*onWrite)(addr, v);
        ram[addr] = v;
    }

    void push(uint8_t v) { ram[0x100 + SP] = v; SP--; }
    uint8_t pull()       { SP++; return ram[0x100 + SP]; }

    void setZN(uint8_t v) {
        if (v == 0) P |= FZ; else P &= ~FZ;
        if (v & 0x80) P |= FN; else P &= ~FN;
    }
    void setC(bool c) { if (c) P |= FC; else P &= ~FC; }

    // Режимы адресации
    enum AMode { IMPL, ACC, IMM, ZP, ZPX, ZPY, ABS, ABSX, ABSY,
                 IND, INDX, INDY, REL };

    uint16_t resolve(AMode m) {
        switch (m) {
            case IMM:  return PC++;
            case ZP:   return read(PC++);
            case ZPX:  return uint8_t(read(PC++) + X);
            case ZPY:  return uint8_t(read(PC++) + Y);
            case ABS:  { uint16_t a = read(PC) | (read(PC+1) << 8); PC += 2; return a; }
            case ABSX: { uint16_t a = read(PC) | (read(PC+1) << 8); PC += 2; return a + X; }
            case ABSY: { uint16_t a = read(PC) | (read(PC+1) << 8); PC += 2; return a + Y; }
            case IND:  {
                uint16_t p = read(PC) | (read(PC+1) << 8);
                PC += 2;
                uint16_t lo = read(p);
                uint16_t hi = read((p & 0xFF00) | ((p + 1) & 0xFF));
                return lo | (hi << 8);
            }
            case INDX: {
                uint8_t z = read(PC++) + X;
                uint16_t lo = read(z);
                uint16_t hi = read((z + 1) & 0xFF);
                return lo | (hi << 8);
            }
            case INDY: {
                uint8_t z = read(PC++);
                uint16_t base = read(z) | (read((z + 1) & 0xFF) << 8);
                return base + Y;
            }
            case REL:  { int8_t r = int8_t(read(PC++)); return PC + r; }
            default:   return 0;
        }
    }

    // Выполняет один опкод. Возвращает потраченные циклы (0 = illegal).
    int step();
};
