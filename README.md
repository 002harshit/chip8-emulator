# Chip-8 Specs 
- [Specs Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- Memory   : 4kb of RAM
- Display  : 64 x 32 pixels
- PC       : aka 'program counter which points to current instruction, 
- I        : 16bit index register which is used to point at locations in memory
- Stack    : for the 16bit addresses
- Delay    : 8 bit delay timer, decremented at 60Hz until 0
- Sound    : 8 bit sound timer, same as above, also gives beep if not 0
- V0 to VF : 16 8bit variable register called 0 to F
- VF       : used as flag register

## Fonts
- in COSMAC VIP computer, Memory from 000 to 1FF  was used to store fonts
- Chip8 loads up program at 0x200
- fonts are builtin, represented by hex 0 to F. each font character is 4 x 5 pixels 
- store fonts in memory b/w 050-09F
- common way to represent font _>
```
0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
0x20, 0x60, 0x20, 0x20, 0x70, // 1
0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
0x90, 0x90, 0xF0, 0x10, 0x10, // 4
0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
0xF0, 0x10, 0x20, 0x40, 0x40, // 7
0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
0xF0, 0x90, 0xF0, 0x90, 0x90, // A
0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
0xF0, 0x80, 0x80, 0x80, 0xF0, // C
0xE0, 0x90, 0x90, 0x90, 0xE0, // D
0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
0xF0, 0x80, 0xF0, 0x80, 0x80  // F
```

## Display
- Pico8 display is monochrome, each pixel is boolean
- Sprites are 8bit wide and 1 to 15 bytes tall
- (Style) give fade out effect when pixels are changed

## Stack
- data structure that performs 'push' and 'pop' operation
- reserve some memory for the stack and variables you use
- atleast 16 2bytes entries

## Timers
- Delay and Sound timers
- Both are one byte each

## Key layout
```
1	2	3	C
4	5	6	D
7	8	9	E
A	0	B	F
```

## Cpu speed
- 700 chip8 instructions per second

## Fetch/Decode/Execute
- Fetch
	- reads the instruction from memory
	- an instruction is 2 bytes
	- increment PC by 2

- Decode
	- divided into 'nibble' aka 'half-byte'
	- first nibble is the type of  instruction
	- second nibble (X)   : used to lookup variable registers (V0 to VF)
	- third nibble  (Y)   : same function as (X)
	- fourth nibble (N)   : 4 bit number
	- second byte   (NN)  : 8 bit immediate number
	- 2nd, 3rd, 4th	(NNN) : 12 bit immediate memory address
	- X and Y are used to lookup values in register

- Execute
	- (just implement each instruction inside a switch case)

## Instruction
- I'm using [this] (http://devernay.free.fr/hacks/chip8/schip.txt) reference, it also specifies instructions for super chip8
- Instructions marked with (*) are new in SUPER-CHIP.
```
- [ ] 00CN* Scroll display N lines down
- [x] 00E0 Clear display 
- [x] 00EE Return from subroutine
- [ ] 00FB* Scroll display 4 pixels right
- [ ] 00FC* Scroll display 4 pixels left
- [ ] 00FD* Exit CHIP interpreter
- [ ] 00FE* Disable extended screen mode
- [ ] 00FF* Enable extended screen mode for full-screen graphics
- [x] 1NNN Jump to NNN
- [x] 2NNN Call subroutine at NNN
- [x] 3XKK Skip next instruction if VX == KK
- [x] 4XKK Skip next instruction if VX <> KK
- [x] 5XY0 Skip next instruction if VX == VY
- [x] 6XKK VX := KK
- [x] 7XKK VX := VX + KK
- [x] 8XY0 VX := VY, VF may change
- [x] 8XY1 VX := VX or VY, VF may change
- [x] 8XY2 VX := VX and VY, VF may change
- [x] 8XY3 VX := VX xor VY, VF may change
- [x] 8XY4 VX := VX + VY, VF := carry
- [x] 8XY5 VX := VX - VY, VF := not borrow
- [x] 8XY6 VX := VX shr 1, VF := carry
- [x] 8XY7 VX := VY - VX, VF := not borrow
- [x] 8XYE VX := VX shl 1, VF := carry
- [x] 9XY0 Skip next instruction if VX <> VY
- [x] ANNN I := NNN
- [x] BNNN Jump to NNN+V0                                       // NOT TESTED
- [x] CXKK VX := pseudorandom_number and KK                     // NOT TESTED
- [x] DXYN* Show N-byte sprite from M(I) at coords (VX,VY), VF := collision. If N=0 and extended mode, show 16x16 sprite.
- [x] EX9E Skip next instruction if key VX pressed
- [x] EXA1 Skip next instruction if key VX not pressed
- [x] FX07 VX := delay_timer
- [x] FX0A wait for keypress, store hex value of key in VX      // NOT TESTED
- [x] FX15 delay_timer := VX
- [x] FX18 sound_timer := VX
- [x] FX1E I := I + VX
- [x] FX29 Point I to 5-byte font sprite for hex character VX   // NOT TESTED
- [ ] FX30* Point I to 10-byte font sprite for digit VX (0..9)
- [x] FX33 Store BCD representation of VX in M(I)..M(I+2)       // NOT TESTED
- [x] FX55 Store V0..VX in memory starting at M(I)
- [x] FX65 Read V0..VX from memory starting at M(I)
- [ ] FX75* Store V0..VX in RPL user flags (X <= 7)
- [ ] FX85* Read V0..VX from RPL user flags (X <= 7) 
```

## Tests passed
- Test suite taken from https://github.com/Timendus/chip8-test-suite
- [x] 1-chip8-logo.ch8
- [x] 2-ibm-logo.ch8
- [x] 3-corax+.ch8
- [x] 4-flags.ch8
- [x] 5-quirks.ch8
- [x] 6-keypad.ch8
- [x] 7-beep.ch8
- [ ] 8-scrolling.ch8
