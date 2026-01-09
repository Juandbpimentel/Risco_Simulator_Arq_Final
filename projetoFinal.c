/* Risc-O Simulator 
    Nome dos integrantes da equipe:
    - Integrante 1: Ryan Lopes Braga Brito, Matrícula: 578267
    - Integrante 2: João Alves Lima de Holanda, Matrícula: 500811
    - Integrante 3: Ariel Andrade Lima, Matrícula: 582428
    - Integrante 4: Ryan Levy Bizerra Pimentel, Matrícula: 578267
    - Integrante 5: Juan David Bizerra Pimentel, Matrícula: 557340
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* Risc-O memory size */
#define RISCO_MEM_SIZE (0x2000)

/* Opcodes (low 4 bits) */
#define OP_JMP (0x0)
#define OP_JEQ_JNE_JLT_JGT (0x1)
#define OP_LDR (0x2)
#define OP_STR (0x3)
#define OP_MOV (0x4)
#define OP_ADD (0x5)
#define OP_ADDI (0x6)
#define OP_SUB (0x7)
#define OP_SUBI (0x8)
#define OP_AND (0x9)
#define OP_OR (0xA)
#define OP_SHR (0xB)
#define OP_SHL (0xC)
#define OP_CMP (0xD)
#define OP_PUSH (0xE)
#define OP_POP (0xF)

#define INSTR_HALT (0xFFFF)

/* IO map */
#define IO_CHAR_IN (0xF000)
#define IO_CHAR_OUT (0xF001)
#define IO_INT_IN (0xF002)
#define IO_INT_OUT (0xF003)

typedef struct risco_t {
	uint16_t reg[16];
	uint16_t memory[RISCO_MEM_SIZE];
	uint16_t next_pc;
	uint8_t flagZ;
	uint8_t flagC;
	bool accessed[RISCO_MEM_SIZE];
} risco_t;

static uint16_t get_pc(const risco_t *cpu) {
	return cpu->reg[15];
}

static void set_next_pc(risco_t *cpu, uint16_t value) {
	cpu->next_pc = value;
}

static void confirm_next_pc(risco_t *cpu) {
	cpu->reg[15] = cpu->next_pc;
}

static uint16_t get_sp(const risco_t *cpu) {
	return cpu->reg[14];
}

static void set_sp(risco_t *cpu, uint16_t sp) {
	cpu->reg[14] = sp;
}

static int16_t offset12_from_ibr(uint16_t ibr) {
	uint16_t raw = (uint16_t)((ibr >> 4) & 0x0FFF);
	if (raw & 0x0800) {
		raw |= 0xF000;
	}
	return (int16_t)raw;
}

static int16_t offset10_from_ibr(uint16_t ibr) {
	uint16_t raw = (uint16_t)((ibr >> 4) & 0x03FF);
	if (raw & 0x0200) {
		raw |= 0xFC00;
	}
	return (int16_t)raw;
}

static uint16_t imm8_from_ibr_signed(uint16_t ibr) {
	uint16_t raw = (uint16_t)((ibr >> 4) & 0x00FF);
	if (raw & 0x0080) {
		raw |= 0xFF00;
	}
	return raw;
}

static int io_char_read(void) {
	unsigned char ch = 0;
	if (scanf(" %c", &ch) != 1) {
		return 0;
	}
	printf("IN => %d\n", ch);
	return (int)ch;
}

static int io_int_read(void) {
	int value = 0;
	if (scanf("%d", &value) != 1) {
		value = 0;
	}
	printf("IN => %d\n", value);
	return value;
}

static void io_char_write(int value) {
	printf("OUT <= %c\n", (char)(value & 0xFF));
}

static void io_int_write(int value) {
	printf("OUT <= %d\n", value);
}

static uint16_t mem_read(risco_t *cpu, uint16_t addr) {
	addr &= 0xFFFF;
	if (addr == IO_CHAR_IN) {
		return (uint16_t)(io_char_read() & 0xFFFF);
	}
	if (addr == IO_INT_IN) {
		return (uint16_t)(io_int_read() & 0xFFFF);
	}

	if (addr < RISCO_MEM_SIZE) {
		cpu->accessed[addr] = true;
		return cpu->memory[addr];
	}

	return 0;
}

