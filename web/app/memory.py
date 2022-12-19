import itertools
from typing import List

class System7Game:
    def __init__(self, data: bytes) -> None:
        self.snapshots = [
            System7Memory(data[(i * 1024):(i * 1024 + 1024)])
            for i in range(0, len(data) // 1024)
        ]

    @property
    def final_snapshot(self):
        return self.snapshots[-1]

    def num_balls(self):
        balls = max(s.ball_in_play for s in self.snapshots)
        if balls == 0:
            return 5
        return balls

    def num_players(self):
        return self.final_snapshot.num_players

    def scores_at_ball(self, ball):
        for s in self.snapshots:
            if s.ball_in_play == ball:
                return s.player_scores
            elif s.ball_in_play > ball:
                break
        return None
    

class System7Memory:
    def __init__(self, memory: bytes) -> None:
        assert len(memory) == 1024
        self.memory = memory

    @property
    def num_players(self) -> int:
        raw = self.memory[0xCC]
        if raw <= 3:
            return raw + 1
        else:
            # The player count isn't right.
            # Most likely, it's 0xFF, uninitialized memory.
            # Try to infer this from the player scores.
            num_players = 0
            for i in range(0, 4):
                addr = 0x38 + (i * 4)
                data = self.memory[addr:(addr + 4)]
                if data != bytes([0xFF, 0xFF, 0xFF, 0xFF]):
                    num_players += 1
                else:
                    break
            return num_players

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