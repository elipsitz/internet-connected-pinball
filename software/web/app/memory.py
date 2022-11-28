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

    @property
    def player_up(self) -> int:
        return self.memory[0xCD] + 1

    @property
    def ball_in_play(self) -> int:
        # 1 byte BCD, just take lower nibble
        return self.memory[0x58] & 0xF

    def __str__(self) -> str:
        output = "System7Memory("
        output += f"\n\tplayers={self.num_players}"
        output += f"\n\tscores={self.player_scores}"
        output += f"\n\tplayer_up={self.player_up}"
        output += f"\n\tball_in_play={self.ball_in_play}"
        output += "\n)"
        return output


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