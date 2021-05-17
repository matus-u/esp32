import serial
import time

s = serial.Serial('/dev/ttyUSB0', baudrate=9600, bytesize=8, parity='N', stopbits=1, timeout=100, xonxoff=0, rtscts=0)
s.write(b"@7030WIFI=ivankassid,ivankapassSS\r\n")
print(s.readline().strip())
time.sleep(4)
s.write(b"@7030WSTAT?SS\r\n")
print(s.readline().strip())
s.close()