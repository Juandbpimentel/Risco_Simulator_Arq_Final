from ctypes import c_int16

# Tamanho da memória, operacoes
RISC_O_MEM_SIZE = 0x2000
JEQ_JNE_JLT_JGT = 0x0001  # 0b0000_0000_0000_0001

# --- JUMPS ---
OP_JMP = 0x0
OP_JEQ_JNE_JLT_JGT = 0x1

# --- DADOS E MEMÓRIA ---
OP_LDR = 0x2
OP_STR = 0x3
OP_MOV = 0x4

# --- ULA ---
OP_ADD = 0x5
OP_ADDI = 0x6
OP_SUB = 0x7
OP_SUBI = 0x8
OP_AND = 0x9
OP_OR = 0xA
OP_SHR = 0xB
OP_SHL = 0xC
OP_CMP = 0xD

# --- PILHA E CONTROLE ---
OP_PUSH = 0xE
OP_POP = 0xF


class Risc_O:
    def __init__(self):
        self.reg: list[int] = [0] * 16
        self.reg[14] = 0x2000
        self.memory = [0] * RISC_O_MEM_SIZE
        # Quando qualquer operação é feita na ula, as flags são resetadas para 0, a não ser que a operação resulte em zero ou em carry

        # Quando uma operação da ula resulta em zero, o flag0 é setado para 1

        # Esse aqui é o comportamento das flags tem o valor de acordo com a tabela abaixo:
        # comparação entre Rn e Rm
        #   Resultados | flag0 flagCarry
        #      Rn == Rm   |   1      0
        #      Rn > Rm    |   0      1
        #      Rn < Rm    |   0      0
        # Loading de valor Rn + imediato para Rd
        #      Rd < 0     |   0      1
        #      Rd == 0    |   1      0
        self.flag0 = 0
        self.flagCarry = 0
        self.nextPc = 1
        self.acessados = set()

    # Ver se vai ser necessário dps
    def get_pc(self):
        return self.reg[15]

    def set_nextPc(self, valor):
        self.nextPc = valor & 0xFFFF

    def confirm_NextPc(self):
        self.reg[15] = self.nextPc

    def get_sp(self):
        return self.reg[14]

    def set_sp(self, sp):
        self.reg[14] = sp


processador = Risc_O()


def JMP_expecial_handler(
    processador: Risc_O,
    ibr: int,
):
    cond = (ibr >> 14) & 0x03

    if cond == 0x0:
        JEQ_instruction(processador, ibr)
    elif cond == 0x1:
        JNE_instruction(processador, ibr)
    elif cond == 0x2:
        JLT_instruction(processador, ibr)
    elif cond == 0x3:
        JGE_instruction(processador, ibr)


def JMP_instruction(processador: Risc_O, ibr: int):
    offset = c_int16(ibr).value >> 4
    novo_destino = processador.get_pc() + 1 + offset
    processador.set_nextPc(novo_destino)


def JEQ_instruction(processador: Risc_O, ibr: int):
    offset = c_int16(ibr << 2).value >> 6
    if processador.flag0 == 1:
        novo_destino = processador.get_pc() + 1 + offset
        processador.set_nextPc(novo_destino)


def JNE_instruction(processador: Risc_O, ibr: int):
    offset = c_int16(ibr << 2).value >> 6
    if processador.flag0 == 0:
        novo_destino = processador.get_pc() + 1 + offset
        processador.set_nextPc(novo_destino)


def JLT_instruction(processador: Risc_O, ibr: int):
    offset = c_int16(ibr << 2).value >> 6
    if processador.flag0 == 0 and processador.flagCarry == 1:
        novo_destino = processador.get_pc() + 1 + offset
        processador.set_nextPc(novo_destino)


def JGE_instruction(processador: Risc_O, ibr: int):
    offset = c_int16(ibr << 2).value >> 6
    if processador.flag0 == 1 or processador.flagCarry == 0:
        novo_destino = processador.get_pc() + 1 + offset
        processador.set_nextPc(novo_destino)


def PUSH_instruction(processador: Risc_O, ibr: int):
    processador.set_sp(processador.get_sp() - 1)
    rn = (ibr >> 4) & 0x000F
    processador.memory[processador.get_sp()] = processador.reg[rn]


