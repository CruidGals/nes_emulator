//
//  6502emu.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

//TODO: Implement All Instructions, Then Focus on Bit

#include "6502emu.hpp"
#include "instructions.hpp"

#include <iostream>
#include <stdint.h>

/* ---------- CPU IMPLEMENTATION ---------- */

cpu6502::cpu6502(Memory& mem) : memory(mem)
{
    ps.i = 1;
    ps._ = 1;
    s = 0xff;
}

/* ---------- HELPER FUNCTIONS ---------- */
void cpu6502::incStack() { if (s < 255) s++; }
void cpu6502::decStack() { if (s > 0) s--; }

uint8_t cpu6502::parseProcessorStatus() { return ps.val; }
void cpu6502::unparseProcessorStatus(const uint8_t status) { ps.val = status; }

/* ---------- FUNCTIONS ---------- */

int cpu6502::interrupt_handler(InterruptType type)
{
    // If interrupt disable is on, abort immediately (except when it is NMI)
    if (ps.i &&
        (type == InterruptType::BRK ||
         type == InterruptType::IRQ ||
         type == InterruptType::RESET)) 
    {
        return 0;
    }

    
    // PC incremented to PC + 2 (pc is already incremented once duriing emulate() )
    pc.val++;
    
    //Push PC onto stack
    memory[0x100 | s] = pc.hi;
    decStack();
    memory[0x100 | s] = pc.lo;
    decStack();
    
    // If BRK is called, then set break flag
    ps.b = type == InterruptType::BRK;
    memory[0x100 | s] = parseProcessorStatus();
    decStack();
    
    //Set Interrupt Flag
    ps.i = 1;
    
    //Load Interrupt vector into memory
    switch (type) {
        
        // BRK and IRQ have the same interrupt vector
        case InterruptType::BRK:
            [[fallthrough]];    // Intentional fallthrough attribute to indicate that they have the same interrupt vector
        case InterruptType::IRQ:
            pc.lo = memory[0xFFFE];
            pc.hi = memory[0xFFFF];
            break;
            
        case InterruptType::NMI:
            pc.lo = memory[0xFFFA];
            pc.hi = memory[0xFFFB];
            break;
            
        case InterruptType::RESET:
            pc.lo = memory[0xFFFC];
            pc.hi = memory[0xFFFD];
            break;
            
        default:
            break;
    }
    
    return 7; // Number of cycles
}

