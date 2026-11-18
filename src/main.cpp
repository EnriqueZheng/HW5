// Simulador del ISA base RV32I de RISC-V
// CS3051 Arquitectura de Computadores - HW#5

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Memoria: plana, byte-addressable, espacio de direcciones de 32 bits.
// Se guarda por páginas de 4 KiB que se crean solo cuando se escriben,
// así no hace falta reservar 4 GiB. Leer una dirección nunca escrita da 0.
// ---------------------------------------------------------------------------
class Memory {
private:
    static constexpr uint32_t PAGE_SIZE = 4096;
    std::unordered_map<uint32_t, std::array<uint8_t, PAGE_SIZE>> pages;

public:
    uint8_t read8(uint32_t addr) const {
        auto it = pages.find(addr / PAGE_SIZE);
        if (it == pages.end())
            return 0;
        return it->second[addr % PAGE_SIZE];
    }

    void write8(uint32_t addr, uint8_t value) {
        auto it = pages.find(addr / PAGE_SIZE);
        if (it == pages.end())
            it = pages.emplace(addr / PAGE_SIZE, std::array<uint8_t, PAGE_SIZE>{}).first;
        it->second[addr % PAGE_SIZE] = value;
    }

    // Little-endian: el byte menos significativo va en la dirección más baja.
    uint16_t read16(uint32_t addr) const {
        return read8(addr) | (read8(addr + 1) << 8);
    }

    uint32_t read32(uint32_t addr) const {
        return read8(addr) | (read8(addr + 1) << 8) | (read8(addr + 2) << 16) |
               ((uint32_t)read8(addr + 3) << 24);
    }

    void write16(uint32_t addr, uint16_t value) {
        write8(addr, value & 0xFF);
        write8(addr + 1, (value >> 8) & 0xFF);
    }

    void write32(uint32_t addr, uint32_t value) {
        write8(addr, value & 0xFF);
        write8(addr + 1, (value >> 8) & 0xFF);
        write8(addr + 2, (value >> 16) & 0xFF);
        write8(addr + 3, (value >> 24) & 0xFF);
    }
};

// ---------------------------------------------------------------------------
// Estado arquitectural de la CPU: PC, 32 registros y memoria.
// ---------------------------------------------------------------------------
class Simulator {
private:
    uint32_t pc = 0;
    uint32_t regs[32] = {};
    Memory memory;
    bool halted = false;
    std::string error;

    void fail(const std::string& msg) {
        halted = true;
        error = msg;
    }

public:
    // Carga un binario crudo a partir de la dirección 0x00000000.
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
            return false;