def POP_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    valor = processador.memory[processador.get_sp()]
    processador.set_sp(processador.get_sp() + 1)

    if rd == 15:
        processador.set_nextPc(valor)
    else:
        processador.reg[rd] = valor


def ADD_instruction(processador: Risc_O, ibr: int):  # Juan Pimentel
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    rn = (ibr >> 4) & 0x000F

    resultado = processador.reg[rm] + processador.reg[rn]

    processador.flagCarry = 1 if resultado > 0xFFFF else 0
    processador.reg[rd] = resultado & 0xFFFF
    processador.flag0 = 1 if processador.reg[rd] == 0 else 0


def CMP_instruction(processador: Risc_O, ibr: int):
    rm = (ibr >> 8) & 0x000F
    rn = (ibr >> 4) & 0x000F
    if processador.reg[rm] == processador.reg[rn]:
        processador.flag0 = 1
        processador.flagCarry = 0
    elif processador.reg[rm] < processador.reg[rn]:
        processador.flag0 = 0
        processador.flagCarry = 1
    else:
        processador.flag0 = 0
        processador.flagCarry = 0
    pass


def MOV_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    im = (ibr >> 4) & 0x00FF
    processador.reg[rd] = im & 0xFFFF


def ADDI_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    im = (ibr >> 4) & 0x000F

    resultado = processador.reg[rm] + im
    processador.flagCarry = 1 if resultado > 0xFFFF else 0
    processador.reg[rd] = resultado & 0xFFFF
    processador.flag0 = 1 if processador.reg[rd] == 0 else 0
    pass


def OR_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    rn = (ibr >> 4) & 0x000F

    processador.reg[rd] = processador.reg[rm] | processador.reg[rn]
    processador.flag0 = 1 if processador.reg[rd] == 0 else 0
    processador.flagCarry = 0
    pass


def SHR_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    im = (ibr >> 4) & 0x000F

    resultado = processador.reg[rm] >> im
    processador.flagCarry = (processador.reg[rm] >> (im - 1)) & 1 if im > 0 else 0
    processador.flag0 = 1 if resultado == 0 else 0

    processador.reg[rd] = resultado & 0xFFFF
    pass


def SHL_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    im = (ibr >> 4) & 0x000F

    resultado = processador.reg[rm] << im
    processador.flagCarry = 1 if resultado > 0xFFFF else 0
    processador.flag0 = 1 if (resultado & 0xFFFF) == 0 else 0

    processador.reg[rd] = resultado & 0xFFFF
    pass


def io_char_write(char: int):
    print(f"OUT <= {chr(char)}")


def io_char_read() -> int:
    value = ord(input())
    print(f"IN => {value}")
    return value


def io_int_write(num: int):
    print(f"OUT <= {num}")


def io_int_read() -> int:
    value = int(input())
    print(f"IN => {value}")
    return value


io_readers = {
    0xF000: io_char_read,
    0xF002: io_int_read,
}

io_writers = {
    0xF001: io_char_write,
    0xF003: io_int_write,
}


def _mem_read(processador: Risc_O, addr: int) -> int:
    addr &= 0xFFFF
    if addr in io_readers:
        return io_readers[addr]()

    processador.acessados.add(addr)
    return processador.memory[addr]


def _mem_write(processador: Risc_O, addr: int, value: int) -> None:
    addr &= 0xFFFF
    value &= 0xFFFF
    if addr in io_writers:
        io_writers[addr](value)
        return

    processador.acessados.add(addr)
    processador.memory[addr] = value
    return


def LDR_instruction(processador: Risc_O, ibr: int):
    rd = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    im = (ibr >> 4) & 0x000F

    addr = (processador.reg[rm] + im) & 0xFFFF
    processador.reg[rd] = _mem_read(processador, addr) & 0xFFFF


def STR_instruction(processador: Risc_O, ibr: int):
    im = (ibr >> 12) & 0x000F
    rm = (ibr >> 8) & 0x000F
    rn = (ibr >> 4) & 0x000F

    addr = (processador.reg[rm] + im) & 0xFFFF
    _mem_write(processador, addr, processador.reg[rn] & 0xFFFF)
    pass


