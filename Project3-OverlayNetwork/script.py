import struct
import socket

ip_txt = '5.6.7.8'
content = 'Just testing if this works\n'
content_len = len(content)

with open('tosend.bin', 'wb') as outF:
	outF.write(socket.inet_aton(ip_txt))
	outF.write(struct.pack('i', content_len))
	outF.write(str.encode(content))
