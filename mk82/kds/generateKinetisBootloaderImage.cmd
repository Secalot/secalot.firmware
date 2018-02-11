hexmerge.py --range=0x1000: --overlap=ignore --no-start-addr --output .\image.hex ..\fs.hex .\bootstrapper\Debug\bootstrapper.hex .\bootloader\Debug\bootloader.hex .\firmware\Debug\firmware.hex
hexmerge.py --range=:0xFFF --overlap=ignore --no-start-addr --output .\lock.hex .\bootstrapper\Debug\bootstrapper.hex
hex2bin.py --range=0x1000: image.hex image.bin
hex2bin.py --range=:0xFFF lock.hex lock.bin 

