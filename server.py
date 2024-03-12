import serial

s = serial.Serial('/dev/ttyUSB0', baudrate=9600, bytesize=8, parity='N', stopbits=1, timeout=100, xonxoff=0, rtscts=0)
#s = serial.Serial('/dev/ttyUSB0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=100, xonxoff=0, rtscts=0)
#s.write(b"@ff70?72\r\n")
#s.write(b"@ff70*WIFI=ivankassid,ivankapassSS\r\n")
#print(s.readline().strip())
#
while True:
    r = s.readline().strip()
    decoded = r.decode('utf-8')
    print("str {}".format(decoded))
    if decoded.startswith("@S?"):
        print ("SENDING BACK")
        toWrite = b"@S=192,168,005,067,192,168,005,001,255,255,255,000,10001,TP-LINK_B2A4" + bytes(8*[0x80])+b",910426mativa" + bytes(8*[0x80]) + b"\r\n"
        s.write(toWrite)
        while True:
            r = s.readline().strip()
            decoded = r.decode('utf-8')
            print("str {}".format(decoded))
            toWrite = b"Test string\r\n"
            s.write(toWrite)
s.close()