int cpu6502::emulate()
{
    using namespace AddressingModeFuncs;
    
    uint8_t *opcode = this->memory.getBaseAddress() + this->pc.val;
    //std::cout << "Opcode: " << std::hex << static_cast<int>(*opcode) << std::dec << " PC: " << static_cast<int>(this->pc.val) << std::endl;
    this->pc.val += 1;
    
    //Instruction cycles given no page is crossed or branch is taken, as those events increase cycles by one
    constexpr int base_cycles[] = { 
        7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, //0x00 - 0x0f
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0x10 - 0x1f
        6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, //0x20 - 0x2f
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0x30 - 0x3f
        6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0, //0x40 - 0x4f
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0x50 - 0x5f
        6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, //0x60 - 0x6f
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0x70 - 0x7f
        0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, //0x80 - 0x8f
        2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, //0x90 - 0x9f
        2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, //0xa0 - 0xaf
        2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0, //0xb0 - 0xbf
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, //0xc0 - 0xcf
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0xd0 - 0xdf
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, //0xe0 - 0xef
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, //0xf0 - 0xff
    };
    
    int cycles = base_cycles[*opcode];
    
    switch (*opcode)
    {
        case 0x00: //BRK
            interrupt_handler(InterruptType::BRK);
            break;
        case 0x01: //ORA - X-Indexed Zero Page Indirect
            Instructions::ORA(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x05: //ORA - Zero Page
            Instructions::ORA(this, opcode, ZERO_PAGE);
            break;
        case 0x06: //ASL - Zero Page
            Instructions::ASL(this, opcode, ZERO_PAGE);
            break;
        case 0x08: //PHP
            Instructions::PHP(this);
            break;
        case 0x09: //ORA - Immediate
            Instructions::ORA(this, opcode, IMMEDIATE);
            break;
        case 0x0a: //ASL - Accumulator
            Instructions::ASL(this, opcode, ACCUMULATOR);
            break;
        case 0x0d: //ORA - Absolute
            Instructions::ORA(this, opcode, ABSOLUTE);
            break;
        case 0x0e: //ASL - Absolute
            Instructions::ASL(this, opcode, ABSOLUTE);
            break;
        case 0x10: //BPL
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.n == 0);
            break;
        case 0x11: //ORA - Zero Page Indirect Y-Indexed
            cycles += Instructions::ORA(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x15: //ORA - X-Indexed Zero Page
            Instructions::ORA(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x16: //ASL - X-Indexed Zero Page
            Instructions::ASL(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x18: //CLC
            this->ps.c = 0;
            break;
        case 0x19: //ORA - Y-Indexed Absolute
            cycles += Instructions::ORA(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x1d: //ORA - X-Indexed Absolute
            cycles += Instructions::ORA(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x1e: //ASL - X-Indexed Absolute
            Instructions::ASL(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x20: //JSR - Absolute
            Instructions::JSR(this, opcode, ABSOLUTE);
            break;
        case 0x21: //AND - X-Indexed Zero Page Indirect
            Instructions::AND(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x24: //BIT- Zero Page
            Instructions::BIT(this, opcode, ZERO_PAGE);
            break;
        case 0x25: //AND - Zero Page
            Instructions::AND(this, opcode, ZERO_PAGE);
            break;
        case 0x26: //ROL - Zero Page
            Instructions::ROL(this, opcode, ZERO_PAGE);
            break;
        case 0x28: //PLP
            Instructions::PLP(this);
            break;
        case 0x29: //AND - Immediate
            Instructions::AND(this, opcode, IMMEDIATE);
            break;
        case 0x2a: //ROL - Accumulator
            Instructions::ROL(this, opcode, ACCUMULATOR);
            break;
        case 0x2c: //BIT - Absolute
            Instructions::BIT(this, opcode, ABSOLUTE);
            break;
        case 0x2d: //AND - Absolute
            Instructions::AND(this, opcode, ABSOLUTE);
            break;
        case 0x2e: //ROL - Absolute
            Instructions::ROL(this, opcode, ABSOLUTE);
            break;
        case 0x30: //BMI
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.n == 1);
            break;
        case 0x31: //AND - Zero Page Indirect Y-Indexed
            cycles += Instructions::AND(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x35: //AND - X-Indexed Zero Page
            Instructions::AND(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x36: //ROL - X-Indexed Zero Page
            Instructions::ROL(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x38: //SEC
            this->ps.c = 1;
            break;
        case 0x39: //AND - Y-Indexed Absolute
            cycles += Instructions::AND(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x3d: //AND - X-Indexed Absolute
            cycles += Instructions::AND(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x3e: //ROL - X-Indexed Absolute
            Instructions::ROL(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x40: //RTI
            Instructions::RTI(this);
            break;
        case 0x41: //EOR - X-Indexed Zero Page Indirect
            Instructions::EOR(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x45: //EOR - Zero Page
            Instructions::EOR(this, opcode, ZERO_PAGE);
            break;
        case 0x46: //LSR - Zero Page
            Instructions::LSR(this, opcode, ZERO_PAGE);
            break;
        case 0x48: //PHA
            Instructions::PHA(this);
            break;
        case 0x49: //EOR - Immediate
            Instructions::EOR(this, opcode, IMMEDIATE);
            break;
        case 0x4a: //LSR - Accumulator
            Instructions::LSR(this, opcode, ACCUMULATOR);
            break;
        case 0x4c: //JMP - Absolute
            Instructions::JMP(this, opcode, ABSOLUTE);
            break;
        case 0x4d: //EOR - Absolute
            Instructions::EOR(this, opcode, ABSOLUTE);
            break;
        case 0x4e: //LSR - Absolute
            Instructions::LSR(this, opcode, ABSOLUTE);
            break;
        case 0x50: //BVC
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.v == 0);
            break;
        case 0x51: //EOR - Zero Page Indirect Y-Indexed
            cycles += Instructions::EOR(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x55: //EOR - X-Indexed Zero Page
            Instructions::EOR(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x56: //LSR - X-Indexed Zero Page
            Instructions::LSR(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x58: //CLI
            this->ps.i = 0;
            break;
        case 0x59: //EOR - Y-Indexed Absolute
            cycles += Instructions::EOR(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x5d: //EOR - X-Indexed Absolute
            cycles += Instructions::EOR(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x5e: //LSR - X-Indexed Absolute
            Instructions::LSR(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x60: //RTS
            Instructions::RTS(this);
            break;
        case 0x61: //ADC - X-Indexed Zero Page Indirect
            Instructions::ADC(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x65: //ADC - Zero Page
            Instructions::ADC(this, opcode, ZERO_PAGE);
            break;
        case 0x66: //ROR - Zero Page
            Instructions::ROR(this, opcode, ZERO_PAGE);
            break;
        case 0x68: //PLA
            Instructions::PLA(this);
            break;
        case 0x69: //ADC - Immediate
            Instructions::ADC(this, opcode, IMMEDIATE);
            break;
        case 0x6a: //ROR - Accumulator
            Instructions::ROR(this, opcode, ACCUMULATOR);
            break;
        case 0x6c: //JMP - Absolute Indirect
            Instructions::JMP(this, opcode, ABSOLUTE_INDIRECT);
            break;
        case 0x6d: //ADC - Absolute
            Instructions::ADC(this, opcode, ABSOLUTE);
            break;
        case 0x6e: //ROR - Absolute
            Instructions::ROR(this, opcode, ABSOLUTE);
            break;
        case 0x70: //BVS
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.v == 1);
            break;
        case 0x71: //ADC - Zero Page Indirect Y-Indexed
            cycles += Instructions::ADC(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x75: //ADC - X-Indexed Zero Page
            Instructions::ADC(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x76: //ROR - X-Indexed Zero Page
            Instructions::ROR(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x78: //SEI
            this->ps.i = 1;
            break;
        case 0x79: //ADC - Y-Indexed Absolute
            cycles += Instructions::ADC(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x7d: //ADC - X-Indexed Absolute
            cycles += Instructions::ADC(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x7e: //ROR - X-Indexed Absolute
            Instructions::ROR(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x81: //STA - X-Indexed Zero Page Indirect
            Instructions::STA(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x84: //STY - Zero Page
            Instructions::STY(this, opcode, ZERO_PAGE);
            break;
        case 0x85: //STA - Zero Page
            Instructions::STA(this, opcode, ZERO_PAGE);
            break;
        case 0x86: //STX - Zero Page
            Instructions::STX(this, opcode, ZERO_PAGE);
            break;
        case 0x88: //DEY - Implied
            Instructions::DEC_INDEX(this, this->y);
            break;
        case 0x8a: //TXA - Implied
            Instructions::TRANSFER(this, this->x, &this->a);
            break;
        case 0x8c: //STY - Absolute
            Instructions::STY(this, opcode, ABSOLUTE);
            break;
        case 0x8d: //STA - Absolute
            Instructions::STA(this, opcode, ABSOLUTE);
            break;
        case 0x8e: //STX - Absolute
            Instructions::STX(this, opcode, ABSOLUTE);
            break;
        case 0x90: //BCC - Relative
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.c == 0);
            break;
        case 0x91: //STA - Zero Page Indirect Y-Indexed
            Instructions::STA(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x94: //STY - X-Indexed Zero Page
            Instructions::STY(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x95: //STA - X-Indexed Zero Page
            Instructions::STA(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x96: //STX - Y-Indexed Zero Page
            Instructions::STY(this, opcode, Y_INDEXED_ZERO_PAGE);
            break;
        case 0x98: //TYA - Implied
            Instructions::TRANSFER(this, this->y, &this->a);
            break;
        case 0x99: //STA - Y-Indexed Absolute
            Instructions::STA(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x9a: //TXS - Implied
            Instructions::TRANSFER(this, this->x, &this->s, false);
            break;
        case 0x9d: //STA - X-Indexed Absolute
            Instructions::STA(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xa0: //LDY - Immediate
            Instructions::LDY(this, opcode, IMMEDIATE);
            break;
        case 0xa1: //LDA - X-Indexed Zero Page Indirect
            Instructions::LDA(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xa2: //LDX - Immediate
            Instructions::LDX(this, opcode, IMMEDIATE);
            break;
        case 0xa4: //LDY - Zero Page
            Instructions::LDY(this, opcode, ZERO_PAGE);
            break;
        case 0xa5: //LDA - Zero Page
            Instructions::LDA(this, opcode, ZERO_PAGE);
            break;
        case 0xa6: //LDX - Zero Page
            Instructions::LDX(this, opcode, ZERO_PAGE);
            break;
        case 0xa8: //TAY
            Instructions::TRANSFER(this, this->a, &this->y);
            break;
        case 0xa9: //LDA - Immediate
            Instructions::LDA(this, opcode, IMMEDIATE);
            break;
        case 0xaa: //TAX
            Instructions::TRANSFER(this, this->a, &this->x);
            break;
        case 0xac: //LDY - Absolute
            Instructions::LDY(this, opcode, ABSOLUTE);
            break;
        case 0xad: //LDA - Absolute
            Instructions::LDA(this, opcode, ABSOLUTE);
            break;
        case 0xae: //LDX - Absolute
            Instructions::LDX(this, opcode, ABSOLUTE);
            break;
        case 0xb0: //BCS - Relative
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.c == 1);
            break;
        case 0xb1: //LDA - Zero Page Indirect Y-Indexed
            cycles += Instructions::LDA(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xb4: //LDY - X-Indexed Zero Page
            Instructions::LDY(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xb5: //LDA - X-Indexed Zero Page
            Instructions::LDA(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xb6: //LDX - Y-Indexed Zero Page
            Instructions::LDX(this, opcode, Y_INDEXED_ZERO_PAGE);
            break;
        case 0xb8: //CLV
            this->ps.v = 0;
            break;
        case 0xb9: //LDA - Y-Indexed Absolute
            cycles += Instructions::LDA(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xba: //TSX
            Instructions::TRANSFER(this, this->s, &this->x);
            break;
        case 0xbc: //LDY - X-Indexed Absolute
            cycles += Instructions::LDY(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xbd: //LDA - X-Indexed Absolute
            cycles += Instructions::LDA(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xbe: //LDX - Y-Indexed Absolute
            cycles += Instructions::LDX(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xc0: //CPY - Immediate
            Instructions::CMP_INDEX(this, opcode, IMMEDIATE, this->y);
            break;
        case 0xc1: //CMP - X-Indexed Zero Page Indirect
            Instructions::CMP_INDEX(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT, this->a);
            break;
        case 0xc4: //CPY - Zero Page
            Instructions::CMP_INDEX(this, opcode, ZERO_PAGE, this->y);
            break;
        case 0xc5: //CMP - Zero Page
            Instructions::CMP_INDEX(this, opcode, ZERO_PAGE, this->a);
            break;
        case 0xc6: //DEC - Zero Page
            Instructions::DEC(this, opcode, ZERO_PAGE);
            break;
        case 0xc8: //INY - Implied
            Instructions::INC_INDEX(this, this->y);
            break;
        case 0xc9: //CMP - Immediate
            Instructions::CMP_INDEX(this, opcode, IMMEDIATE, this->a);
            break;
        case 0xca: //DEX - Implied
            Instructions::DEC_INDEX(this, this->x);
            break;
        case 0xcc: //CPY - Absolute
            Instructions::CMP_INDEX(this, opcode, ABSOLUTE, this->y);
            break;
        case 0xcd: //CMP - Absolute
            Instructions::CMP_INDEX(this, opcode, ABSOLUTE, this->a);
            break;
        case 0xce: //DEC - Absolute
            Instructions::DEC(this, opcode, ABSOLUTE);
            break;
        case 0xd0: //BNE
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.z == 0);
            break;
        case 0xd1: //CMP - Zero Page Indirect Y-Indexed
            cycles += Instructions::CMP_INDEX(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED, this->a);
            break;
        case 0xd5: //CMP - X-Indexed Zero Page
            Instructions::CMP_INDEX(this, opcode, X_INDEXED_ZERO_PAGE, this->a);
            break;
        case 0xd6: //DEC - X-Indexed Zero Page
            Instructions::DEC(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xd8: //CLD
            this->ps.d = 0;
            break;
        case 0xd9: //CMP - Y-Indexed Absolute
            cycles += Instructions::CMP_INDEX(this, opcode, Y_INDEXED_ABSOLUTE, this->a);
            break;
        case 0xdd: //CMP - X-Indexed Absolute
            cycles += Instructions::CMP_INDEX(this, opcode, X_INDEXED_ABSOLUTE, this->a);
            break;
        case 0xde: //DEC - X-Indexed Absolute
            Instructions::DEC(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xe0: //CPX - Immediate
            Instructions::CMP_INDEX(this, opcode, IMMEDIATE, this->x);
            break;
        case 0xe1: //SBC - X-Indexed Zero Page Indirect
            Instructions::SBC(this, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xe4: //CPX- Zero Page
            Instructions::CMP_INDEX(this, opcode, ZERO_PAGE, this->x);
            break;
        case 0xe5: //SBC - Zero Page
            Instructions::SBC(this, opcode, ZERO_PAGE);
            break;
        case 0xe6: //INC - Zero Page
            Instructions::INC(this, opcode, ZERO_PAGE);
            break;
        case 0xe8: //INX
            Instructions::INC_INDEX(this, this->x);
            break;
        case 0xe9: //SBC - Immediate
            Instructions::SBC(this, opcode, IMMEDIATE);
            break;
        case 0xea: //NOP
            break;
        case 0xec: //CPX - Absolute
            Instructions::CMP_INDEX(this, opcode, ABSOLUTE, this->x);
            break;
        case 0xed: //SBC - Absolute
            Instructions::SBC(this, opcode, ABSOLUTE);
            break;
        case 0xee: //INC - Absolute
            Instructions::INC(this, opcode, ABSOLUTE);
            break;
        case 0xf0: //BEQ
            cycles += Instructions::BRANCH(&this->pc.val, opcode, this->ps.z == 1);
            break;
        case 0xf1: //SBC - Zero Page Indirect Y-Indexed
            cycles += Instructions::SBC(this, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xf5: //SBC - X-Indexed Zero Page
            Instructions::SBC(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xf6: //INC - X-Indexed Zero Page
            Instructions::INC(this, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xf8: //SED
            this->ps.d = 1;
            break;
        case 0xf9: //SBC - Y-Indexed Absolute
            cycles += Instructions::SBC(this, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xfd: //SBC - X-Indexed Absolute
            cycles += Instructions::SBC(this, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xfe: //INC - X-Indexed Absolute
            Instructions::INC(this, opcode, X_INDEXED_ABSOLUTE);
            break;
    }
    
    std::cout << "A: " << static_cast<int>(this->a) << " X: " << static_cast<int>(this->x) << " Y: " << static_cast<int>(this->y) << " S: " << static_cast<int>(this->s) << std::endl;
    std::cout << "C: " << static_cast<int>(this->ps.c) << " Z: " << static_cast<int>(this->ps.z) << " I: " << static_cast<int>(this->ps.i) << " D: " << static_cast<int>(this->ps.d) << " B: " << static_cast<int>(this->ps.b) << " V:" << static_cast<int>(this->ps.v) << " N: " << static_cast<int>(this->ps.n) << std::endl;
    
    
    return cycles;
}

void cpu6502::disassemble()
{
    using namespace AddressingModeFuncs;
    
    uint8_t *opcode = this->memory.getBaseAddress() + this->pc.val;
    this->pc.val += 1;
    
    switch (*opcode)
    {
        case 0x00: //BRK
            printf("%04x\tBRK\n", this->pc.val);
            break;
        case 0x01: //ORA - X-Indexed Zero Page Indirect
            printf("%04x\tORA ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x05: //ORA - Zero Page
            printf("%04x\tORA $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x06: //ASL - Zero Page
            printf("%04x\tASL $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x08: //PHP
            printf("%04x\tPHP\n", this->pc.val);
            break;
        case 0x09: //ORA - Immediate
            printf("%04x\tORA #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0x0a: //ASL - Accumulator
            printf("%04x\tASL A\n", this->pc.val);
            break;
        case 0x0d: //ORA - Absolute
            printf("%04x\tORA $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x0e: //ASL - Absolute
            printf("%04x\tASL $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x10: //BPL
            printf("%04x\tBPL $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0x11: //ORA - Zero Page Indirect Y-Indexed
            printf("%04x\tORA ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x15: //ORA - X-Indexed Zero Page
            printf("%04x\tORA $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x16: //ASL - X-Indexed Zero Page
            printf("%04x\tASL $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x18: //CLC
            printf("%04x\tCLC\n", this->pc.val);
            break;
        case 0x19: //ORA - Y-Indexed Absolute
            printf("%04x\tORA $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x1d: //ORA - X-Indexed Absolute
            printf("%04x\tORA $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x1e: //ASL - X-Indexed Absolute
            printf("%04x\tASL $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x20: //JSR - Absolute
            printf("%04x\tJSR $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x21: //AND - X-Indexed Zero Page Indirect
            printf("%04x\tAND ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x24: //BIT- Zero Page
            printf("%04x\tBIT $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x25: //AND - Zero Page
            printf("%04x\tAND $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x26: //ROL - Zero Page
            printf("%04x\tROL $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x28: //PLP
            printf("%04x\tPLP\n", this->pc.val);
            break;
        case 0x29: //AND - Immediate
            printf("%04x\tAND #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0x2a: //ROL - Accumulator
            printf("%04x\tROL A\n", this->pc.val);
            break;
        case 0x2c: //BIT - Absolute
            printf("%04x\tBIT $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x2d: //AND - Absolute
            printf("%04x\tAND $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x2e: //ROL - Absolute
            printf("%04x\tROL $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x30: //BMI
            printf("%04x\tBMI $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0x31: //AND - Zero Page Indirect Y-Indexed
            printf("%04x\tAND ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x35: //AND - X-Indexed Zero Page
            printf("%04x\tAND $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x36: //ROL - X-Indexed Zero Page
            printf("%04x\tROL $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x38: //SEC
            printf("%04x\tSEC\n", this->pc.val);
            break;
        case 0x39: //AND - Y-Indexed Absolute
            printf("%04x\tAND $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x3d: //AND - X-Indexed Absolute
            printf("%04x\tAND $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x3e: //ROL - X-Indexed Absolute
            printf("%04x\tROL $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x40: //RTI
            printf("%04x\tRTI\n", this->pc.val);
            break;
        case 0x41: //EOR - X-Indexed Zero Page Indirect
            printf("%04x\tEOR ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x45: //EOR - Zero Page
            printf("%04x\tEOR $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x46: //LSR - Zero Page
            printf("%04x\tLSR $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x48: //PHA
            printf("%04x\tPHA\n", this->pc.val);
            break;
        case 0x49: //EOR - Immediate
            printf("%04x\tEOR #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0x4a: //LSR - Accumulator
            printf("%04x\tLSR A\n", this->pc.val);
            break;
        case 0x4c: //JMP - Absolute
            printf("%04x\tJMP $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x4d: //EOR - Absolute
            printf("%04x\tEOR $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x4e: //LSR - Absolute
            printf("%04x\tLSR $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x50: //BVC
            printf("%04x\tBVC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0x51: //EOR - Zero Page Indirect Y-Indexed
            printf("%04x\tEOR ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x55: //EOR - X-Indexed Zero Page
            printf("%04x\tEOR $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x56: //LSR - X-Indexed Zero Page
            printf("%04x\tLSR $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x58: //CLI
            printf("%04x\tCLI\n", this->pc.val);
            break;
        case 0x59: //EOR - Y-Indexed Absolute
            printf("%04x\tEOR $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x5d: //EOR - X-Indexed Absolute
            printf("%04x\tEOR $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x5e: //LSR - X-Indexed Absolute
            printf("%04x\tLSR $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x60: //RTS
            printf("%04x\tRTS\n", this->pc.val);
            break;
        case 0x61: //ADC - X-Indexed Zero Page Indirect
            printf("%04x\tADC ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x65: //ADC - Zero Page
            printf("%04x\tADC $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x66: //ROR - Zero Page
            printf("%04x\tROR $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x68: //PLA
            printf("%04x\tPLA\n", this->pc.val);
            break;
        case 0x69: //ADC - Immediate
            printf("%04x\tADC #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0x6a: //ROR - Accumulator
            printf("%04x\tROR A\n", this->pc.val);
            break;
        case 0x6c: //JMP - Absolute Indirect
            printf("%04x\tJMP ($%02x%02x)\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE_INDIRECT);
            break;
        case 0x6d: //ADC - Absolute
            printf("%04x\tADC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x6e: //ROR - Absolute
            printf("%04x\tROR $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x70: //BVS
            printf("%04x\tBVS $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0x71: //ADC - Zero Page Indirect Y-Indexed
            printf("%04x\tADC ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x75: //ADC - X-Indexed Zero Page
            printf("%04x\tADC $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x76: //ROR - X-Indexed Zero Page
            printf("%04x\tROR $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x78: //SEI
            printf("%04x\tSEI\n", this->pc.val);
            break;
        case 0x79: //ADC - Y-Indexed Absolute
            printf("%04x\tADC $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x7d: //ADC - X-Indexed Absolute
            printf("%04x\tADC $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x7e: //ROR - X-Indexed Absolute
            printf("%04x\tROR $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x81: //STA - X-Indexed Zero Page Indirect
            printf("%04x\tSTA ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x84: //STY - Zero Page
            printf("%04x\tSTY $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x85: //STA - Zero Page
            printf("%04x\tSTA $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x86: //STX - Zero Page
            printf("%04x\tSTX $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0x88: //DEY - Implied
            printf("%04x\tDEY\n", this->pc.val);
            break;
        case 0x8a: //TXA - Implied
            printf("%04x\tTXA\n", this->pc.val);
            break;
        case 0x8c: //STY - Absolute
            printf("%04x\tSTY $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x8d: //STA - Absolute
            printf("%04x\tSTA $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x8e: //STX - Absolute
            printf("%04x\tSTX $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0x90: //BCC - Relative
            printf("%04x\tBCC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0x91: //STA - Zero Page Indirect Y-Indexed
            printf("%04x\tSTA ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x94: //STY - X-Indexed Zero Page
            printf("%04x\tSTY $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x95: //STA - X-Indexed Zero Page
            printf("%04x\tSTA $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x96: //STX - Y-Indexed Zero Page
            printf("%04x\tSTX $%02x,Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ZERO_PAGE);
            break;
        case 0x98: //TYA - Implied
            printf("%04x\tTYA\n", this->pc.val);
            break;
        case 0x99: //STA - Y-Indexed Absolute
            printf("%04x\tSTA $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x9a: //TXS - Implied
            printf("%04x\tTXS\n", this->pc.val);
            break;
        case 0x9d: //STA - X-Indexed Absolute
            printf("%04x\tSTA $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xa0: //LDY - Immediate
            printf("%04x\tLDY #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xa1: //LDA - X-Indexed Zero Page Indirect
            printf("%04x\tLDA ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xa2: //LDX - Immediate
            printf("%04x\tLDX #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xa4: //LDY - Zero Page
            printf("%04x\tLDY $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xa5: //LDA - Zero Page
            printf("%04x\tLDA $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xa6: //LDX - Zero Page
            printf("%04x\tLDX $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xa8: //TAY
            printf("%04x\tTAY\n", this->pc.val);
            break;
        case 0xa9: //LDA - Immediate
            printf("%04x\tLDA #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xaa: //TAX
            printf("%04x\tTAX\n", this->pc.val);
            break;
        case 0xac: //LDY - Absolute
            printf("%04x\tLDY $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xad: //LDA - Absolute
            printf("%04x\tLDA $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xae: //LDX - Absolute
            printf("%04x\tLDX $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xb0: //BCS - Relative
            printf("%04x\tBCS $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0xb1: //LDA - Zero Page Indirect Y-Indexed
            printf("%04x\tLDA ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xb4: //LDY - X-Indexed Zero Page
            printf("%04x\tLDY $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xb5: //LDA - X-Indexed Zero Page
            printf("%04x\tLDA $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xb6: //LDX - Y-Indexed Zero Page
            printf("%04x\tLDX $%02x,Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ZERO_PAGE);
            break;
        case 0xb8: //CLV
            printf("%04x\tCLV\n", this->pc.val);
            break;
        case 0xb9: //LDA - Y-Indexed Absolute
            printf("%04x\tLDA $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xba: //TSX
            printf("%04x\tTSX\n", this->pc.val);
            break;
        case 0xbc: //LDY - X-Indexed Absolute
            printf("%04x\tLDY $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xbd: //LDA - X-Indexed Absolute
            printf("%04x\tLDA $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xbe: //LDX - Y-Indexed Absolute
            printf("%04x\tLDX $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xc0: //CPY - Immediate
            printf("%04x\tCPY #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xc1: //CMP - X-Indexed Zero Page Indirect
            printf("%04x\tCMP ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xc4: //CPY - Zero Page
            printf("%04x\tCPY $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xc5: //CMP - Zero Page
            printf("%04x\tCMP $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xc6: //DEC - Zero Page
            printf("%04x\tDEC $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xc8: //INY - Implied
            printf("%04x\tINY\n", this->pc.val);
            break;
        case 0xc9: //CMP - Immediate
            printf("%04x\tCMP #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xca: //DEX - Implied
            printf("%04x\tDEX\n", this->pc.val);
            break;
        case 0xcc: //CPY - Absolute
            printf("%04x\tCPY $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xcd: //CMP - Absolute
            printf("%04x\tCMP $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xce: //DEC - Absolute
            printf("%04x\tDEC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xd0: //BNE - Relative
            printf("%04x\tBNE $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0xd1: //CMP - Zero Page Indirect Y-Indexed
            printf("%04x\tCMP ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xd5: //CMP - X-Indexed Zero Page
            printf("%04x\tCMP $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xd6: //DEC - X-Indexed Zero Page
            printf("%04x\tDEC $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xd8: //CLD
            printf("%04x\tCLD\n", this->pc.val);
            break;
        case 0xd9: //CMP - Y-Indexed Absolute
            printf("%04x\tCMP $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xdd: //CMP - X-Indexed Absolute
            printf("%04x\tCMP $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xde: //DEC - X-Indexed Absolute
            printf("%04x\tDEC $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xe0: //CPX - Immediate
            printf("%04x\tCPX #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xe1: //SBC - X-Indexed Zero Page Indirect
            printf("%04x\tSBC ($%02x,X)\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xe4: //CPX - Zero Page
            printf("%04x\tCPX $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xe5: //SBC - Zero Page
            printf("%04x\tSBC $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xe6: //INC - Zero Page
            printf("%04x\tINC $%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE);
            break;
        case 0xe8: //INX
            printf("%04x\tINX\n", this->pc.val);
            break;
        case 0xe9: //SBC - Immediate
            printf("%04x\tSBC #%02x\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(IMMEDIATE);
            break;
        case 0xea: //NOP
            printf("%04x\tNOP\n", this->pc.val);
            break;
        case 0xec: //CPX - Absolute
            printf("%04x\tCPX $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xed: //SBC - Absolute
            printf("%04x\tSBC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xee: //INC - Absolute
            printf("%04x\tINC $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(ABSOLUTE);
            break;
        case 0xf0: //BEQ
            printf("%04x\tBEQ $%02x%02x\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(RELATIVE);
            break;
        case 0xf1: //SBC - Zero Page Indirect Y-Indexed
            printf("%04x\tSBC ($%02x),Y\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xf5: //SBC - X-Indexed Zero Page
            printf("%04x\tSBC $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xf6: //INC - X-Indexed Zero Page
            printf("%04x\tINC $%02x,X\n", this->pc.val, opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xf8: //SED
            printf("%04x\tSED\n", this->pc.val);
            break;
        case 0xf9: //SBC - Y-Indexed Absolute
            printf("%04x\tSBC $%02x%02x,Y\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xfd: //SBC - X-Indexed Absolute
            printf("%04x\tSBC $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xfe: //INC - X-Indexed Absolute
            printf("%04x\tINC $%02x%02x,X\n", this->pc.val, opcode[2], opcode[1]);
            this->pc.val += pcByMode(X_INDEXED_ABSOLUTE);
            break;
    }
}



