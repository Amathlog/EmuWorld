from pathlib import Path
from typing import NamedTuple, Tuple
import re


class CompleteInstruction(NamedTuple):
    addr: int
    ins: str
    operands: str
    a: int
    x: int
    y: int
    p: int
    sp: int
    ppu: Tuple[int, int]
    cyc: int

    def __repr__(self) -> str:
        i = -1
        while self.operands[i] == ' ' and -i < len(self.operands):
            i -= 1

        operands_clipped = self.operands[:i+1]

        res = []
        res.append("addr: " + hex(self.addr))
        res.append("ins: " + self.ins)
        res.append("operands: " + operands_clipped)
        res.append("a: " + hex(self.a))
        res.append("x: " + hex(self.x))
        res.append("y: " + hex(self.y))
        res.append("p: " + hex(self.p))
        res.append("sp: " + hex(self.sp))
        res.append("ppu: " + str(self.ppu))
        res.append("cyc: " + str(self.cyc))
        return "; ".join(res)

def import_good_log(path: Path):
    with path.open("r") as f:
        data = f.readlines()

    res = []

    regex = re.compile("([0-9A-F]{4}) *(([0-9A-F]{2}) ){1,3} *(\*?[A-Z]{3}) (.*) +A:([0-9A-F]{2}) X:([0-9A-F]{2}) Y:([0-9A-F]{2}) P:([0-9A-F]{2}) SP:([0-9A-F]{2}) PPU:( *[0-9]+),( *[0-9]*) CYC:([0-9]*)")
    for line in data:
        m = regex.match(line)
        res.append(CompleteInstruction(
            int(m[1], 16),
            m[4],
            m[5],
            int(m[6],16),
            int(m[7], 16),
            int(m[8], 16),
            int(m[9], 16),
            int(m[10], 16),
            (int(m[11]), int(m[12])),
            int(m[13])
        ))

    return res

def import_our_log(path: Path):
    with path.open("r") as f:
        data = f.readlines()

    res = []

    regex = re.compile("0x([0-9A-F]{4}) (\*?[A-Z\?]{3}) (.*)A:([0-9a-f]{1,2}) X:([0-9a-f]{1,2}) Y:([0-9a-f]{1,2}) P:([0-9a-f]{1,2}) SP:([0-9a-f]{1,2}) PPU:( *-?[0-9]+),( *[0-9]*) CYC:([0-9]*)")
    for line in data:
        m = regex.match(line)
        res.append(CompleteInstruction(
            int(m[1], 16),
            m[2],
            m[3],
            int(m[4],16),
            int(m[5], 16),
            int(m[6], 16),
            int(m[7], 16),
            int(m[8], 16),
            (int(m[9]), int(m[10])),
            int(m[11])
        ))

    return res

def compare_log(good_log, our_log):
    for i in range(len(good_log)):
        if i >= len(our_log):
            break

        good_ins = good_log[i]
        our_ins = our_log[i]

        if (good_ins.addr != our_ins.addr):
            print(f"Address divergence at line {i+1}")
            return i

        if (good_ins.ins != our_ins.ins and good_ins.ins[0] != "*"):
            print(f"Ins divergence at line {i+1}")
            return i

        if (good_ins.a != our_ins.a):
            print(f"A divergence at line {i+1}")
            return i

        if (good_ins.x != our_ins.x):
            print(f"X divergence at line {i+1}")
            return i

        if (good_ins.y != our_ins.y):
            print(f"Y divergence at line {i+1}")
            return i

        if (good_ins.sp != our_ins.sp):
            print(f"SP divergence at line {i+1}")
            return i

        if (good_ins.p != our_ins.p):
            print(f"P divergence at line {i+1}")
            return i

if __name__ == "__main__":
    import os
    root = Path(os.path.abspath(__file__)).parent
    good_log = root / "good_log.txt"
    good_log_data = import_good_log(good_log)
    #print(good_log_data)

    our_log = root / "our_log.txt"
    our_log_data = import_our_log(our_log)
    #print(our_log_data)

    faulty_index = compare_log(good_log_data, our_log_data)

    print("Good log:")
    print(good_log_data[faulty_index-1])
    print(good_log_data[faulty_index])

    print("Our log:")
    print(our_log_data[faulty_index-1])
    print(our_log_data[faulty_index])