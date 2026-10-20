#include "SidImporter.h"
#include "Cpu6502.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QMap>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

quint16 be16(const QByteArray &d, int off) {
    return (quint8(d[off]) << 8) | quint8(d[off + 1]);
}
quint16 le16(const QByteArray &d, int off) {
    return quint8(d[off]) | (quint8(d[off + 1]) << 8);
}
quint32 be32(const QByteArray &d, int off) {
    return (quint32(quint8(d[off]))     << 24) |
           (quint32(quint8(d[off + 1])) << 16) |
           (quint32(quint8(d[off + 2])) <<  8) |
            quint32(quint8(d[off + 3]));
}

constexpr double CLOCK_HZ     = 985248.0;
constexpr double CYCLES_FRAME = CLOCK_HZ / 50.0;
constexpr uint16_t RET_PC     = 0x0001;

void dumpMemoryContext(Cpu6502 &cpu, uint16_t pc) {
    qDebug("--- Memory around PC=$%04X ---", pc);
    for (int i = -8; i < 16; ++i) {
        uint16_t a = uint16_t((pc + i) & 0xFFFF);
        qDebug("  %04X: %02X", a, cpu.ram[a]);
    }
    qDebug("--- A=%02X X=%02X Y=%02X SP=%02X P=%02X PC=%04X ---",
           cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.P, cpu.PC);
}

} // namespace

QString SidImporter::parseHeader(const QByteArray &d, SidHeader &h) {
    if (d.size() < 118) return "File too small";

    h.magic = be32(d, 0);
    bool isPSID = (h.magic == 0x50534944);
    bool isRSID = (h.magic == 0x52534944);
    if (!isPSID && !isRSID) {
        QByteArray m = d.left(4);
        QString hex;
        for (int i = 0; i < m.size(); ++i)
            hex += QString("%1 ").arg(quint8(m[i]), 2, 16, QChar('0'));
        return "Not PSID/RSID (magic='" + hex.trimmed() + "')";
    }
    if (isRSID) qWarning() << "RSID detected — trying to emulate";

    h.version     = be16(d, 4);
    h.dataOffset  = be16(d, 6);
    h.loadAddress = be16(d, 8);
    h.initAddress = be16(d, 10);
    h.playAddress = be16(d, 12);
    h.songs       = be16(d, 14);
    h.startSong   = be16(d, 16);
    h.speed       = be32(d, 18);

    auto readField = [&](int off) -> QString {
        if (d.size() < off + 32) return {};
        QByteArray b = d.mid(off, 32);
        int z = b.indexOf('\0');
        if (z >= 0) b.truncate(z);
        return QString::fromLatin1(b).trimmed();
    };
    h.name     = readField(22);
    h.author   = readField(54);
    h.released = readField(86);

    if (h.loadAddress == 0) {
        if (d.size() < int(h.dataOffset) + 2) return "Truncated payload";
        h.loadAddress = le16(d, h.dataOffset);
        h.dataOffset += 2;
    }
    if (h.initAddress == 0) {
        qWarning("initAddress=$0000 → fallback loadAddress=$%04X", h.loadAddress);
        h.initAddress = h.loadAddress;
    }
    if (h.playAddress == 0)
        return "playAddress=$0000 (raster IRQ mode unsupported)";
    return {};
}