def SUB_instruction(processador: Risc_O, ibr: int):
    rd, rm, rn = (ibr >> 12) & 0xF, (ibr >> 8) & 0xF, (ibr >> 4) & 0xF
    # C = 1 se houver borrow (rm < rn)
    processador.flagCarry = 1 if processador.reg[rm] < processador.reg[rn] else 0
    res = (processador.reg[rm] - processador.reg[rn]) & 0xFFFF
    processador.reg[rd] = res
    processador.flag0 = 1 if res == 0 else 0


def SUBI_instruction(processador: Risc_O, ibr: int):
    rd, rm = (ibr >> 12) & 0xF, (ibr >> 8) & 0xF
    im = (ibr >> 4) & 0xF  # Imediato de 4 bits
    processador.flagCarry = 1 if processador.reg[rm] < im else 0
    res = (processador.reg[rm] - im) & 0xFFFF
    processador.reg[rd] = res
    processador.flag0 = 1 if res == 0 else 0


def AND_instruction(processador: Risc_O, ibr: int):
    rd, rm, rn = (ibr >> 12) & 0xF, (ibr >> 8) & 0xF, (ibr >> 4) & 0xF
    res = (processador.reg[rm] & processador.reg[rn]) & 0xFFFF
    processador.reg[rd] = res
    processador.flag0 = 1 if res == 0 else 0
    processador.flagCarry = 0  # Lógicas resetam carry


operations_map = {
    OP_JMP: JMP_instruction,
    OP_JEQ_JNE_JLT_JGT: JMP_expecial_handler,
    OP_LDR: LDR_instruction,
    OP_STR: STR_instruction,
    OP_MOV: MOV_instruction,
    OP_ADD: ADD_instruction,
    OP_ADDI: ADDI_instruction,
    OP_SUB: SUB_instruction,
    OP_SUBI: SUBI_instruction,
    OP_AND: AND_instruction,
    OP_OR: OR_instruction,
    OP_SHR: SHR_instruction,
    OP_SHL: SHL_instruction,
    OP_CMP: CMP_instruction,
    OP_PUSH: PUSH_instruction,
    OP_POP: POP_instruction,
}


def main():
    num_of_breakpoints = int(input())
    bp = []
    for _ in range(num_of_breakpoints):
        bp.append(int(input(), 16))
    memoryBreakpoints = [0] * RISC_O_MEM_SIZE

    if len(bp) > 0:
        for breakpoint in bp:
            memoryBreakpoints[breakpoint] = 1

    while True:
        line = input()
        splited_line = line.split()
        if len(splited_line) < 2:
            break
        address, data = splited_line
        address = int(address, 16)
        data = int(data, 16)
        if address == 0 and data == 0:
            break
        processador.memory[address] = data

    processador_halted = False
    while not processador_halted:
        pc_atual = processador.get_pc()

        if pc_atual >= RISC_O_MEM_SIZE:
            print(f"Program counter out of bounds: 0x{pc_atual:04X}!")
            break

        ir = processador.memory[pc_atual]

        if ir == 0xFFFF:  # HALT
            processador_halted = True
        else:
            processador.set_nextPc(pc_atual + 1)

            op = ir & 0x000F
            ibr = ir & 0xFFF0

            ##INSTRUÇÕES##

            if op in operations_map:
                operations_map[op](processador, ibr)

            else:
                print(f"Invalid instruction {ibr:04X}!")
                break

        if memoryBreakpoints[pc_atual] or processador_halted:
            for i in range(16):
                if i == 14:
                    print(f"R{i} = 0x{processador.reg[i]:04X}")
                elif i == 15:
                    print(f"R{i} = 0x{processador.reg[i]:04X}")
                else:
                    print(f"R{i} = 0x{processador.reg[i]:04X}")
            print(f"Z = {processador.flag0}")
            print(f"C = {processador.flagCarry}")
            if processador.get_sp() != 0x2000:
                for addr in reversed(range(processador.get_sp(), 0x2000)):
                    print(f"[0x{addr:04X}] = 0x{processador.memory[addr]:04X}")

            if len(processador.acessados) > 0:
                for addr in sorted(processador.acessados):
                    if addr < 0xF000:
                        print(f"[0x{addr:04X}] = 0x{processador.memory[addr]:04X}")

        processador.confirm_NextPc()
        # Fim do ciclo de instrução


if __name__ == "__main__":
    main()