        std::vector<char> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        for (uint32_t i = 0; i < data.size(); i++)
            memory.write8(i, (uint8_t)data[i]);
        return true;
    }

    uint32_t get_pc() const { return pc; }
    uint32_t get_reg(int i) const { return regs[i]; }
    uint8_t get_byte(uint32_t addr) const { return memory.read8(addr); }
    bool is_halted() const { return halted; }
    const std::string& get_error() const { return error; }

    // Ejecuta una instrucción: fetch, decode, execute y actualización del estado.
    void step() {
        if (halted)
            return;

        // ---- Fetch ----
        if (pc % 4 != 0) {
            fail("PC desalineado");
            return;
        }
        uint32_t instr = memory.read32(pc);

        // ---- Decode ----
        uint32_t opcode = instr & 0x7F;
        uint32_t rd = (instr >> 7) & 0x1F;
        uint32_t funct3 = (instr >> 12) & 0x7;
        uint32_t rs1 = (instr >> 15) & 0x1F;
        uint32_t rs2 = (instr >> 20) & 0x1F;
        uint32_t funct7 = instr >> 25;

        // Inmediatos con extensión de signo (el corrimiento aritmético de
        // int32_t replica el bit 31 de la instrucción).
        int32_t imm_i = (int32_t)instr >> 20;
        int32_t imm_s = ((int32_t)(instr & 0xFE000000) >> 20) | ((instr >> 7) & 0x1F);
        int32_t imm_b = ((int32_t)(instr & 0x80000000) >> 19) | ((instr & 0x80) << 4) |
                        ((instr >> 20) & 0x7E0) | ((instr >> 7) & 0x1E);
        uint32_t imm_u = instr & 0xFFFFF000;
        int32_t imm_j = ((int32_t)(instr & 0x80000000) >> 11) | (instr & 0xFF000) |
                        ((instr >> 9) & 0x800) | ((instr >> 20) & 0x7FE);

        uint32_t a = regs[rs1];
        uint32_t b = regs[rs2];
        uint32_t next_pc = pc + 4;
        bool valid = true;

        // ---- Execute ----
        switch (opcode) {
            case 0x03: {  // Loads
                uint32_t addr = a + imm_i;
                switch (funct3) {
                    case 0x0: regs[rd] = (int32_t)(int8_t)memory.read8(addr); break;    // lb
                    case 0x1: regs[rd] = (int32_t)(int16_t)memory.read16(addr); break;  // lh
                    case 0x2: regs[rd] = memory.read32(addr); break;                    // lw
                    case 0x4: regs[rd] = memory.read8(addr); break;                     // lbu
                    case 0x5: regs[rd] = memory.read16(addr); break;                    // lhu
                    default: valid = false;
                }
                break;
            }
            case 0x13: {  // Operaciones con inmediato
                uint32_t shamt = rs2;
                switch (funct3) {
                    case 0x0: regs[rd] = a + imm_i; break;                            // addi
                    case 0x2: regs[rd] = (int32_t)a < imm_i; break;                   // slti
                    case 0x3: regs[rd] = a < (uint32_t)imm_i; break;                  // sltiu
                    case 0x4: regs[rd] = a ^ imm_i; break;                            // xori
                    case 0x6: regs[rd] = a | imm_i; break;                            // ori
                    case 0x7: regs[rd] = a & imm_i; break;                            // andi
                    case 0x1:                                                         // slli
                        if (funct7 == 0x00) regs[rd] = a << shamt;
                        else valid = false;
                        break;
                    case 0x5:
                        if (funct7 == 0x00) regs[rd] = a >> shamt;                    // srli
                        else if (funct7 == 0x20) regs[rd] = (int32_t)a >> shamt;      // srai
                        else valid = false;
                        break;
                }
                break;
            }
            case 0x17:  // auipc
                regs[rd] = pc + imm_u;
                break;
            case 0x23: {  // Stores
                uint32_t addr = a + imm_s;
                switch (funct3) {
                    case 0x0: memory.write8(addr, b & 0xFF); break;      // sb
                    case 0x1: memory.write16(addr, b & 0xFFFF); break;   // sh
                    case 0x2: memory.write32(addr, b); break;            // sw
                    default: valid = false;
                }
                break;
            }
            case 0x33: {  // Operaciones registro-registro
                uint32_t shamt = b & 0x1F;
                if (funct7 == 0x00) {
                    switch (funct3) {
                        case 0x0: regs[rd] = a + b; break;                    // add
                        case 0x1: regs[rd] = a << shamt; break;               // sll
                        case 0x2: regs[rd] = (int32_t)a < (int32_t)b; break;  // slt
                        case 0x3: regs[rd] = a < b; break;                    // sltu
                        case 0x4: regs[rd] = a ^ b; break;                    // xor
                        case 0x5: regs[rd] = a >> shamt; break;               // srl
                        case 0x6: regs[rd] = a | b; break;                    // or
                        case 0x7: regs[rd] = a & b; break;                    // and
                    }
                } else if (funct7 == 0x20 && funct3 == 0x0) {
                    regs[rd] = a - b;                                         // sub
                } else if (funct7 == 0x20 && funct3 == 0x5) {
                    regs[rd] = (int32_t)a >> shamt;                           // sra
                } else {
                    valid = false;
                }
                break;
            }
            case 0x37:  // lui
                regs[rd] = imm_u;
                break;
            case 0x63: {  // Branches
                bool taken;
                switch (funct3) {
                    case 0x0: taken = a == b; break;                    // beq
                    case 0x1: taken = a != b; break;                    // bne
                    case 0x4: taken = (int32_t)a < (int32_t)b; break;   // blt
                    case 0x5: taken = (int32_t)a >= (int32_t)b; break;  // bge
                    case 0x6: taken = a < b; break;                     // bltu
                    case 0x7: taken = a >= b; break;                    // bgeu
                    default: taken = false; valid = false;
                }
                if (taken)
                    next_pc = pc + imm_b;
                break;
            }
            case 0x67:  // jalr
                if (funct3 == 0x0) {
                    next_pc = (a + imm_i) & ~1u;  // se calcula antes de escribir rd
                    regs[rd] = pc + 4;
                } else {
                    valid = false;
                }
                break;
            case 0x6F:  // jal
                regs[rd] = pc + 4;
                next_pc = pc + imm_j;
                break;
            default:
                valid = false;
        }

        if (!valid) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Instruccion invalida 0x%08X", instr);
            fail(buf);
            return;
        }

        regs[0] = 0;  // x0 siempre vale 0
        pc = next_pc;
    }
};

