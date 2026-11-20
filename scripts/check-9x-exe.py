#!/usr/bin/env python3
"""Verify the 9x ROFLMAO exe can run on Windows 95 RTM on a 386.

Checks:
  1. Only imports DLLs that ship with Windows 95 RTM.
  2. PE subsystem / OS version is at most 4.0 (Win95 refuses newer).
  3. Every instruction in executable sections is in the 80386 integer ISA
     (allowlist), with no MMX/SSE registers, no LOCK on non-memory operands,
     and no undecodable bytes.

Usage: check-9x-exe.py <exe>     (needs: pip install capstone==5.0.7)
"""

import struct
import sys
from collections import Counter

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

ALLOWED_DLLS = {"kernel32.dll", "user32.dll", "gdi32.dll", "shell32.dll", "advapi32.dll", "comctl32.dll"}

# 80386 integer instructions, as capstone (Intel syntax) names them.
I386_MNEMONICS = set("""
aaa aad aam aas adc add and arpl bound bsf bsr bt btc btr bts call cbw cwde cwd cdq clc cld cli clts
cmc cmp cmpsb cmpsw cmpsd daa das dec div enter hlt idiv imul in inc insb insw insd int int3 into iret
iretd ja jae jb jbe jcxz jecxz je jg jge jl jle jmp jne jno jnp jns jo jp js lahf lar lds lea leave les
lfs lgdt lgs lidt lldt lmsw lodsb lodsw lodsd loop loope loopne lsl lss ltr mov movsb movsw movsd movsx
movzx mul neg nop not or out outsb outsw outsd pop popal popaw popfd popfw push pushal pushaw pushfd
pushfw rcl rcr ret retf rol ror sahf sal sar sbb scasb scasw scasd seta setae setb setbe sete setg setge
setl setle setne setno setnp setns seto setp sets sgdt shl shld shr shrd sidt sldt smsw stc std sti
stosb stosw stosd str sub test verr verw wait xchg xlatb xor ljmp lcall
""".split())

PREFIXES = {"lock", "rep", "repe", "repne", "repz", "repnz"}
SIMD_REGISTER_HINTS = ("mm", "xmm", "ymm", "zmm")


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def main(path):
    data = open(path, "rb").read()
    print(f"{path}: {len(data):,} bytes")
    pe = u32(data, 0x3C)
    if data[pe:pe + 4] != b"PE\0\0":
        sys.exit(f"{path}: not a PE file")
    nsections = u16(data, pe + 6)
    opt_size = u16(data, pe + 20)
    opt = pe + 24
    image_base = u32(data, opt + 28)
    os_major, os_minor = u16(data, opt + 40), u16(data, opt + 42)
    sub_major, sub_minor = u16(data, opt + 48), u16(data, opt + 50)

    sections = []
    off = opt + opt_size
    for _ in range(nsections):
        name = data[off:off + 8].rstrip(b"\0").decode()
        vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, off + 8)
        chars = u32(data, off + 36)
        sections.append((name, va, vsize, rptr, rsize, chars))
        off += 40

    def rva_to_off(rva):
        for _, va, vsize, rptr, rsize, _ in sections:
            if va <= rva < va + max(vsize, rsize):
                return rva - va + rptr
        raise ValueError(f"RVA {rva:#x} not in any section")

    failures = []

    # 1. Imports
    import_rva = u32(data, opt + 104)
    imports = []
    if import_rva:
        p = rva_to_off(import_rva)
        while u32(data, p + 12):
            n = rva_to_off(u32(data, p + 12))
            imports.append(data[n:data.index(b"\0", n)].decode())
            p += 20
    print("Imports:", ", ".join(imports) or "(none)")
    bad = [d for d in imports if d.lower() not in ALLOWED_DLLS]
    if bad:
        failures.append(f"imports DLLs not shipped with Windows 95 RTM: {', '.join(bad)}")

    # 2. Version fields
    print(f"OS version {os_major}.{os_minor}, subsystem version {sub_major}.{sub_minor}")
    if (sub_major, sub_minor) > (4, 0) or (os_major, os_minor) > (4, 0):
        failures.append("OS/subsystem version above 4.0; Windows 95 will refuse to load it")

    # 3. Instructions
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.skipdata = True
    total = 0
    offenders = Counter()
    examples = {}
    for name, va, vsize, rptr, rsize, chars in sections:
        if not chars & 0x20000000:  # IMAGE_SCN_MEM_EXECUTE
            continue
        code = data[rptr:rptr + min(vsize, rsize)]
        for insn in md.disasm(code, image_base + va):
            total += 1
            tokens = insn.mnemonic.split()
            mnemonic = tokens[-1]
            reason = None
            if insn.mnemonic.startswith("."):
                reason = "undecodable bytes"
            elif any(t not in PREFIXES for t in tokens[:-1]):
                reason = "unexpected prefix"
            elif mnemonic not in I386_MNEMONICS:
                reason = "not an 80386 instruction"
            elif mnemonic == "nop" and insn.op_str:
                reason = "multi-byte NOP (P6+)"
            elif any(r in insn.op_str for r in SIMD_REGISTER_HINTS):
                reason = "MMX/SSE register"
            elif "lock" in tokens[:-1] and "ptr [" not in insn.op_str:
                reason = "LOCK without memory operand (#UD)"
            if reason:
                key = f"{insn.mnemonic} ({reason})"
                offenders[key] += 1
                examples.setdefault(key, f"{insn.address:#010x}: {insn.mnemonic} {insn.op_str}")

    print(f"Disassembled {total} instructions in executable sections")
    if offenders:
        failures.append(f"{sum(offenders.values())} instruction(s) a 386 can't run:")
        for key, count in offenders.most_common():
            failures.append(f"  {count:5}  {key}   e.g. {examples[key]}")

    if failures:
        for line in failures:
            print(f"::error::{line}" if not line.startswith("  ") else line)
        sys.exit(1)
    print("OK: Windows 95 RTM / 80386 compatible")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