QString SidImporter::dumpRegistersEmbedded(const QString &sidPath,
                                            int maxSeconds,
                                            QVector<SidRegisterWrite> &writes,
                                            SidGlobal *outGlobal) {
    writes.clear();

    QFile f(sidPath);
    if (!f.open(QIODevice::ReadOnly)) return "Cannot open: " + sidPath;
    QByteArray data = f.readAll();
    f.close();

    SidHeader hdr;
    QString err = parseHeader(data, hdr);
    if (!err.isEmpty()) return err;

    QByteArray payload = data.mid(hdr.dataOffset);
    if (payload.size() >= 4 && le16(payload, 0) == hdr.loadAddress) {
        qDebug() << "Skip redundant PRG header";
        payload = payload.mid(2);
    }
    if (payload.isEmpty()) return "Empty payload";

    qDebug("Load=$%04X Init=$%04X Play=$%04X payload=%d",
           hdr.loadAddress, hdr.initAddress, hdr.playAddress, payload.size());

    Cpu6502 cpu;
    std::memset(cpu.ram, 0, sizeof(cpu.ram));
    for (int i = 0; i < payload.size(); ++i)
        cpu.ram[(hdr.loadAddress + i) & 0xFFFF] = quint8(payload[i]);

    // CIA #1
    struct Cia1State {
        uint16_t timerA = 0xFFFF, latch = 0xFFFF;
        uint8_t cra = 0, irqFlags = 0;
        bool running = false;
    } cia1;

    std::function<void(uint16_t, uint8_t)> onWrite =
        [&](uint16_t a, uint8_t v) {
            if (a >= 0xD400 && a <= 0xD41F) {
                SidRegisterWrite w;
                w.frame = cpu.currentFrame;
                w.reg   = int(a - 0xD400);
                w.value = int(v);
                writes.append(w);
            }
            if (a >= 0xDC00 && a <= 0xDC0F) {
                int r = a - 0xDC00;
                if      (r == 0x04) cia1.latch = (cia1.latch & 0xFF00) | v;
                else if (r == 0x05) cia1.latch = (cia1.latch & 0x00FF) | (uint16_t(v) << 8);
                else if (r == 0x0E) {
                    cia1.cra = v;
                    if (v & 0x10) cia1.timerA = cia1.latch;
                    cia1.running = (v & 0x01) != 0;
                }
                else if (r == 0x0D) {
                    if (v & 0x80) cia1.irqFlags &= ~(v & 0x7F);
                    else          cia1.irqFlags |=  (v & 0x7F);
                }
            }
            cpu.ram[a] = v;
        };

    std::function<uint8_t(uint16_t)> onRead =
        [&](uint16_t a) -> uint8_t {
            if (a == 0xD012) return uint8_t(cpu.currentFrame % 312);
            if (a == 0xD011) return uint8_t(0x1B | ((cpu.currentFrame / 8) & 0x80));
            if (a == 0xD019) return 0x00;
            if (a >= 0xDC00 && a <= 0xDC0F) {
                int r = a - 0xDC00;
                if (r == 0x04) return uint8_t(cia1.timerA & 0xFF);
                if (r == 0x05) return uint8_t((cia1.timerA >> 8) & 0xFF);
                if (r == 0x0D) return uint8_t(cia1.irqFlags | (cia1.irqFlags ? 0x80 : 0));
                if (r == 0x0E) return cia1.cra;
                return 0;
            }
            return cpu.ram[a];
        };
    cpu.onWrite = &onWrite;
    cpu.onRead  = &onRead;

    auto pushReturn = [&](uint16_t retMinus1) {
        cpu.ram[0x100 + cpu.SP] = uint8_t(retMinus1 >> 8); cpu.SP--;
        cpu.ram[0x100 + cpu.SP] = uint8_t(retMinus1 & 0xFF); cpu.SP--;
    };

    // init
    cpu.SP = 0xFD;
    pushReturn(RET_PC - 1);
    cpu.PC = hdr.initAddress;
    uint64_t initLimit = cpu.cycles + 500000;
    int initSteps = 0;
    while (cpu.PC != RET_PC && cpu.cycles < initLimit) {
        int c = cpu.step(); initSteps++;
        if (c == 0) {
            uint16_t badPC = uint16_t((cpu.PC - 1) & 0xFFFF);
            uint8_t badOp  = cpu.ram[badPC];
            qWarning("!!! init halted at PC=$%04X op=$%02X (%d steps)",
                     badPC, badOp, initSteps);
            dumpMemoryContext(cpu, badPC);
            return QString("init halted PC=$%1 op=$%2")
                   .arg(badPC, 4, 16, QChar('0'))
                   .arg(badOp, 2, 16, QChar('0'));
        }
        cpu.cycles += c;
        if (cia1.running && cia1.timerA > 0) {
            cia1.timerA = (cia1.timerA >= uint16_t(c)) ? cia1.timerA - uint16_t(c) : 0;
            if (cia1.timerA == 0) { cia1.timerA = cia1.latch; cia1.irqFlags |= 0x01; }
        }
    }
    qDebug("Init OK: %d steps, CIA timerA=$%04X latch=$%04X CRA=$%02X run=%d",
           initSteps, cia1.timerA, cia1.latch, cia1.cra, cia1.running ? 1 : 0);

    // play
    int totalFrames = std::max(1, maxSeconds) * 50;
    for (int fr = 0; fr < totalFrames; ++fr) {
        cpu.currentFrame = fr;
        uint64_t frameStart = cpu.cycles;
        uint64_t frameLimit = frameStart + uint64_t(CYCLES_FRAME) + 5000;
        pushReturn(RET_PC - 1);
        cpu.PC = hdr.playAddress;
        while (cpu.PC != RET_PC && cpu.cycles < frameLimit) {
            int c = cpu.step();
            if (c == 0) {
                uint16_t badPC = uint16_t((cpu.PC - 1) & 0xFFFF);
                uint8_t badOp  = cpu.ram[badPC];
                qWarning("!!! play halted at frame=%d PC=$%04X op=$%02X",
                         fr, badPC, badOp);
                dumpMemoryContext(cpu, badPC);
                return QString("play halted frame %1 PC=$%2 op=$%3")
                       .arg(fr).arg(badPC, 4, 16, QChar('0'))
                       .arg(badOp, 2, 16, QChar('0'));
            }
            cpu.cycles += c;
            if (cia1.running && cia1.timerA > 0) {
                cia1.timerA = (cia1.timerA >= uint16_t(c)) ? cia1.timerA - uint16_t(c) : 0;
                if (cia1.timerA == 0) { cia1.timerA = cia1.latch; cia1.irqFlags |= 0x01; }
            }
        }
        if (cpu.cycles < frameStart + uint64_t(CYCLES_FRAME))
            cpu.cycles = frameStart + uint64_t(CYCLES_FRAME);
    }
    qDebug("Play OK: %d frames, %d writes", totalFrames, writes.size());

    if (writes.isEmpty()) return "No SID writes captured";

    // ===== Extract global filter state =====
    if (outGlobal) {
        *outGlobal = SidGlobal{};
        int lastFC_lo = 0, lastFC_hi = 0, lastRes = 0;
        int lastFilt = 0, lastMode = 0, lastVol = 15, lastVoices = 0;
        bool f23 = false;
        for (const auto &w : writes) {
            if (w.reg == 21) lastFC_lo = w.value;
            if (w.reg == 22) lastFC_hi = w.value;
            if (w.reg == 23) {
                lastRes  = (w.value >> 4) & 0x0F;
                lastFilt = (w.value >> 3) & 0x01;
                lastMode = (w.value >> 1) & 0x03;
                f23 = true;
            }
            if (w.reg == 24) {
                lastVol = w.value & 0x0F;
                lastVoices = (w.value >> 4) & 0x07;
            }
        }
        outGlobal->filterCutoff    = ((lastFC_hi & 0x07) << 8) | lastFC_lo;
        outGlobal->filterResonance = lastRes;
        outGlobal->filterMode      = (lastMode <= 2) ? lastMode : 0;
        outGlobal->filterEnabled   = f23 && (lastFilt != 0);
        outGlobal->masterVolume    = lastVol;
        outGlobal->filterVoices    = lastVoices;
        qDebug("Global SID: cutoff=%d res=%d mode=%d enabled=%d vol=%d voices=0x%X",
               outGlobal->filterCutoff, outGlobal->filterResonance,
               outGlobal->filterMode, outGlobal->filterEnabled ? 1 : 0,
               outGlobal->masterVolume, outGlobal->filterVoices);
    }
    return {};
}

