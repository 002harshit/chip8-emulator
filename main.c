#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// for window creation and key events
#include "fenster.h"

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int64_t i64;

#define TERM_WHITE "\e[0;37m"
#define TERM_RED "\e[1;31m"
#define TERM_BLUE "\e[1;36m"

#define printerr(...)                                                                              \
	fprintf(stderr, TERM_RED "ERROR: " TERM_WHITE __VA_ARGS__);                                    \
	printf("\n")
#define printfatal(...)                                                                            \
	fprintf(stdout, TERM_BLUE "FATAL: " TERM_WHITE __VA_ARGS__);                                   \
	printf("\n");                                                                                  \
	exit(1)
#define printlog(...)                                                                              \
	fprintf(stdout, TERM_BLUE "LOGS: " TERM_WHITE __VA_ARGS__);                                    \
	printf("\n")

// C8 Display Dimentions
#define C8_WIDTH 64
#define C8_HEIGHT 32

// Actual Dimentions
#define WND_SCALE 10
#define WND_WIDTH (WND_SCALE * C8_WIDTH)
#define WND_HEIGHT (WND_SCALE * C8_HEIGHT)

// between 0.0 to 1.0
#define COLOR_FADE 0.075
#define BG_COLOR 0x424BF1
#define FG_COLOR 0xECF3E5

// IPS: Instruction per secound
#define C8_CPU_IPS 1000

#define C8_FONT_START 0x000
#define C8_CODE_START 0x200

u8 c8_memory[1024 * 4]; // 4 kilobytes
u8 c8_sound_timer;
u8 c8_delay_timer;
u16 c8_pc = C8_CODE_START;
u16 c8_i;
u8 c8_v[16];
u16 c8_stack_top;
u16 c8_stack[16];
u8 c8_pixels[C8_WIDTH * C8_HEIGHT] = {0};
// clang-format off
u16 c8_keys[16] = {
	'X', '1', '2', '3',
	'Q', 'W', 'E', 'A',
	'S', 'D', 'Z', 'C',
	'4', 'R', 'F', 'V'
};

u32 wnd_buf[WND_WIDTH * WND_HEIGHT] = {0};
struct fenster wnd = {
	.title = "Chip8 Emulator",
	.width = WND_WIDTH,
	.height = WND_HEIGHT,
	.buf = wnd_buf
};
float fade = 0.0;
// clang-format on
int should_quit;

void open_window()
{
	for (u32 i = 0; i < WND_WIDTH * WND_HEIGHT; i++) {
		wnd_buf[i] = BG_COLOR;
	}
	fenster_open(&wnd);
	printlog("Opened a window");
}

void close_screen()
{
	fenster_close(&wnd);
	printlog("Closed Screen");
}

#define LERP(a, b, t) ((1.0 - (t)) * (a) + (t) * (b))
#define _COL_LERP(a, b, t) ((u32)LERP(a, b, t))

u32 color_lerp(u32 x, u32 y, float t)
{
	u8 xr = (x >> 16) & 0xFF;
	u8 xb = (x >> 8) & 0xFF;
	u8 xg = x & 0xFF;
	u8 yr = (y >> 16) & 0xFF;
	u8 yb = (y >> 8) & 0xFF;
	u8 yg = y & 0xFF;
	return (_COL_LERP(xr, yr, t) << 16) | (_COL_LERP(xb, yb, t) << 8) | (_COL_LERP(xg, yg, t));
}
void set_pixel(u32 x, u32 y, u32 color)
{
	fenster_pixel(&wnd, x, y) = color_lerp(fenster_pixel(&wnd, x, y), color, COLOR_FADE);
}

void render_screen()
{
	for (u32 y = 0; y < WND_HEIGHT; y++) {
		u32 py = y / WND_SCALE;
		for (u32 x = 0; x < WND_WIDTH; x++) {
			u32 px = x / WND_SCALE;
			u32 color = c8_pixels[py * C8_WIDTH + px] != 0 ? FG_COLOR : BG_COLOR;
			set_pixel(x, y, color);
		}
	}
}

void update_screen() { should_quit = should_quit || fenster_loop(&wnd) != 0; }

void set_fonts()
{
	// clang-format off
	u8 c8_fonts[5 * 16] = {
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
	};
	// clang-format on
	for (u16 i = 0; i < 16 * 5; i++) {
		c8_memory[C8_FONT_START + i] = c8_fonts[i];
	}
	printlog("Loaded fonts");
}

int is_key_pressed(u16 index) { return wnd.keys[c8_keys[index]]; }

