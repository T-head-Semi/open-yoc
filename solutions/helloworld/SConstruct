#! /bin/env python

from yoctools import Make

defconfig = Make(elf='yoc.elf', objcopy='yoc.bin', objdump='yoc.asm')

Export('defconfig')

defconfig.build_components()