QString SidImporter::buildPatternFromWrites(
    const QVector<SidRegisterWrite> &writes,
    Pattern &pat, QVector<Instrument> &instrs)
{
    struct VState {
        bool gate = false;
        int  startFrame = 0;
        int  currentFreq = 0, currentControl = 0;
        int  currentAD = 0, currentSR = 0, currentPW = 0;
        QVector<QPair<int,int>> freqLog, pwLog;
    } vs[3];

    auto freqToMidi = [&](int f) -> int {
        if (f <= 0) return 60;
        double hz = double(f) * CLOCK_HZ / 16777216.0;
        if (hz < 15.0 || hz > 12000.0) return 60;
        int m = int(std::round(69.0 + 12.0 * std::log2(hz / 440.0)));
        return std::clamp(m, 24, 108);
    };

    QMap<int, int> instrCache;
    QVector<Note> pendingNotes;

    auto finalizeNote = [&](int v) {
        auto &st = vs[v];
        if (!st.gate) return;
        st.gate = false;
        if (st.freqLog.isEmpty()) return;

        int lenFrames = std::max(1, st.freqLog.last().first - st.startFrame);
        int basePitch = freqToMidi(st.freqLog.first().second);

        QVector<int> pitchEnv;
        int lastSampleFrame = st.freqLog.first().first;
        for (const auto &fr : st.freqLog) {
            if (fr.first - lastSampleFrame < 2) continue;
            int midi = freqToMidi(fr.second);
            pitchEnv.append(std::clamp(midi - basePitch + 8, 0, 15));
            lastSampleFrame = fr.first;
        }

        QVector<int> dutyEnv;
        if (!st.pwLog.isEmpty()) {
            int lastD = st.pwLog.first().first;
            dutyEnv.append(std::clamp(st.pwLog.first().second / 273, 0, 15));
            for (const auto &fr : st.pwLog) {
                if (fr.first - lastD < 2) continue;
                dutyEnv.append(std::clamp(fr.second / 273, 0, 15));
                lastD = fr.first;
            }
        }

        int waveform = (st.currentControl >> 4) & 0x0F;
        int key = (waveform << 24) | (st.currentAD << 16)
                | (st.currentSR << 8) | (st.currentPW & 0xFF)
                | (pitchEnv.size() > 1 ? 0x10000 : 0);
        int idx = instrCache.value(key, -1);
        if (idx < 0) {
            Instrument ins;
            ins.name = QString("SID V%1").arg(v + 1);
            ins.params.waveform   = waveform;
            ins.params.attack     = (st.currentAD >> 4) & 0x0F;
            ins.params.decay      = st.currentAD & 0x0F;
            ins.params.sustain    = (st.currentSR >> 4) & 0x0F;
            ins.params.release    = st.currentSR & 0x0F;
            ins.params.pulseWidth = st.currentPW & 0x0FFF;
            ins.params.ringMod    = (st.currentControl & 0x04) != 0;
            ins.params.sync       = (st.currentControl & 0x02) != 0;
            if (pitchEnv.size() > 1) {
                ins.params.pitchEnvelope = pitchEnv;
                ins.params.pitchEnvelopeAbsolute = false;
                ins.params.envelopeSpeed = std::max(1, 100 / std::max(1, lenFrames));
            }
            if (dutyEnv.size() > 1) {
                ins.params.dutyEnvelope = dutyEnv;
                if (ins.params.envelopeSpeed <= 0)
                    ins.params.envelopeSpeed = std::max(1, 100 / std::max(1, lenFrames));
            }
            idx = instrs.size();
            instrs.append(ins);
            instrCache[key] = idx;
        }

        Note n;
        n.tick   = int(std::round(st.startFrame * 0.16));
        n.length = std::max(1, int(std::round(lenFrames * 0.16)));
        n.pitch  = basePitch;
        n.voice  = v;
        n.instrumentIndex = idx;
        pendingNotes.append(n);
    };

    for (const auto &e : writes) {
        int v = e.reg / 7;
        int r = e.reg % 7;
        if (v > 2) continue;
        auto &st = vs[v];

        if      (r == 0) st.currentFreq = (st.currentFreq & 0xFF00) | e.value;
        else if (r == 1) st.currentFreq = (st.currentFreq & 0x00FF) | (e.value << 8);
        else if (r == 2) st.currentPW   = (st.currentPW & 0x0F00) | e.value;
        else if (r == 3) st.currentPW   = (st.currentPW & 0x00FF) | ((e.value & 0x0F) << 8);
        else if (r == 5) st.currentAD   = e.value;
        else if (r == 6) st.currentSR   = e.value;
        else if (r == 4) {
            bool ng = (e.value & 0x01) != 0;
            int  nw = (e.value >> 4) & 0x0F;

            if (ng && !st.gate) {
                st.gate = true;
                st.startFrame = e.frame;
                st.currentControl = e.value;
                st.freqLog.clear(); st.pwLog.clear();
                st.freqLog.append({e.frame, st.currentFreq});
                st.pwLog.append({e.frame, st.currentPW});
            } else if (!ng && st.gate) {
                st.freqLog.append({e.frame, st.currentFreq});
                finalizeNote(v);
            } else if (st.gate) {
                int ow = (st.currentControl >> 4) & 0x0F;
                if (nw != ow) {
                    st.freqLog.append({e.frame, st.currentFreq});
                    finalizeNote(v);
                    st.gate = true;
                    st.startFrame = e.frame;
                    st.currentControl = e.value;
                    st.freqLog.clear(); st.pwLog.clear();
                    st.freqLog.append({e.frame, st.currentFreq});
                    st.pwLog.append({e.frame, st.currentPW});
                } else {
                    st.currentControl = e.value;
                    st.freqLog.append({e.frame, st.currentFreq});
                    st.pwLog.append({e.frame, st.currentPW});
                }
            }
        }
    }

    for (int v = 0; v < 3; ++v) {
        if (vs[v].gate) {
            int lf = writes.isEmpty() ? 0 : writes.last().frame;
            vs[v].freqLog.append({lf, vs[v].currentFreq});
            finalizeNote(v);
        }
    }

    pat.notes = pendingNotes;
    std::sort(pat.notes.begin(), pat.notes.end(),
              [](const Note &a, const Note &b) {
                  if (a.tick != b.tick) return a.tick < b.tick;
                  return a.pitch < b.pitch;
              });

    int maxEnd = 64;
    for (const auto &n : pat.notes) maxEnd = std::max(maxEnd, n.tick + n.length);
    pat.lengthTicks = ((maxEnd + 15) / 16) * 16;

    qDebug("Pattern: %d notes, %d instruments, %d ticks",
           pat.notes.size(), instrs.size(), pat.lengthTicks);
    return {};
}
