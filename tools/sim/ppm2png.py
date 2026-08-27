import sys, zlib, struct, glob, os
def conv(src):
    d = open(src,'rb').read()
    parts = d.split(b'\n',3)
    w,h = map(int, parts[1].split())
    px = parts[3]
    raw = b''.join(b'\x00' + px[y*w*3:(y+1)*w*3] for y in range(h))
    def chunk(t,data):
        c = t+data
        return struct.pack('>I',len(data))+c+struct.pack('>I',zlib.crc32(c)&0xffffffff)
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))
           + chunk(b'IDAT',zlib.compress(raw,9)) + chunk(b'IEND',b''))
    dst = src[:-4]+'.png'
    open(dst,'wb').write(png)
    os.remove(src)
for f in glob.glob(sys.argv[1]): conv(f)
