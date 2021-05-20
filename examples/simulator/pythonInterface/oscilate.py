from pyRofiHal import RoFI
import threading
from math import radians

"""
Simple showcase that possesses one of the RoFIs in simulation, chooses the first
joint and moves it between its limits forever.
"""

def run():
    r = RoFI.getLocal() # Or use RoFI.getRemote(<id>) to get a specific RoFI
    j = r.getJoint(0)
    event = threading.Event()
    while True:
        event.clear()
        j.setPosition(j.minPosition, radians(90), lambda j: event.set())
        event.wait()
        event.clear()
        j.setPosition(j.maxPosition, radians(90), lambda j: event.set())
        event.wait()

if __name__ == "__main__":
    run()