void c8_fetch_decode_execute()
{

	// fetch
	u16 buffer = ((u16 *)c8_memory)[c8_pc >> 1];
	c8_pc += 2;

	// decode
	// NOTE:  chip8 stores its instruction as little endian
	u16 nib1 = (0x00f0 & buffer) >> 0x4;
	u16 nib2 = (0x000f & buffer) >> 0x0;
	u16 nib3 = (0xf000 & buffer) >> 0xc;
	u16 nib4 = (0x0f00 & buffer) >> 0x8;
	u16 imme_num = (0xff00 & buffer) >> 0x8;
	u16 mem_addr = ((0x000f & buffer) << 0x8) | imme_num;
	u16 instr = (nib1 << 0xc) | mem_addr;

// execute
#define NOT_IMPLEMENTED printfatal("NOT IMPLEMENTED <%04x>", instr)
	switch (nib1) {
	case 0x0:
		if (instr == 0x00e0) {
			// 00E0
			for (u8 py = 0; py < C8_HEIGHT; py++) {
				for (u8 px = 0; px < C8_WIDTH; px++) {
					c8_pixels[py * C8_WIDTH + px] = 0;
				}
			}
			fade = 0.0;
			render_screen();
		} else if (instr == 0x00ee) {
			// 00EE
			if (c8_stack_top < 1) {
				printfatal("Stack Underflow");
			}
			c8_pc = c8_stack[--c8_stack_top];
		} else {
			NOT_IMPLEMENTED;
		}
		break;
	case 0x1:
		// 1NNN
		c8_pc = mem_addr;

		break;
	case 0x2:
		// 2NNN
		if (c8_stack_top > 15) {
			printfatal("Stack Overflow");
		}
		c8_stack[c8_stack_top++] = c8_pc;
		c8_pc = mem_addr;
		break;
	case 0x3:
		// 3XKK
		if (c8_v[nib2] == imme_num) {
			c8_pc += 2;
		}
		break;
	case 0x4:
		// 4XKK
		if (c8_v[nib2] != imme_num) {
			c8_pc += 2;
		}
		break;
	case 0x5:
		// 5XY0
		if (c8_v[nib2] == c8_v[nib3]) {
			c8_pc += 2;
		}
		break;
	case 0x6:
		// 6XKK
		c8_v[nib2] = imme_num;
		break;
	case 0x7:
		// 7XKK
		c8_v[nib2] = c8_v[nib2] + imme_num;
		break;
	case 0x8:
		switch (nib4) {
		case 0x0:
			// 8XY0
			c8_v[nib2] = c8_v[nib3];
			break;
		case 0x1:
			// 8XY1
			c8_v[nib2] = c8_v[nib2] | c8_v[nib3];
			break;
		case 0x2:
			// 8XY2
			c8_v[nib2] = c8_v[nib2] & c8_v[nib3];
			break;
		case 0x3:
			// 8XY3
			c8_v[nib2] = c8_v[nib2] ^ c8_v[nib3];
			break;
		case 0x4: {
			// 8XY4
			u8 flag = c8_v[nib2] + c8_v[nib3] > (u16)255;
			c8_v[nib2] = c8_v[nib2] + c8_v[nib3];
			c8_v[0xF] = flag;
			break;
		}
		case 0x5: {
			// 8XY5
			u8 flag = c8_v[nib2] >= c8_v[nib3];
			c8_v[nib2] = c8_v[nib2] - c8_v[nib3];
			c8_v[0xF] = flag;
			break;
		}
		case 0x6: {
			// 8XY6
			u8 flag = (c8_v[nib2] & (1 << 0)) >> 0;
			c8_v[nib2] = c8_v[nib3] >> 1;
			c8_v[0xF] = flag;
			break;
		}
		case 0x7: {
			// 8XY7
			u8 flag = c8_v[nib3] >= c8_v[nib2];
			c8_v[nib2] = c8_v[nib3] - c8_v[nib2];
			c8_v[0xF] = flag;
			break;
		}
		case 0xE: {
			// 8XYE
			u8 flag = (c8_v[nib2] & (1 << 7)) >> 7;
			c8_v[nib2] = c8_v[nib3] << 1;
			c8_v[0xF] = flag;
			break;
		}
		default:
			NOT_IMPLEMENTED;
		}
		break;
	case 0x9:
		// 9XY0
		if (c8_v[nib2] != c8_v[nib3]) {
			c8_pc += 2;
		}
		break;
	case 0xA:
		// ANNN
		c8_i = mem_addr;
		break;
	case 0xB:
		// BNNN
		c8_pc = mem_addr + c8_v[0];
	case 0xC:
		// CXKK
		c8_v[nib2] = rand() + imme_num;
	case 0xD: {
		// DXYN
		c8_v[0xF] = 0;
		u8 y = c8_v[nib3] % C8_HEIGHT;
		for (u16 row = 0; row < nib4; row++) {
			u8 sprite = c8_memory[c8_i + row];
			if (y >= C8_HEIGHT) {
				break;
			}
			u8 x = c8_v[nib2] % C8_WIDTH;
			for (u16 col = 0; col < 8; col++) {
				if (x >= C8_WIDTH) {
					break;
				}
				u8 pixel = ((sprite & (1 << (7 - col))) >> (7 - col));
				if (c8_pixels[y * C8_WIDTH + x] && !pixel) {
					c8_v[0xF] = 1;
				}
				c8_pixels[y * C8_WIDTH + x] = c8_pixels[y * C8_WIDTH + x] ^ pixel;

				x++;
			}
			y++;
		}
		fade = 0.0;
		render_screen();
		break;
	}
	case 0xE:
		switch (imme_num) {
		case 0x9E:
			if (is_key_pressed(c8_v[nib2])) {
				c8_pc += 2;
			}
			break;
		case 0xA1:
			if (!is_key_pressed(c8_v[nib2])) {
				c8_pc += 2;
			}
			break;
		default:
			NOT_IMPLEMENTED;
		}
		break;
	case 0xF:
		switch (imme_num) {
		case 0x07:
			// FX07
			c8_v[nib2] = c8_delay_timer;
			break;
		case 0x0A:
			// FX0A
			for (u8 i = 0; i < 16; i++) {
				if (is_key_pressed(i)) {
					c8_v[nib2] = i;
					break;
				}
			}
			c8_pc -= 2;
			break;

		case 0x15:
			// FX15
			c8_delay_timer = c8_v[nib2];
			break;
		case 0x18:
			// FX18
			c8_sound_timer = c8_v[nib2];
			break;
		case 0x1E:
			// FX1E
			c8_i = c8_i + c8_v[nib2];
			break;
		case 0x29:
			// FX29
			c8_i = C8_FONT_START + (0x0f & c8_v[nib2]) * 5;
			break;
		case 0x33:
			// FX33
			for (u16 i = 0, p = 100; i < 3; i++, p /= 10) {
				c8_memory[c8_i + i] = (c8_v[nib2] / p) % 10;
			}
			// break;
		case 0x55:
			// FX55
			for (u8 i = 0; i <= nib2; i++) {
				c8_memory[c8_i + i] = c8_v[i];
			}
			break;
		case 0x65:
			// FX65
			for (u8 i = 0; i <= nib2; i++) {
				c8_v[i] = c8_memory[c8_i + i];
			}
			break;

		default:
			NOT_IMPLEMENTED;
		}
		break;
	default:
		NOT_IMPLEMENTED;
	}
}