static void mem_write(risco_t *cpu, uint16_t addr, uint16_t value) {
	addr &= 0xFFFF;
	value &= 0xFFFF;
	if (addr == IO_CHAR_OUT) {
		io_char_write(value);
		return;
	}
	if (addr == IO_INT_OUT) {
		io_int_write((int16_t)value);
		return;
	}

	if (addr < RISCO_MEM_SIZE) {
		cpu->accessed[addr] = true;
		cpu->memory[addr] = value;
	}
}

static void instr_jmp(risco_t *cpu, uint16_t ibr) {
	/* Offset de JMP está em [15:4] (12 bits) com sinal. */
	int16_t offset = offset12_from_ibr(ibr);
	int32_t dest = (int32_t)get_pc(cpu) + offset;
	set_next_pc(cpu, (uint16_t)dest);
}

static void instr_jeq(risco_t *cpu, uint16_t ibr) {
	int16_t offset = offset10_from_ibr(ibr);
	if (cpu->flagZ == 1) {
		int32_t dest = (int32_t)get_pc(cpu) + offset;
		set_next_pc(cpu, (uint16_t)dest);
	}
}

static void instr_jne(risco_t *cpu, uint16_t ibr) {
	int16_t offset = offset10_from_ibr(ibr);
	if (cpu->flagZ == 0) {
		int32_t dest = (int32_t)get_pc(cpu) + offset;
		set_next_pc(cpu, (uint16_t)dest);
	}
}

static void instr_jlt(risco_t *cpu, uint16_t ibr) {
	int16_t offset = offset10_from_ibr(ibr);
	if (cpu->flagZ == 0 && cpu->flagC == 1) {
		int32_t dest = (int32_t)get_pc(cpu) + offset;
		set_next_pc(cpu, (uint16_t)dest);
	}
}

static void instr_jge(risco_t *cpu, uint16_t ibr) {
	int16_t offset = offset10_from_ibr(ibr);
	if (cpu->flagZ == 1 || cpu->flagC == 0) {
		int32_t dest = (int32_t)get_pc(cpu) + offset;
		set_next_pc(cpu, (uint16_t)dest);
	}
}

static void instr_jmp_special(risco_t *cpu, uint16_t ibr) {
	uint16_t cond = (uint16_t)((ibr >> 14) & 0x0003);
	if (cond == 0x0) {
		instr_jeq(cpu, ibr);
	} else if (cond == 0x1) {
		instr_jne(cpu, ibr);
	} else if (cond == 0x2) {
		instr_jlt(cpu, ibr);
	} else if (cond == 0x3) {
		instr_jge(cpu, ibr);
	}
}

static void instr_push(risco_t *cpu, uint16_t ibr) {
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);
	set_sp(cpu, (uint16_t)(get_sp(cpu) - 1));
	if (get_sp(cpu) < RISCO_MEM_SIZE) {
		cpu->memory[get_sp(cpu)] = cpu->reg[rn];
	}
}

static void instr_pop(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t value = 0;
	if (get_sp(cpu) < RISCO_MEM_SIZE) {
		value = cpu->memory[get_sp(cpu)];
	}
	set_sp(cpu, (uint16_t)(get_sp(cpu) + 1));

	if (rd == 15) {
		set_next_pc(cpu, value);
	} else {
		cpu->reg[rd] = value;
	}
}

static void instr_add(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	uint32_t result = (uint32_t)cpu->reg[rm] + (uint32_t)cpu->reg[rn];
	cpu->flagC = (uint8_t)(result > 0xFFFF ? 1 : 0);
	cpu->reg[rd] = (uint16_t)(result & 0xFFFF);
	cpu->flagZ = (uint8_t)(cpu->reg[rd] == 0 ? 1 : 0);
}

static void instr_addi(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t imm = (uint16_t)((ibr >> 4) & 0x000F);

	uint32_t result = (uint32_t)cpu->reg[rm] + (uint32_t)imm;
	cpu->flagC = (uint8_t)(result > 0xFFFF ? 1 : 0);
	cpu->reg[rd] = (uint16_t)(result & 0xFFFF);
	cpu->flagZ = (uint8_t)(cpu->reg[rd] == 0 ? 1 : 0);
}

