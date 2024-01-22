import random
import os
import sys

sectors  = 0
build    = False
dump     = False
noise    = False
bootable = False
filename = ''

skip = 0
for argi in range(1, len(sys.argv)):
    if skip > 0:
        skip -= 1
        continue

    args = sys.argv[argi]
    if args == '-h' or args == '--help':
        print(
            'usage: %s [options] filename\n'
            'options:\n'
            '  -h, --help    : print this message.\n'
            '  -s, --sectors : number of sectors to build or dump.\n'
            '  -d, --dump    : dump the image.\n'
            '  -n, --noise   : random image content.\n'
            '  -b, --build   : build the image.\n'
            '  -t, --boot    : bootable image.\n'
            % sys.argv[0]
        )
        os._exit(0)

    if args == '-s' or args == '--sector':
        if argi + 1 < len(sys.argv):
            sectors = int(sys.argv[argi + 1])
            skip = 1
        else:
            print('error: missing argument for option %s' % args)
    elif args == '-b' or args == '--build':
        build = True
    elif args == '-d' or args == '--dump':
        dump = True
    elif args == '-n' or args == '--noise':
        noise = True
    elif args == '-t' or args == '--boot':
        bootable = True
    else:
        filename = args

if filename is None:
    print('fatal: no input')
    os._exit(1)

if build is True:
    print('Building... ', end = '')
    with open(filename, 'wb') as file:
        for sector in range(sectors):
            for offset in range(0, 512, 4):
                if noise is True:
                    if bootable is True and sector == 0 and offset == 0:
                        file.write(b'\x45\x70\xFE\xED')
                    else:
                        file.write(b'%c%c%c%c' % (random.randint(0x00, 0xFF), random.randint(0x00, 0xFF), random.randint(0x00, 0xFF), random.randint(0x00, 0xFF)))
                else:
                    if bootable is True and sector == 0 and offset == 0:
                        file.write(b'\x45\x70\xFE\xED')
                    else:
                        file.write(b'\x00\x00\x00\x00')
    print('done')

if dump is True:
    cdrom_size    = os.stat(filename).st_size
    cdrom_sectors = cdrom_size // 512

    if sectors > cdrom_sectors or sectors == 0:
        sectors = cdrom_sectors

    print('Dumping %u sectors...' % sectors)
    with open(filename, 'rb') as file:
        for sector in range(sectors):
            print('sector 0x%08X' % sector)
            buffer = file.read(512)
            for offset in range(0, 512, 16):
                print('0x%08X |' % offset, end = '')
                for index in range(16):
                    print(' %02X' % buffer[offset + index], end = '')
                print('    ', end = '')
                for index in range(16):
                    c = buffer[offset + index]
                    if 0x20 <= c and c <= 0x7E:
                        print('%c' % chr(c), end = '')
                    else:
                        print('.', end = '')
                print()
