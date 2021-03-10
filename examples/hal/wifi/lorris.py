# You can use terminal.clear() and terminal.appendText(string) to set term content
# You can use lorris.sendData(list) to send data to device.

# This function gets called on data received
# it should return string, which is automatically appended to terminal
def onDataChanged(data, dev, cmd, index):
    return ""

# This function is called on key press in terminal.
# Param is string
def onKeyPress(key):
    return

# This function is called when data arrives to serial port
# parameter is array with unparsed data
def onRawData(data):
    return

# Called when new widget is added.
# widget is widget's object, name is string
def onWidgetAdd(widget, name):
    return

# Called when new widget is removed.
# widget is widget's object, name is string
def onWidgetRemove(widget, name):
    return

# Called when this script instance is destroyed.
# useful for saving data.
def onScriptExit():
    return

# Called when this analyzer session is saved to data file.
# Useful for saving data.
def onSave():
    return
    
from time import sleep

def LeftShoe_valueChanged(val):
    lorris.sendData("move left {}\n".format(val))
    
def RightShoe_valueChanged(val):
    lorris.sendData("move right {}\n".format(val))
    
def leftMin_clicked():
    LeftShoe.setValue(-100)
    
def leftMid_clicked():
    LeftShoe.setValue(-4)
    
def leftMax_clicked():
    LeftShoe.setValue(94)
    
def rightMin_clicked():
    RightShoe.setValue(-100)
    
def rightMid_clicked():
    RightShoe.setValue(-4)
    
def rightMax_clicked():
    RightShoe.setValue(105)
    
def LeftConnE_clicked():
    terminal.appendText('Clicked\n')
    lorris.sendData("connector left expand\n")
    
def LeftConnR_clicked():
    terminal.appendText('Clicked\n')
    lorris.sendData("connector left retract\n")
    
def RightConnE_clicked():
    terminal.appendText('Clicked\n')
    lorris.sendData("connector right expand\n")
    
def RightConnR_clicked():
    terminal.appendText('Clicked\n')
    lorris.sendData("connector right retract\n")

timer = lorris.newTimer()
dataQueue = []

def onTimeout():
    global dataQueue
    if len(dataQueue) > 0:
        x = dataQueue.pop(0)
        if x and len(x) > 0:
            lorris.sendData(x)
    
timer.connect('timeout()', onTimeout);
timer.start(1000)

def macro1_clicked():
    global dataQueue
    dataQueue += ["move left -110\n", None, None]
    dataQueue += ["move right 90\n", None, None]
    dataQueue += ["connector left expand\n", None, None]
    
def macro2_clicked():
    global dataQueue
    dataQueue += ["connector right retract\n", None, None]
    dataQueue += ["move left 12\n", None, None]
    dataQueue += ["move right 0\n", None, None]
    
def macro3_clicked():
    global dataQueue
    dataQueue += ["move right -110\n", None, None]
    dataQueue += ["move left 95\n", None, None]
    dataQueue += ["connector right expand\n", None, None]
    
def macro4_clicked():
    global dataQueue
    dataQueue += ["connector left retract\n", None, None]
    dataQueue += ["move right 1\n", None, None]
    dataQueue += ["move left 0\n", None, None]
    
def reset_clicked():
    global dataQueue
    dataQueue += ["connector left retract\n"]
    dataQueue += ["connector right retract\n"]
    dataQueue += ["move left 0\n"]
    dataQueue += ["move right 0\n"]