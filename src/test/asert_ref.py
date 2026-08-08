"""An independent ASERT, written from the formula and not from the C++.

Python integers are unbounded, so this needs none of the overflow care the C++
does -- which is exactly what makes it a check on it.
"""
SPACING  = 150
HALFLIFE = 12 * 60 * 60
ANCHOR_H, ANCHOR_BITS, ANCHOR_T = 0, 0x1d00ffff, 1785762002
POWLIMIT_BITS = 0x1d00ffff

def from_compact(c):
    size = c >> 24
    word = c & 0x007fffff
    return word >> (8 * (3 - size)) if size <= 3 else word << (8 * (size - 3))

def to_compact(n):
    size = (n.bit_length() + 7) // 8
    compact = (n >> (8 * (size - 3))) if size > 3 else (n << (8 * (3 - size)))
    if compact & 0x00800000:
        compact >>= 8
        size += 1
    return compact | (size << 24)

def asert(prev_height, prev_time, anchor_bits=ANCHOR_BITS, anchor_t=ANCHOR_T,
          powlimit_bits=POWLIMIT_BITS):
    anchor = from_compact(anchor_bits)
    limit  = from_compact(powlimit_bits)
    blocks  = prev_height - ANCHOR_H
    elapsed = prev_time - anchor_t
    exponent = ((elapsed - SPACING * blocks) * 65536) // HALFLIFE
    shifts = exponent >> 16
    frac = exponent - (shifts << 16)
    factor = 65536 + ((195766423245049 * frac + 971821376 * frac * frac
                       + 5127 * frac ** 3 + (1 << 47)) >> 48)
    nxt = anchor * factor
    if shifts <= 0:
        if -shifts >= 256: return to_compact(1)
        nxt >>= -shifts
    else:
        if shifts >= 256: return to_compact(limit)
        nxt <<= shifts
    nxt >>= 16
    if nxt == 0: return to_compact(1)
    if nxt > limit: return to_compact(limit)
    return to_compact(nxt)

if __name__ == "__main__":
    print("-- anchor at powLimit (the real chain) --")
    for h, late in ((1,0),(288,HALFLIFE),(288,-HALFLIFE),(2016,3600),(840000,-12345)):
        t = ANCHOR_T + SPACING*h + late
        print('        {%d, %d, "%08x"},' % (h, late, asert(h, t)))
    print("-- anchor 256x harder, so both directions move --")
    hard = to_compact(from_compact(0x1d00ffff) >> 8)
    print("   hard anchor bits = %08x" % hard)
    for h, late in ((1,0),(288,HALFLIFE),(288,-HALFLIFE),(2016,3600),(840000,-12345)):
        t = ANCHOR_T + SPACING*h + late
        print('        {%d, %d, "%08x"},' % (h, late, asert(h, t, anchor_bits=hard)))