static void instr_sub(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	cpu->flagC = (uint8_t)(cpu->reg[rm] < cpu->reg[rn] ? 1 : 0);
	uint16_t res = (uint16_t)((cpu->reg[rm] - cpu->reg[rn]) & 0xFFFF);
	cpu->reg[rd] = res;
	cpu->flagZ = (uint8_t)(res == 0 ? 1 : 0);
}

static void instr_subi(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t imm = (uint16_t)((ibr >> 4) & 0x000F);

	cpu->flagC = (uint8_t)(cpu->reg[rm] < imm ? 1 : 0);
	uint16_t res = (uint16_t)((cpu->reg[rm] - imm) & 0xFFFF);
	cpu->reg[rd] = res;
	cpu->flagZ = (uint8_t)(res == 0 ? 1 : 0);
}

static void instr_and(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	uint16_t res = (uint16_t)(cpu->reg[rm] & cpu->reg[rn]);
	cpu->reg[rd] = (uint16_t)(res & 0xFFFF);
	cpu->flagZ = (uint8_t)(res == 0 ? 1 : 0);
	cpu->flagC = 0;
}

static void instr_or(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	uint16_t res = (uint16_t)(cpu->reg[rm] | cpu->reg[rn]);
	cpu->reg[rd] = (uint16_t)(res & 0xFFFF);
	cpu->flagZ = (uint8_t)(res == 0 ? 1 : 0);
	cpu->flagC = 0;
}

static void instr_shr(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t imm = (uint16_t)((ibr >> 4) & 0x000F);

	uint16_t val = cpu->reg[rm];
	uint16_t res = (imm > 0) ? (uint16_t)(val >> imm) : val;
	cpu->flagC = (uint8_t)((imm > 0) ? ((val >> (imm - 1)) & 1u) : 0);
	cpu->flagZ = (uint8_t)(res == 0 ? 1 : 0);
	cpu->reg[rd] = (uint16_t)(res & 0xFFFF);
}

static void instr_shl(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t imm = (uint16_t)((ibr >> 4) & 0x000F);

	uint32_t result = (uint32_t)cpu->reg[rm] << imm;
	cpu->flagC = (uint8_t)(result > 0xFFFF ? 1 : 0);
	cpu->flagZ = (uint8_t)(((uint16_t)result) == 0 ? 1 : 0);
	cpu->reg[rd] = (uint16_t)(result & 0xFFFF);
}

static void instr_cmp(risco_t *cpu, uint16_t ibr) {
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	if (cpu->reg[rm] == cpu->reg[rn]) {
		cpu->flagZ = 1;
		cpu->flagC = 0;
	} else if (cpu->reg[rm] < cpu->reg[rn]) {
		cpu->flagZ = 0;
		cpu->flagC = 1;
	} else {
		cpu->flagZ = 0;
		cpu->flagC = 0;
	}
}

static void instr_mov(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t imm = imm8_from_ibr_signed(ibr);
	cpu->reg[rd] = imm;
}

static void instr_ldr(risco_t *cpu, uint16_t ibr) {
	uint16_t rd = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t imm = (uint16_t)((ibr >> 4) & 0x000F);

	uint16_t addr = (uint16_t)((cpu->reg[rm] + imm) & 0xFFFF);
	cpu->reg[rd] = (uint16_t)(mem_read(cpu, addr) & 0xFFFF);
}

static void instr_str(risco_t *cpu, uint16_t ibr) {
	uint16_t imm = (uint16_t)((ibr >> 12) & 0x000F);
	uint16_t rm = (uint16_t)((ibr >> 8) & 0x000F);
	uint16_t rn = (uint16_t)((ibr >> 4) & 0x000F);

	uint16_t addr = (uint16_t)((cpu->reg[rm] + imm) & 0xFFFF);
	mem_write(cpu, addr, (uint16_t)(cpu->reg[rn] & 0xFFFF));
}

