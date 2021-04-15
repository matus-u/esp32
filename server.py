import serial

s = serial.Serial('/dev/ttyUSB0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=100, xonxoff=0, rtscts=0)
s.write(b"TAKE\n")
length = int(s.readline().strip())
data = s.read(length)
s.close()
print (length)
print (data)
with open("test.jpeg", "wb") as f:
    f.write(data)