void load_program(const char *filepath)
{
	FILE *file = fopen(filepath, "rb");
	if (!file) {
		printfatal("Given file not found at path: %s", filepath);
	}
	fseek(file, 0, SEEK_END);
	size_t fsize = ftell(file);
	rewind(file);
	if (fsize % sizeof(u8) != 0) {
		fclose(file);
		printfatal("File is not divisible by u8 size\n");
	}
	size_t bytes_read = fread(&c8_memory[C8_CODE_START], sizeof(u8), fsize, file);
	if (bytes_read * sizeof(u8) != fsize) {
		fclose(file);
		printfatal("Bytes read: %zu, Actual size: %zu", bytes_read * sizeof(u8), fsize);
	}
	fclose(file);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printfatal("Provide input filepath as first arguement");
	}
	set_fonts();
	load_program(argv[1]);
	open_window();
	i64 time_now = fenster_time();
	i64 one_sec_timer = time_now;
	i64 hz60_timer = time_now;
	while (!should_quit) {
		time_now = fenster_time();
		update_screen();
		c8_fetch_decode_execute();
		if (time_now - hz60_timer > 1000.0 / 60.0) {
			hz60_timer = time_now;
			if (fade < 5.0) {
				render_screen();
				fade += COLOR_FADE;
			}
		}
		if (time_now - one_sec_timer > 1000.0) {
			one_sec_timer = time_now;
			c8_delay_timer = c8_delay_timer < 1 ? 0 : c8_delay_timer - 1;
			c8_sound_timer = c8_sound_timer < 1 ? 0 : c8_sound_timer - 1;
			if (c8_sound_timer > 0) {
				printf("\a");
				fflush(stdout);
			}
		}
	}

	close_screen();
	return 0;
}
