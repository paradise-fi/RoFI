Import('env')
from platformio import util
import threading
from threading import Thread
from base64 import b64decode
import sys
import glob
import time

# Based on https://github.com/platformio/platformio-core/issues/1383

upload_cmd = env.subst('$UPLOADCMD')

def getPorts():
    simultaneous_upload_ports = ARGUMENTS.get("SIMULTANEOUS_UPLOAD_PORTS")
    ports = map(str.strip, b64decode(simultaneous_upload_ports).split(','))
    if ports[0] == "AUTO":
        ports = util.get_serial_ports()
    return ports

returnCodes=[]#list of tuples storing com port and upload result
def run(port):
    for i in range (5): # try up to 3 times
        command = upload_cmd.replace('--port ""', '--port "' + port + '"')
        command = command +" "+ env.subst('$BUILD_DIR/$PROGNAME') +".bin"
        errorCode = env.Execute(command)
        if errorCode == 0:
            returnCodes.append( (port, errorCode) )
            return
        time.sleep(2) # Wait between tries
    returnCodes.append((port,errorCode))

def multi_upload(source, target, env):
    print("Multi-target upload active on ports: ")
    for x in getPorts():
        print("|    " + x)
    print("")

    threads = []
    for port in getPorts() :
        thread = Thread(target=run, args=(port,))
        thread.start()
        threads.append(thread)
    for thread in threads:
        thread.join()
    encounteredError=False
    sorted(returnCodes, key=lambda code: code[0])

    print("")
    print("Upload result  -------------------------------------------------")
    for code in returnCodes:
        if(code[1]==0):
            print("|    " + code[0] + " Uploaded Successfully")
        elif(code[1]==1):
            print("|    " + code[0] + " Encountered Exception, Check serial port")
            encounteredError=True
        elif(code[1]==2):
            print("|    " + code[0] + " Encountered Fatal Error")
            encounteredError=True
    print("")
    if(encounteredError):
        Exit(1)

if ARGUMENTS.get("SIMULTANEOUS_UPLOAD_PORTS") > 0:
    env.Replace(UPLOADCMD="true")
    env.AddPreAction("upload", multi_upload)