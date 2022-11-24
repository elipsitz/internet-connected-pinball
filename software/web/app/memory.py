import itertools
from typing import List

class System7Memory:
    def __init__(self, memory: bytes) -> None:
        assert len(memory) == 1024
        self.memory = memory

    @property
    def num_players(self) -> int:
        return self.memory[0xCC] + 1

    @property
    def player_scores(self) -> List[int]:
        scores = []
        for i in range(self.num_players):
            addr = 0x38 + (i * 4)
            data = self.memory[addr:(addr + 4)]
            scores.append(decode_bcd(data))
        return scores


def decode_bcd(data: bytes) -> int:
    # Big endian BCD. Anything >9 doesn't show up.
    digits = list(itertools.chain.from_iterable((((x & 0xF0) >> 4), x & 0xF) for x in data))
    value = 0
    for digit in digits:
        if digit >= 10:
            continue
        value *= 10
        value += digit
    return value