# Verify that the logic in the System 7 MPU for doing address decoding works the way it's expected to.

# Active low:
# ~(
#     ~(~(A12 or A7) and A8)
#         and
#     ((clock and VMA) and ~(A13 or A14))
# )

def is_ram_address(address: int) -> bool:
    if address > 0xFFFF:
        return False
    a = [(address & (1 << i)) != 0 for i in range(0, 16)]

    # Williams logic
    #return (not ((not (a[12] or a[7])) and a[8])) and (not (a[13] or a[14]))
    # Williams logic (corrected )
    return (not ((not (a[12] or a[9])) and a[8])) and (not (a[13] or a[14]))
    #return (a[12] and (not a[13]) and (not a[14])) or ((not a[8]) and (not a[13]) and (not a[14])) or (a[7] and (not a[13]) and (not a[14]))

    # Mine:
    # return ((not a[10]) and (not a[11]) and a[12] and (not a[13]) and (not a[14]) and (not a[15])) or ((not a[8]) and (not a[9]) and (not a[10]) and (not a[11]) and (not a[13]) and (not a[14]) and (not a[15]))


low_addr = 0
for addr in range(1, 0xFFFF + 2):
    prev_ok = is_ram_address(addr - 1)
    this_ok = is_ram_address(addr)

    if this_ok and (not prev_ok):
        low_addr = addr
    if (not this_ok) and prev_ok:
        last_good = addr - 1
        print(f"Range: {low_addr:04X} to {last_good:04X}")