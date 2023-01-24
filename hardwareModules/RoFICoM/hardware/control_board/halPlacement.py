import pcbnew
import math

CENTER_X = 140
CENTER_Y = 90
BASE_ROT = 180
ANGLE_OFFSET = 7.5
ANGLE_SPAN = 60
RADIUS = 19

board = pcbnew.GetBoard()

sensors = [x for x in board.GetFootprints() if x.GetReference().startswith("HAL")]
sensors.sort(key=lambda x: -int(x.GetReference().replace("HAL", "")))

for i, s in enumerate(sensors):
    angle = (-ANGLE_OFFSET + -i * ANGLE_SPAN / (len(sensors) - 1)) % 360
    rotation = (BASE_ROT - angle) % 360
    x = math.cos(angle * math.pi / 180) * RADIUS + CENTER_X
    y = math.sin(angle * math.pi / 180) * RADIUS + CENTER_Y
    s.SetPosition(pcbnew.wxPointMM(x, y))
    s.SetOrientation(pcbnew.EDA_ANGLE(rotation, pcbnew.DEGREES_T))
    print(f"s {x}, {y}")


pcbnew.Refresh()
