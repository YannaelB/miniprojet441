
import sys
from pygame.locals import *
from paddle import Paddle
from puck import Puck
from startScreen import air_hockey_start, disp_text
from themeScreen import theme_screen
from globals import *
from endScreen import game_end


import serial
import serial.tools.list_ports

import struct

def bytes_to_int(byte1, byte2):
    # Créez une séquence de deux octets à partir de byte1 et byte2
    bytes_sequence = struct.pack('BB', byte1, byte2)

    # Déballez la séquence en un entier (utilisez 'H' pour un entier non signé sur 2 octets)
    result = struct.unpack('>H', bytes_sequence)[0]

    return result

#http://lense.institutoptique.fr/mine/python-pyserial-premier-script/

ser = serial.Serial(port='COM6', baudrate=115200,bytesize = 8,parity='N', stopbits=1, timeout=None,  write_timeout=None, xonxoff=False, rtscts=False, dsrdtr=False)
print("here before read")

while(True):

    while(ser.in_waiting != 1):
        pass
        #print("Wait1 !")
    read_bytes = ser.read(1)
    print(" read_bytes decodé = ", read_bytes.decode())

    if read_bytes.decode() == 'd':

        while(ser.in_waiting != 2):
            pass
            #print("Wait2 !")
        read_bytes = ser.read(2)

        print(" read_bytes  = ", read_bytes)
        print(" read_bytes [0] = ", read_bytes[0])
        print(" read_bytes [-1] = ", read_bytes[1])

        entier_c = bytes_to_int(read_bytes[1], read_bytes[0])
        print("entier c = ", entier_c)

    elif read_bytes.decode() == 'g':
        while(ser.in_waiting != 2):
            pass
            #print("Wait2 !")
        read_bytes = ser.read(2)

        print(" read_bytes  = ", read_bytes)
        print(" read_bytes [0] = ", read_bytes[0])
        print(" read_bytes [-1] = ", read_bytes[1])

        entier_c = bytes_to_int(read_bytes[1], read_bytes[0])
        print("entier c = ", entier_c)
    else:
        pass



    # print(" read_bytes [2-4] = ", read_bytes[2:4])



    # print(" here after read")
    # print(" read_bytes  [2]= ", int(read_bytes[2]))
    # print(" read_bytes decodé = ", read_bytes.decode())
ser.close()
