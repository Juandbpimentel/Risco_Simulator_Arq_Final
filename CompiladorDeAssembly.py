import sys
import re


class RiscOAssembler:
    def __init__(self):
        self.opcodes = {
            "JMP": 0x0,
            "JEQ": 0x1,
            "JNE": 0x1,
            "JLT": 0x1,
            "JGT": 0x1,
            "JGE": 0x1,
            "LDR": 0x2,
            "STR": 0x3,
            "MOV": 0x4,
            "ADD": 0x5,
            "ADDI": 0x6,
            "SUB": 0x7,
            "SUBI": 0x8,
            "AND": 0x9,
            "OR": 0xA,
            "SHR": 0xB,
            "SHL": 0xC,
            "CMP": 0xD,
            "PUSH": 0xE,
            "POP": 0xF,
            "HALT": 0xFFFF,
        }
        self.symbol_table = {}

    def assemble(self, asm_text):
        raw_lines = asm_text.split("\n")
        processed_lines = []
        address = 0

        label_re = re.compile(r"^\s*([A-Za-z_]\w*)\s*:\s*(.*)$")

        # PASSAGEM 1: Mapear Símbolos
        for line in raw_lines:
            line = re.sub(r";.*|//.*", "", line).rstrip()
            if not line.strip():
                continue

            # Permite múltiplas labels na mesma linha:
            # ex: "foo: bar: ADD R0, R0, R0".
            while True:
                match = label_re.match(line)
                if not match:
                    break
                label, rest = match.group(1), match.group(2)
                self.symbol_table[label] = address
                line = rest

            line = line.strip()
            if line:
                processed_lines.append((address, line))
                address += 1

        # PASSAGEM 2: Codificação
        hex_output = []
        for addr, line in processed_lines:
            hex_val = self.encode(line, addr)
            hex_output.append(f"{addr:04X} {hex_val:04X}")
        return hex_output

    def parse_reg(self, r):
        return int(r.upper().replace("R", "").replace(",", ""))

    def get_val(self, s):
        s = s.replace("#", "").replace(",", "").strip()
        if s in self.symbol_table:
            return self.symbol_table[s]
        try:
            return int(s, 0)
        except:
            return 0

    def encode(self, line, pc):
        if line.startswith("'"):
            c = line.strip("'")
            return 0 if c == "\\0" else ord(c)

        parts = re.findall(r"[\[\]\w']+|[#\-?\w]+", line.replace(",", " "))
        mnemonic = parts[0].upper()
        if mnemonic == "HALT":
            return 0xFFFF

        op = self.opcodes.get(mnemonic, 0)
        try:
            if mnemonic in ["JEQ", "JNE", "JLT", "JGT", "JGE"]:
                conds = {"JEQ": 0, "JNE": 1, "JLT": 2, "JGT": 3, "JGE": 3}
                target = self.get_val(parts[1])
                # Offset de 10 bits para saltos especiais
                offset = (target - pc - 1) & 0x3FF
                return (conds[mnemonic] << 14) | (offset << 4) | op

            elif mnemonic == "JMP":
                target = self.get_val(parts[1])
                offset = (target - pc - 1) & 0xFFF
                return (offset << 4) | op

            elif mnemonic == "LDR":
                # LDR: Rd (15-12), Rm (11-8), Imm (7-4)
                rd = self.parse_reg(parts[1])
                rm = self.parse_reg(parts[2].replace("[", ""))
                imm = self.get_val(parts[3].replace("]", ""))
                return (rd << 12) | (rm << 8) | ((imm & 0xF) << 4) | op

            elif mnemonic == "STR":
                # STR: Imm (15-12), Rm (11-8), Rn (7-4) -- CONFORME A TABELA
                rn = self.parse_reg(parts[1])  # O valor a ser salvo
                rm = self.parse_reg(parts[2].replace("[", ""))  # O endereço base
                imm = self.get_val(parts[3].replace("]", ""))  # O offset
                # Note que aqui o imm vai para o shift 12 e o rn para o shift 4
                return ((imm & 0xF) << 12) | (rm << 8) | (rn << 4) | op

            elif mnemonic == "MOV":
                rd = self.parse_reg(parts[1])
                imm = self.get_val(parts[2])
                return (rd << 12) | ((imm & 0xFF) << 4) | op

            elif mnemonic in ["ADD", "OR", "SUB", "AND", "CMP"]:
                rd, rm, rn = (
                    self.parse_reg(parts[1]),
                    self.parse_reg(parts[2]),
                    self.parse_reg(parts[3]),
                )
                return (rd << 12) | (rm << 8) | (rn << 4) | op

            elif mnemonic in ["ADDI", "SUBI", "SHL", "SHR"]:
                rd, rm, imm = (
                    self.parse_reg(parts[1]),
                    self.parse_reg(parts[2]),
                    self.get_val(parts[3]),
                )
                return (rd << 12) | (rm << 8) | ((imm & 0xF) << 4) | op

            elif mnemonic == "PUSH":
                return (self.parse_reg(parts[1]) << 4) | op
            elif mnemonic == "POP":
                return (self.parse_reg(parts[1]) << 12) | op
        except:
            return 0
        return 0


def main():
    if len(sys.argv) < 2:
        return
    with open(sys.argv[1], "r") as f:
        out = RiscOAssembler().assemble(f.read())
        print("\n".join(out))
        print("0000 0000")


if __name__ == "__main__":
    main()
