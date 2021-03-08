Z80 machines emulator - CPC6128, ZX48K

Usage for zx48k:
 - download 48.rom from somewhere
 - downlaod a TAP file
 - ./zx48k --rom 48.rom --tap file.tap
 - inside - do load "" (j rightshift-p rightshift-p enter)
 - press 'f7' to start tape
 - you can use 'f8' to start turbo mode to speed up tape load

Usage for cpc
 - download CPC6128.ROM, BASIC_1.1.ROM and AMSDOS_0.5.ROM
 - ./cpc --floppy floppy.dsk

Keys:
 - f5 - start trace
 - f6 - stop trace
 - f7 - start tape
 - f8 - turbo mode on/off

TODO:
 - Add missing CPU ops (what happens if Z80 gets a really invalid op?)
 - Do a manual z80 review  - bugs are visible eg. in Jet Set Willy
 - add PCM audio support 
 - add AY audio support
 - Add kempston joystick support