// ---------------------------------------------------------------------------
// Interfaz de línea de comandos
// ---------------------------------------------------------------------------
static const char* ABI_NAMES[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

// Acepta "x0".."x31" o el nombre ABI ("a0", "sp", "fp", ...). Devuelve -1 si no es válido.
static int parse_reg(const std::string& name) {
    if (name.size() >= 2 && name[0] == 'x') {
        try {
            size_t pos;
            int n = std::stoi(name.substr(1), &pos);
            if (pos == name.size() - 1 && n >= 0 && n < 32)
                return n;
        } catch (...) {
        }
        return -1;
    }
    if (name == "fp")
        return 8;
    for (int i = 0; i < 32; i++)
        if (name == ABI_NAMES[i])
            return i;
    return -1;
}

static bool parse_num(const std::string& s, uint32_t& out) {
    try {
        size_t pos;
        unsigned long v = std::stoul(s, &pos, 0);  // base 0: acepta 0x.. y decimal
        if (pos != s.size() || v > 0xFFFFFFFFul)
            return false;
        out = (uint32_t)v;
        return true;
    } catch (...) {
        return false;
    }
}

static void print_reg(const Simulator& sim, int i) {
    std::printf("x%-2d (%-4s) = 0x%08X\n", i, ABI_NAMES[i], sim.get_reg(i));
}

static void print_help() {
    std::cout << "Comandos:\n"
                 "  step [n]           Ejecuta n instrucciones (por defecto 1)\n"
                 "  pc                 Muestra el PC\n"
                 "  regs [r1 r2 ...]   Muestra registros (todos si no se indica ninguno)\n"
                 "  mem <inicio> [fin] Muestra la memoria de inicio a fin (inclusive)\n"
                 "  help               Muestra esta ayuda\n"
                 "  exit               Sale del simulador\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <programa.bin>\n";
        return 1;
    }

    Simulator sim{};
    if (!sim.load_from_file(argv[1])) {
        std::cerr << "No se pudo cargar el archivo \"" << argv[1] << "\".\n";
        return 1;
    }
    std::cout << "\"" << argv[1] << "\" cargado a memoria. Escribe 'help' para ver los comandos.\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line))
            break;

        std::istringstream iss(line);
        std::string cmd;
        std::vector<std::string> args;
        iss >> cmd;
        for (std::string arg; iss >> arg;)
            args.push_back(arg);

        if (cmd.empty()) {
            continue;
        } else if (cmd == "step") {
            uint32_t n = 1;
            if (!args.empty() && (!parse_num(args[0], n) || n == 0)) {
                std::cout << "Cantidad invalida.\n";
                continue;
            }
            if (sim.is_halted()) {
                std::cout << "Ejecucion detenida: " << sim.get_error() << "\n";
                continue;
            }
            uint32_t done = 0;
            while (done < n && !sim.is_halted()) {
                sim.step();
                if (!sim.is_halted())
                    done++;
            }
            std::printf("Ejecutadas %u instruccion(es). pc = 0x%08X\n", done, sim.get_pc());
            if (sim.is_halted())
                std::printf("Error en pc = 0x%08X: %s\n", sim.get_pc(), sim.get_error().c_str());
        } else if (cmd == "pc") {
            std::printf("pc = 0x%08X\n", sim.get_pc());
        } else if (cmd == "regs") {
            if (args.empty()) {
                for (int i = 0; i < 32; i++)
                    print_reg(sim, i);
            } else {
                for (const auto& a : args) {
                    int r = parse_reg(a);
                    if (r < 0)
                        std::cout << "Registro invalido: " << a << "\n";
                    else
                        print_reg(sim, r);
                }
            }
        } else if (cmd == "mem") {
            uint32_t start, end;
            if (args.empty() || !parse_num(args[0], start)) {
                std::cout << "Uso: mem <inicio> [fin]\n";
                continue;
            }
            end = start;
            if (args.size() > 1 && (!parse_num(args[1], end) || end < start)) {
                std::cout << "Rango invalido.\n";
                continue;
            }
            // 16 bytes por fila, en el orden en que están en memoria
            for (uint64_t addr = start; addr <= end; addr += 16) {
                std::printf("0x%08X:", (uint32_t)addr);
                for (uint64_t i = addr; i < addr + 16 && i <= end; i++)
                    std::printf(" %02X", sim.get_byte((uint32_t)i));
                std::printf("\n");
            }
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "exit") {
            break;
        } else {
            std::cout << "Comando desconocido: " << cmd << ". Escribe 'help'.\n";
        }
    }

    return 0;
}
