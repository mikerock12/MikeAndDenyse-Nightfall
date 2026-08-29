"""Mirrors Progress + LanIp rules so we can fail before a Unity build."""

def can_play(unlocked, i):
    return 0 <= i < unlocked <= 16 and i < 16

def on_clear(unlocked, i):
    return max(unlocked, min(16, i + 2))

def score_ip(s):
    b = [int(x) for x in s.split(".")]
    if b[0] in (0, 127, 255):
        return -1
    if b[0] == 169 and b[1] == 254:
        return 1
    if b[0] == 192 and b[1] == 168:
        return 40
    if b[0] == 10:
        return 30
    if b[0] == 172 and 16 <= b[1] <= 31:
        return 28
    return 10

def usable(s):
    if not s:
        return False
    return score_ip(s) >= 10

def main():
    assert can_play(1, 0) and not can_play(1, 1)
    u = 1
    u = on_clear(u, 0)
    assert u == 2 and can_play(u, 1)
    u = on_clear(u, 0)
    assert u == 2
    for i in range(1, 15):
        u = on_clear(u, i)
    assert u == 16
    u = on_clear(u, 15)
    assert u == 16
    assert score_ip("127.0.0.1") < 0
    assert score_ip("169.254.1.1") < 10
    assert score_ip("192.168.0.14") >= 30
    assert usable("192.168.1.5") and not usable("127.0.0.1") and not usable("")
    print("OK progress+lanip")

if __name__ == "__main__":
    main()