static void print_state(const risco_t *cpu) {
	for (int i = 0; i < 16; i++) {
		printf("R%d = 0x%04X\n", i, cpu->reg[i]);
	}
	printf("Z = %d\n", cpu->flagZ);
	printf("C = %d\n", cpu->flagC);

	if (get_sp(cpu) != 0x2000) {
		int sp = (int)get_sp(cpu);
		for (int addr = 0x1FFF; addr >= sp; addr--) {
			printf("[0x%04X] = 0x%04X\n", addr, cpu->memory[addr]);
		}
	}

	bool has_access = false;
	for (int addr = 0; addr < RISCO_MEM_SIZE; addr++) {
		if (cpu->accessed[addr]) {
			has_access = true;
			break;
		}
	}
	if (has_access) {
		for (int addr = 0; addr < RISCO_MEM_SIZE; addr++) {
			if (cpu->accessed[addr]) {
				printf("[0x%04X] = 0x%04X\n", addr, cpu->memory[addr]);
			}
		}
	}
}

int main(void) {
	int num_breakpoints = 0;
	if (scanf("%d", &num_breakpoints) != 1) {
		return 0;
	}

	bool breakpoints[RISCO_MEM_SIZE];
	memset(breakpoints, 0, sizeof(breakpoints));

	for (int i = 0; i < num_breakpoints; i++) {
		unsigned int addr = 0;
		if (scanf("%x", &addr) != 1) {
			addr = 0;
		}
		if (addr < RISCO_MEM_SIZE) {
			breakpoints[addr] = true;
		}
	}

	risco_t cpu;
	memset(&cpu, 0, sizeof(cpu));
	cpu.reg[14] = 0x2000;
	cpu.next_pc = 0;

	unsigned int address = 0;
	unsigned int data = 0;
	while (scanf("%x %x", &address, &data) == 2) {
		if (address == 0 && data == 0) {
			break;
		}
		if (address < RISCO_MEM_SIZE) {
			cpu.memory[address] = (uint16_t)(data & 0xFFFF);
		}
	}

	bool halted = false;
	while (!halted) {
		uint16_t pc = get_pc(&cpu);

		if (pc >= RISCO_MEM_SIZE) {
			printf("Program counter out of bounds: 0x%04X!\n", pc);
			break;
		}

		uint16_t ir = cpu.memory[pc];

		/* Busca: PC é incrementado antes da execução. */
		cpu.reg[15] = (uint16_t)(pc + 1);
		set_next_pc(&cpu, cpu.reg[15]);

		if (ir == INSTR_HALT) {
			halted = true;
		} else {

			uint16_t op = (uint16_t)(ir & 0x000F);
			uint16_t ibr = (uint16_t)(ir & 0xFFF0);

			switch (op) {
			case OP_JMP:
				instr_jmp(&cpu, ibr);
				break;
			case OP_JEQ_JNE_JLT_JGT:
				instr_jmp_special(&cpu, ibr);
				break;
			case OP_LDR:
				instr_ldr(&cpu, ibr);
				break;
			case OP_STR:
				instr_str(&cpu, ibr);
				break;
			case OP_MOV:
				instr_mov(&cpu, ibr);
				break;
			case OP_ADD:
				instr_add(&cpu, ibr);
				break;
			case OP_ADDI:
				instr_addi(&cpu, ibr);
				break;
			case OP_SUB:
				instr_sub(&cpu, ibr);
				break;
			case OP_SUBI:
				instr_subi(&cpu, ibr);
				break;
			case OP_AND:
				instr_and(&cpu, ibr);
				break;
			case OP_OR:
				instr_or(&cpu, ibr);
				break;
			case OP_SHR:
				instr_shr(&cpu, ibr);
				break;
			case OP_SHL:
				instr_shl(&cpu, ibr);
				break;
			case OP_CMP:
				instr_cmp(&cpu, ibr);
				break;
			case OP_PUSH:
				instr_push(&cpu, ibr);
				break;
			case OP_POP:
				instr_pop(&cpu, ibr);
				break;
			default:
				printf("Invalid instruction %04X!\n", ibr);
				return 0;
			}
		}

		confirm_next_pc(&cpu);

		if ((pc < RISCO_MEM_SIZE && breakpoints[pc]) || halted) {
			print_state(&cpu);
		}
	}

	return 0;
}
