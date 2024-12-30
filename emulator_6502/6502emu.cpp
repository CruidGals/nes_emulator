//
//  6502emu.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

//TODO: Implement All Instructions, Then Focus on Bit

#include "6502emu.hpp"

#include <iostream>
#include <stdint.h>

enum AddressingMode {
    IMPLIED,
    ACCUMULATOR,                    //A
    IMMEDIATE,                      //#$nn
    ABSOLUTE,                       //$nnnn
    X_INDEXED_ABSOLUTE,             //$nnnn, X
    Y_INDEXED_ABSOLUTE,             //$nnnn, Y
    ABSOLUTE_INDIRECT,              //($nnnn)
    ZERO_PAGE,                      //$nn
    X_INDEXED_ZERO_PAGE,            //$nn,X
    Y_INDEXED_ZERO_PAGE,            //$nn,Y
    X_INDEXED_ZERO_PAGE_INDIRECT,   //($nn,X)
    ZERO_PAGE_INDIRECT_Y_INDEXED,   //($nn),Y
    RELATIVE                        //$nnnn
};

//Addressing Mode Functions -------------------------------------------

//Returns memory offset for Absolute Addressing Mode; index is used if X or Y indexed
uint16_t AbsoluteOffset(const uint8_t *const opcode, const uint8_t index = 0)
{
    return ((static_cast<uint16_t>(opcode[2]) << 8) | static_cast<uint16_t>(opcode[1])) + index;
}

//Returns Zero-Page offset for Absolute Addressing Mode; index is used if X or Y indexed
uint16_t ZPOffset(const uint8_t *const opcode, const uint8_t index = 0)
{
    return static_cast<uint16_t>(opcode[1] + index);
}

//Returns memory offset for X-Indexed Zero Page Indirect Addressing Mode
uint16_t XIndexZPIndirectOffset(const State6502 *const state, const uint8_t *const opcode)
{
    return static_cast<uint16_t>(state->memory[static_cast<uint16_t>(opcode[1] + state->x + 1)]) << 8 | static_cast<uint16_t>(state->memory[static_cast<uint16_t>(opcode[1] + state->x)]);
}

//Returns memory offset for Zero Page Indirect Y-Indexed Addressing Mode
uint16_t YIndexZPIndirectOffset(const State6502 *const state, const uint8_t *const opcode)
{
    return (static_cast<uint16_t>(state->memory[opcode[1] + 1]) << 8 | static_cast<uint16_t>(state->memory[opcode[1]])) + static_cast<uint16_t>(state->y);
}

//Specific function for Absolute Indirect Addressing Mode
uint16_t AbsoluteIndirectOffset(const State6502 *const state, const uint8_t *const opcode)
{
    const uint8_t lowByte = state->memory[static_cast<uint16_t>(opcode[2]) << 8 | static_cast<uint16_t>(opcode[1])];
    
    //Preserves quirk that page is not crossed during Absolute Indirect Addressing (meaning high byte of address is unchanged)
    const uint8_t highByte = state->memory[static_cast<uint16_t>(opcode[2]) << 8 | static_cast<uint16_t>(opcode[1] + 1)];
    
    return static_cast<uint16_t>(highByte) << 8 | static_cast<uint16_t>(lowByte);
}

uint8_t* offsetByMode(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    switch (mode) {
        case ACCUMULATOR: return &state->a;
        case IMMEDIATE: return &opcode[1];
        case ABSOLUTE: return &state->memory[AbsoluteOffset(opcode)];
        case ABSOLUTE_INDIRECT: return &state->memory[AbsoluteIndirectOffset(state, opcode)];
        case X_INDEXED_ABSOLUTE: return &state->memory[AbsoluteOffset(opcode, state->x)];
        case Y_INDEXED_ABSOLUTE: return &state->memory[AbsoluteOffset(opcode, state->y)];
        case ZERO_PAGE: return &state->memory[ZPOffset(opcode)];
        case X_INDEXED_ZERO_PAGE: return &state->memory[ZPOffset(opcode, state->x)];
        case Y_INDEXED_ZERO_PAGE: return &state->memory[ZPOffset(opcode, state->y)];
        case X_INDEXED_ZERO_PAGE_INDIRECT: return &state->memory[XIndexZPIndirectOffset(state, opcode)];
        case ZERO_PAGE_INDIRECT_Y_INDEXED: return &state->memory[YIndexZPIndirectOffset(state, opcode)];
        case RELATIVE: return &opcode[1];
        
        default:
            return nullptr;
    }
}

//Returns the program counter increment for each mode
uint8_t pcByMode(const AddressingMode mode)
{
    switch (mode) {
        case IMMEDIATE: return 1;
        case ABSOLUTE: return 2;
        case X_INDEXED_ABSOLUTE: return 2;
        case Y_INDEXED_ABSOLUTE: return 2;
        case ABSOLUTE_INDIRECT: return 2;
        case ZERO_PAGE: return 1;
        case X_INDEXED_ZERO_PAGE: return 1;
        case Y_INDEXED_ZERO_PAGE: return 1;
        case X_INDEXED_ZERO_PAGE_INDIRECT: return 1;
        case ZERO_PAGE_INDIRECT_Y_INDEXED: return 1;
        case RELATIVE: return 1;
        
        default:
            return 0;
    }
}

//Helper Functions
static uint8_t parseProcessorStatus(const ProcessorStatus *const ps)
{
    return ((ps->n << 7) | (ps->v << 6) | (ps->b << 4) | (ps->d << 3) | (ps->i << 2) | (ps->z << 1) | ps->c);
}

int detectPageCross(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t prev_high = opcode[2];
    uint8_t new_high;
    
    switch (mode)
    {
        case X_INDEXED_ABSOLUTE:
            //Get high bit of new address
            new_high = static_cast<uint8_t>(AbsoluteOffset(opcode, state->x) >> 8);
            break;
        case Y_INDEXED_ABSOLUTE:
            //Get high bit of new address
            new_high = static_cast<uint8_t>(AbsoluteOffset(opcode, state->y) >> 8);
            break;
        case ZERO_PAGE_INDIRECT_Y_INDEXED:
            prev_high = static_cast<uint8_t>((static_cast<uint16_t>(state->memory[opcode[1] + 1] << 8) | static_cast<uint16_t>(state->memory[opcode[1]])) >> 8);
            new_high = static_cast<uint8_t>(YIndexZPIndirectOffset(state, opcode) >> 8);
            break;
        default:
            return 0;
    }
    
    return prev_high != new_high;
}

static void unparseProcessorStatus(ProcessorStatus *const ps, const uint8_t status)
{
    ps->n = (status & 0x80) > 0;
    ps->v = (status & 0x40) > 0;
    ps->b = (status & 0x10) > 0;
    ps->d = (status & 0x08) > 0;
    ps->i = (status & 0x04) > 0;
    ps->z = (status & 0x02) > 0;
    ps->c = (status & 0x01) > 0;
}

//Flags -----------------------------------------

void bitwiseOpFlags(State6502 *const state, uint8_t comp)
{
    state->ps.z = comp == 0x00;
    state->ps.n = 0x80 == (comp & 0x80);
}

//Opcode Instructions ---------------------------

/* ---------------- LOGIC INSTRUCTIONS ---------------- */

//Returns extra cycle if page crossing detected
int ORA(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->a = *offsetByMode(state, opcode, mode) | state->a;
    bitwiseOpFlags(state, state->a);
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

void BIT(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t offset = *offsetByMode(state, opcode, mode);
    uint8_t result = state->a & offset;
    
    state->ps.v = (0x40 == (offset & 0x40));
    state->ps.z = result == 0x00;
    state->ps.n = 0x80 == (offset & 0x80);
    state->pc += pcByMode(mode);
}

//Returns extra cycle if page crossing detected
int AND(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->a = *offsetByMode(state, opcode, mode) & state->a;
    bitwiseOpFlags(state, state->a);
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

//Returns extra cycle if page crossing detected
int EOR(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->a = *offsetByMode(state, opcode, mode) ^ state->a;
    bitwiseOpFlags(state, state->a);
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

/* ---------------- SHIFT (BIT) INSTRUCTIONS ---------------- */

void ASL(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* p = offsetByMode(state, opcode, mode);
    
    //Set carry bit before lost
    state->ps.c = 0x80 == (*p & 0x80);
    
    *p = *p << 1;
    
    //Flags
    state->ps.z = *p == 0x00;
    state->ps.n = 0x80 == (*p & 0x80);
    state->pc += pcByMode(mode);
}

void LSR(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* p = offsetByMode(state, opcode, mode);
    
    //Set carry bit before lost
    state->ps.c = 0x01 == (*p & 0x01);
    
    *p = *p >> 1;
    
    //Rest of Flags
    state->ps.z = *p == 0x00;
    state->ps.n = 0;
    state->pc += pcByMode(mode);
}

/*
 Shifts memory or accumulator left by one bit, with carry bit rotating into the 0th bit and the 7th bit rotating into the carry bit
 Affects carry, zero, negative bit
 */
void ROL(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* offset = offsetByMode(state, opcode, mode);
    uint8_t result = *offset << 1 | state->ps.c;
    
    //Set carry bit before lost
    state->ps.c = 0x80 == (*offset & 0x80);
    
    *offset = result;
    
    state->ps.z = *offset == 0x00;
    state->ps.n = 0x80 == (*offset & 0x80);
    state->pc += pcByMode(mode);
}

/*
 Shifts memory or accumulator right by one bit, with carry bit rotating into the 7th bit and the 0th bit rotating into the carry bit
 Affects carry, zero, negative bit
 */
void ROR(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* offset = offsetByMode(state, opcode, mode);
    uint8_t result = *offset >> 1 | state->ps.c << 7;
    
    //Set carry bit before lost by bit maneuver
    state->ps.c = 0x01 == (*offset & 0x01);
    
    *offset = result;
    state->ps.z = *offset == 0x00;
    state->ps.n = 0x80 == (*offset & 0x80);
    state->pc += pcByMode(mode);
}

/* ---------------- ARITHMETIC INSTRUCTIONS ---------------- */

//Returns extra cycle if page crossing detected
int ADC(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t offset = *offsetByMode(state, opcode, mode);
    uint16_t result = static_cast<uint16_t>(state->a) + static_cast<uint16_t>(offset) + state->ps.c;
    
    //Set overflow flag before state->a is updated
    state->ps.v = (((state->a ^ offset) & (state->a ^ result) & 0x80) != 0); //Overflow if num both negative/positive (given by 7th bit)
    
    state->a = static_cast<uint8_t>(result);
    
    //Flags
    state->ps.c = 0x0100 == (result & 0x0100);
    state->ps.z = state->a == 0x00;
    state->ps.n = 0x80 == (state->a & 0x80);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

//Returns extra cycle if page crossing detected
int SBC(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t offset = *offsetByMode(state, opcode, mode);
    uint16_t result = static_cast<uint16_t>(state->a) - static_cast<uint16_t>(offset) - (0x0001 - state->ps.c);
    
    //Set overflow flag before state->a is updated
    state->ps.v = (((state->a ^ result) & (state->a ^ offset) & 0x80) != 0); //Overflow if num both negative/positive (given by 7th bit)
    
    state->a = static_cast<uint8_t>(result);
    
    //Flags
    state->ps.c = result <= UINT8_MAX;
    state->ps.z = state->a == 0x00;
    state->ps.n = 0x80 == (state->a & 0x80);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

/*
 Covers CMP, CPX, and CPY
 Returns extra cycle if page crossing detected
 */
int CMP_INDEX(State6502 *const state, uint8_t *const opcode, const AddressingMode mode, const uint8_t index)
{
    uint8_t offset = *offsetByMode(state, opcode, mode);
    uint8_t result = index - offset;
    
    //Flags
    state->ps.c = index >= offset;
    bitwiseOpFlags(state, result);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

/* ---------------- LOAD INSTRUCTIONS ---------------- */

void STA(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    *offsetByMode(state, opcode, mode) = state->a;
    state->pc += pcByMode(mode);
}

void STX(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    *offsetByMode(state, opcode, mode) = state->x;
    state->pc += pcByMode(mode);
}

void STY(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    *offsetByMode(state, opcode, mode) = state->y;
    state->pc += pcByMode(mode);
}

//Returns extra cycle if page crossing detected
int LDA(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->a = *offsetByMode(state, opcode, mode);
    bitwiseOpFlags(state, state->a);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

//Returns extra cycle if page crossing detected
int LDX(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->x = *offsetByMode(state, opcode, mode);
    bitwiseOpFlags(state, state->x);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

//Returns extra cycle if page crossing detected
int LDY(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    state->y = *offsetByMode(state, opcode, mode);
    bitwiseOpFlags(state, state->y);
    
    state->pc += pcByMode(mode);
    
    return detectPageCross(state, opcode, mode);
}

/* ---------------- TRANSFER INSTRUCTIONS ---------------- */

/*
 Transfer values from one variable to another.
 Covers transfer instructions TAX, TAY, TSX, TXA, TXS. TYA
 */
void TRANSFER(State6502 *const state, const uint8_t value, uint8_t *const var, const bool affect_flags = true)
{
    //Transfer value to variable
    *var = value;
    
    //Only TXS doesn't affect flags
    if (affect_flags) bitwiseOpFlags(state, *var);
}

/* ---------------- INCREMENT/DECREMENT INSTRUCTIONS ---------------- */

/*
 Covers decrements instructions DEY and DEX
 */
void DEC_INDEX(State6502 *const state, uint8_t *const index)
{
    (*index)--;
    bitwiseOpFlags(state, *index);
}

void DEC(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* offset = offsetByMode(state, opcode, mode);
    (*offset)--;
    bitwiseOpFlags(state, *offset);
    state->pc += pcByMode(mode);
}

/*
 Covers decrements instructions INY and INX
 */
void INC_INDEX(State6502 *const state, uint8_t *const index)
{
    (*index)++;
    bitwiseOpFlags(state, *index);
}

void INC(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    uint8_t* offset = offsetByMode(state, opcode, mode);
    (*offset)++;
    bitwiseOpFlags(state, *offset);
    state->pc += pcByMode(mode);
}

/* ---------------- BRANCH INSTRUCTIONS ---------------- */

/*
 Condition branches (similar to if statements, but tests bits on processor status)
 Returns extra number of cycles if page crossed or branch is taken
 Covers BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS instructions
 */
int BRANCH(uint16_t *const pc, uint8_t *const opcode, const bool test_set)
{
    int cycles = 0;
    
    if (test_set)
    {
        //Used to handle page crossing cycle increment
        const uint8_t prev_high = static_cast<uint8_t>(*pc >> 8);
        
        //Branch taken
        ++cycles;
        *pc += static_cast<int8_t>(opcode[1]); //Turn into a signed offset
        
        const uint8_t new_high = static_cast<uint8_t>(*pc >> 8);
        
        //New pc high bytes don't match, means page crossed
        if (prev_high != new_high) ++cycles;
    }
    
    //Branch functions itself take 2 bytes
    *pc += pcByMode(RELATIVE);
    
    return cycles;
}

/* ---------------- STACK INSTRUCTIONS ---------------- */

void PHA(State6502 *const state)
{
    state->memory[0x100 | state->s] = state->a;
    state->s--;
}

void PHP(State6502 *const state)
{
    //Parse Processor Status into uin8_t
    uint8_t status = parseProcessorStatus(&state->ps);
    state->memory[0x100 | state->s] = status;
    state->s--;
}

void PLA(State6502 *const state)
{
    state->s++;
    state->a = state->memory[0x100 | state->s];
    bitwiseOpFlags(state, state->a);
}

void PLP(State6502 *const state)
{
    state->s++;
    uint8_t status = state->memory[0x100 | state->s];
    
    //Unparse Processor Status
    unparseProcessorStatus(&state->ps, status);
    
    //Break flag not restored though (lowk ignored)
    state->ps.b = 0;
}

/* ---------------- CONTROL INSTRUCTIONS ---------------- */

void BRK(State6502 *const state)
{
    state->pc++;
    
    //Push PC onto stack
    state->memory[0x100 | state->s] = static_cast<uint8_t>(state->pc & 0x80);
    state->s--;
    state->memory[0x100 | state->s] = static_cast<uint8_t>(state->pc >> 8);
    state->s--;
    
    //Set Break Flag and Push status Stack
    state->memory[0x100 | state->s] = parseProcessorStatus(&state->ps);
    state->s--;
    
    //Set Interrupt Flag
    state->ps.i = 1;
    
    //Load Interrupt vector into memory
    state->pc = (static_cast<uint16_t>(state->memory[0xFFFF]) << 8) | static_cast<uint16_t>(state->memory[0xFFFE]);
}

void RTI(State6502 *const state)
{
    // Get processor status
    unparseProcessorStatus(&state->ps, state->memory[0x100 | (state->s + 1)]);
    
    //Load program counter (subtract by one to get exact memory address next time through)
    state->pc = static_cast<uint16_t>(state->memory[0x100 | (state->s + 3)]) << 8 | static_cast<uint16_t>(state->memory[0x100 | (state->s + 2)]);
    state->pc--;
    
    state->s += 3;
}

void RTS(State6502 *const state)
{
    //Load program counter (subtract by one to get exact memory address next time through)
    state->pc = static_cast<uint16_t>(state->memory[0x100 | (state->s + 2)]) << 8 | static_cast<uint16_t>(state->memory[0x100 | (state->s + 1)]);
    
    state->s += 2;
}

void JMP(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    //In order to return correct address, subtract by state->memory to eliminate arbitrary memory placement
    state->pc = static_cast<uint16_t>(offsetByMode(state, opcode, mode) - state->memory);
}

void JSR(State6502 *const state, uint8_t *const opcode, const AddressingMode mode)
{
    //PC incremented in emulate(), need to store (pc + 2) into stack
    state->pc++;
    
    //Gets correct address by subtracting where state->memory begins
    uint16_t offset = (offsetByMode(state, opcode, mode) - state->memory);
    
    //Load program counter onto stack
    state->memory[state->s] = static_cast<uint8_t>(state->pc >> 8);
    state->s--;
    state->memory[state->s] = static_cast<uint8_t>(state->pc);
    state->s--;
    
    //Redirect program counter
    state->pc = offset;
}

/* ---------------- FLAG INSTRUCTIONS ---------------- */
/* Already written within switch statement (one liner) */

int emulate(State6502 *state)
{
    uint8_t *opcode = &state->memory[state->pc];
    //std::cout << "Opcode: " << std::hex << static_cast<int>(*opcode) << std::dec << " PC: " << static_cast<int>(state->pc) << std::endl;
    state->pc += 1;
    
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
            BRK(state);
            break;
        case 0x01: //ORA - X-Indexed Zero Page Indirect
            ORA(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x05: //ORA - Zero Page
            ORA(state, opcode, ZERO_PAGE);
            break;
        case 0x06: //ASL - Zero Page
            ASL(state, opcode, ZERO_PAGE);
            break;
        case 0x08: //PHP
            PHP(state);
            break;
        case 0x09: //ORA - Immediate
            ORA(state, opcode, IMMEDIATE);
            break;
        case 0x0a: //ASL - Accumulator
            ASL(state, opcode, ACCUMULATOR);
            break;
        case 0x0d: //ORA - Absolute
            ORA(state, opcode, ABSOLUTE);
            break;
        case 0x0e: //ASL - Absolute
            ASL(state, opcode, ABSOLUTE);
            break;
        case 0x10: //BPL
            cycles += BRANCH(&state->pc, opcode, state->ps.n == 0);
            break;
        case 0x11: //ORA - Zero Page Indirect Y-Indexed
            cycles += ORA(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x15: //ORA - X-Indexed Zero Page
            ORA(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x16: //ASL - X-Indexed Zero Page
            ASL(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x18: //CLC
            state->ps.c = 0;
            break;
        case 0x19: //ORA - Y-Indexed Absolute
            cycles += ORA(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x1d: //ORA - X-Indexed Absolute
            cycles += ORA(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x1e: //ASL - X-Indexed Absolute
            ASL(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x20: //JSR - Absolute
            JSR(state, opcode, ABSOLUTE);
            break;
        case 0x21: //AND - X-Indexed Zero Page Indirect
            AND(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x24: //BIT- Zero Page
            BIT(state, opcode, ZERO_PAGE);
            break;
        case 0x25: //AND - Zero Page
            AND(state, opcode, ZERO_PAGE);
            break;
        case 0x26: //ROL - Zero Page
            ROL(state, opcode, ZERO_PAGE);
            break;
        case 0x28: //PLP
            PLP(state);
            break;
        case 0x29: //AND - Immediate
            AND(state, opcode, IMMEDIATE);
            break;
        case 0x2a: //ROL - Accumulator
            ROL(state, opcode, ACCUMULATOR);
            break;
        case 0x2c: //BIT - Absolute
            BIT(state, opcode, ABSOLUTE);
            break;
        case 0x2d: //AND - Absolute
            AND(state, opcode, ABSOLUTE);
            break;
        case 0x2e: //ROL - Absolute
            ROL(state, opcode, ABSOLUTE);
            break;
        case 0x30: //BMI
            cycles += BRANCH(&state->pc, opcode, state->ps.n == 1);
            break;
        case 0x31: //AND - Zero Page Indirect Y-Indexed
            cycles += AND(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x35: //AND - X-Indexed Zero Page
            AND(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x36: //ROL - X-Indexed Zero Page
            ROL(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x38: //SEC
            state->ps.c = 1;
            break;
        case 0x39: //AND - Y-Indexed Absolute
            cycles += AND(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x3d: //AND - X-Indexed Absolute
            cycles += AND(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x3e: //ROL - X-Indexed Absolute
            ROL(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x40: //RTI
            RTI(state);
            break;
        case 0x41: //EOR - X-Indexed Zero Page Indirect
            EOR(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x45: //EOR - Zero Page
            EOR(state, opcode, ZERO_PAGE);
            break;
        case 0x46: //LSR - Zero Page
            LSR(state, opcode, ZERO_PAGE);
            break;
        case 0x48: //PHA
            PHA(state);
            break;
        case 0x49: //EOR - Immediate
            EOR(state, opcode, IMMEDIATE);
            break;
        case 0x4a: //LSR - Accumulator
            LSR(state, opcode, ACCUMULATOR);
            break;
        case 0x4c: //JMP - Absolute
            JMP(state, opcode, ABSOLUTE);
            break;
        case 0x4d: //EOR - Absolute
            EOR(state, opcode, ABSOLUTE);
            break;
        case 0x4e: //LSR - Absolute
            LSR(state, opcode, ABSOLUTE);
            break;
        case 0x50: //BVC
            cycles += BRANCH(&state->pc, opcode, state->ps.v == 0);
            break;
        case 0x51: //EOR - Zero Page Indirect Y-Indexed
            cycles += EOR(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x55: //EOR - X-Indexed Zero Page
            EOR(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x56: //LSR - X-Indexed Zero Page
            LSR(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x58: //CLI
            state->ps.i = 0;
            break;
        case 0x59: //EOR - Y-Indexed Absolute
            cycles += EOR(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x5d: //EOR - X-Indexed Absolute
            cycles += EOR(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x5e: //LSR - X-Indexed Absolute
            LSR(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x60: //RTS
            RTS(state);
            break;
        case 0x61: //ADC - X-Indexed Zero Page Indirect
            ADC(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x65: //ADC - Zero Page
            ADC(state, opcode, ZERO_PAGE);
            break;
        case 0x66: //ROR - Zero Page
            ROR(state, opcode, ZERO_PAGE);
            break;
        case 0x68: //PLA
            PLA(state);
            break;
        case 0x69: //ADC - Immediate
            ADC(state, opcode, IMMEDIATE);
            break;
        case 0x6a: //ROR - Accumulator
            ROR(state, opcode, ACCUMULATOR);
            break;
        case 0x6c: //JMP - Absolute Indirect
            JMP(state, opcode, ABSOLUTE_INDIRECT);
            break;
        case 0x6d: //ADC - Absolute
            ADC(state, opcode, ABSOLUTE);
            break;
        case 0x6e: //ROR - Absolute
            ROR(state, opcode, ABSOLUTE);
            break;
        case 0x70: //BVS
            cycles += BRANCH(&state->pc, opcode, state->ps.v == 1);
            break;
        case 0x71: //ADC - Zero Page Indirect Y-Indexed
            cycles += ADC(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x75: //ADC - X-Indexed Zero Page
            ADC(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x76: //ROR - X-Indexed Zero Page
            ROR(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x78: //SEI
            state->ps.i = 1;
            break;
        case 0x79: //ADC - Y-Indexed Absolute
            cycles += ADC(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x7d: //ADC - X-Indexed Absolute
            cycles += ADC(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x7e: //ROR - X-Indexed Absolute
            ROR(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0x81: //STA - X-Indexed Zero Page Indirect
            STA(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x84: //STY - Zero Page
            STY(state, opcode, ZERO_PAGE);
            break;
        case 0x85: //STA - Zero Page
            STA(state, opcode, ZERO_PAGE);
            break;
        case 0x86: //STX - Zero Page
            STX(state, opcode, ZERO_PAGE);
            break;
        case 0x88: //DEY - Implied
            DEC_INDEX(state, &state->y);
            break;
        case 0x8a: //TXA - Implied
            TRANSFER(state, state->x, &state->a);
            break;
        case 0x8c: //STY - Absolute
            STY(state, opcode, ABSOLUTE);
            break;
        case 0x8d: //STA - Absolute
            STA(state, opcode, ABSOLUTE);
            break;
        case 0x8e: //STX - Absolute
            STX(state, opcode, ABSOLUTE);
            break;
        case 0x90: //BCC - Relative
            cycles += BRANCH(&state->pc, opcode, state->ps.c == 0);
            break;
        case 0x91: //STA - Zero Page Indirect Y-Indexed
            STA(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x94: //STY - X-Indexed Zero Page
            STY(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x95: //STA - X-Indexed Zero Page
            STA(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0x96: //STX - Y-Indexed Zero Page
            STY(state, opcode, Y_INDEXED_ZERO_PAGE);
            break;
        case 0x98: //TYA - Implied
            TRANSFER(state, state->y, &state->a);
            break;
        case 0x99: //STA - Y-Indexed Absolute
            STA(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0x9a: //TXS - Implied
            TRANSFER(state, state->x, &state->s, false);
            break;
        case 0x9d: //STA - X-Indexed Absolute
            STA(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xa0: //LDY - Immediate
            LDY(state, opcode, IMMEDIATE);
            break;
        case 0xa1: //LDA - X-Indexed Zero Page Indirect
            LDA(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xa2: //LDX - Immediate
            LDX(state, opcode, IMMEDIATE);
            break;
        case 0xa4: //LDY - Zero Page
            LDY(state, opcode, ZERO_PAGE);
            break;
        case 0xa5: //LDA - Zero Page
            LDA(state, opcode, ZERO_PAGE);
            break;
        case 0xa6: //LDX - Zero Page
            LDX(state, opcode, ZERO_PAGE);
            break;
        case 0xa8: //TAY
            TRANSFER(state, state->a, &state->y);
            break;
        case 0xa9: //LDA - Immediate
            LDA(state, opcode, IMMEDIATE);
            break;
        case 0xaa: //TAX
            TRANSFER(state, state->a, &state->x);
            break;
        case 0xac: //LDY - Absolute
            LDY(state, opcode, ABSOLUTE);
            break;
        case 0xad: //LDA - Absolute
            LDA(state, opcode, ABSOLUTE);
            break;
        case 0xae: //LDX - Absolute
            LDX(state, opcode, ABSOLUTE);
            break;
        case 0xb0: //BCS - Relative
            cycles += BRANCH(&state->pc, opcode, state->ps.c == 1);
            break;
        case 0xb1: //LDA - Zero Page Indirect Y-Indexed
            cycles += LDA(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xb4: //LDY - X-Indexed Zero Page
            LDY(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xb5: //LDA - X-Indexed Zero Page
            LDA(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xb6: //LDX - Y-Indexed Zero Page
            LDX(state, opcode, Y_INDEXED_ZERO_PAGE);
            break;
        case 0xb8: //CLV
            state->ps.v = 0;
            break;
        case 0xb9: //LDA - Y-Indexed Absolute
            cycles += LDA(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xba: //TSX
            TRANSFER(state, state->s, &state->x);
            break;
        case 0xbc: //LDY - X-Indexed Absolute
            cycles += LDY(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xbd: //LDA - X-Indexed Absolute
            cycles += LDA(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xbe: //LDX - Y-Indexed Absolute
            cycles += LDX(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xc0: //CPY - Immediate
            CMP_INDEX(state, opcode, IMMEDIATE, state->y);
            break;
        case 0xc1: //CMP - X-Indexed Zero Page Indirect
            CMP_INDEX(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT, state->a);
            break;
        case 0xc4: //CPY - Zero Page
            CMP_INDEX(state, opcode, ZERO_PAGE, state->y);
            break;
        case 0xc5: //CMP - Zero Page
            CMP_INDEX(state, opcode, ZERO_PAGE, state->a);
            break;
        case 0xc6: //DEC - Zero Page
            DEC(state, opcode, ZERO_PAGE);
            break;
        case 0xc8: //INY - Implied
            INC_INDEX(state, &state->y);
            break;
        case 0xc9: //CMP - Immediate
            CMP_INDEX(state, opcode, IMMEDIATE, state->a);
            break;
        case 0xca: //DEX - Implied
            DEC_INDEX(state, &state->x);
            break;
        case 0xcc: //CPY - Absolute
            CMP_INDEX(state, opcode, ABSOLUTE, state->y);
            break;
        case 0xcd: //CMP - Absolute
            CMP_INDEX(state, opcode, ABSOLUTE, state->a);
            break;
        case 0xce: //DEC - Absolute
            DEC(state, opcode, ABSOLUTE);
            break;
        case 0xd0: //BNE
            cycles += BRANCH(&state->pc, opcode, state->ps.z == 0);
            break;
        case 0xd1: //CMP - Zero Page Indirect Y-Indexed
            cycles += CMP_INDEX(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED, state->a);
            break;
        case 0xd5: //CMP - X-Indexed Zero Page
            CMP_INDEX(state, opcode, X_INDEXED_ZERO_PAGE, state->a);
            break;
        case 0xd6: //DEC - X-Indexed Zero Page
            DEC(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xd8: //CLD
            state->ps.d = 0;
            break;
        case 0xd9: //CMP - Y-Indexed Absolute
            cycles += CMP_INDEX(state, opcode, Y_INDEXED_ABSOLUTE, state->a);
            break;
        case 0xdd: //CMP - X-Indexed Absolute
            cycles += CMP_INDEX(state, opcode, X_INDEXED_ABSOLUTE, state->a);
            break;
        case 0xde: //DEC - X-Indexed Absolute
            DEC(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xe0: //CPX - Immediate
            CMP_INDEX(state, opcode, IMMEDIATE, state->x);
            break;
        case 0xe1: //SBC - X-Indexed Zero Page Indirect
            SBC(state, opcode, X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xe4: //CPX- Zero Page
            CMP_INDEX(state, opcode, ZERO_PAGE, state->x);
            break;
        case 0xe5: //SBC - Zero Page
            SBC(state, opcode, ZERO_PAGE);
            break;
        case 0xe6: //INC - Zero Page
            INC(state, opcode, ZERO_PAGE);
            break;
        case 0xe8: //INX
            INC_INDEX(state, &state->x);
            break;
        case 0xe9: //SBC - Immediate
            SBC(state, opcode, IMMEDIATE);
            break;
        case 0xea: //NOP
            break;
        case 0xec: //CPX - Absolute
            CMP_INDEX(state, opcode, ABSOLUTE, state->x);
            break;
        case 0xed: //SBC - Absolute
            SBC(state, opcode, ABSOLUTE);
            break;
        case 0xee: //INC - Absolute
            INC(state, opcode, ABSOLUTE);
            break;
        case 0xf0: //BEQ
            cycles += BRANCH(&state->pc, opcode, state->ps.z == 1);
            break;
        case 0xf1: //SBC - Zero Page Indirect Y-Indexed
            cycles += SBC(state, opcode, ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xf5: //SBC - X-Indexed Zero Page
            SBC(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xf6: //INC - X-Indexed Zero Page
            INC(state, opcode, X_INDEXED_ZERO_PAGE);
            break;
        case 0xf8: //SED
            state->ps.d = 1;
            break;
        case 0xf9: //SBC - Y-Indexed Absolute
            cycles += SBC(state, opcode, Y_INDEXED_ABSOLUTE);
            break;
        case 0xfd: //SBC - X-Indexed Absolute
            cycles += SBC(state, opcode, X_INDEXED_ABSOLUTE);
            break;
        case 0xfe: //INC - X-Indexed Absolute
            INC(state, opcode, X_INDEXED_ABSOLUTE);
            break;
    }
    
    std::cout << "A: " << static_cast<int>(state->a) << " X: " << static_cast<int>(state->x) << " Y: " << static_cast<int>(state->y) << " S: " << static_cast<int>(state->s) << std::endl;
    std::cout << "C: " << static_cast<int>(state->ps.c) << " Z: " << static_cast<int>(state->ps.z) << " I: " << static_cast<int>(state->ps.i) << " D: " << static_cast<int>(state->ps.d) << " B: " << static_cast<int>(state->ps.b) << " V:" << static_cast<int>(state->ps.v) << " N: " << static_cast<int>(state->ps.n) << std::endl;
    
    
    return cycles;
}

void disassemble(State6502 *state)
{
    uint8_t *opcode = &state->memory[state->pc];
    state->pc += 1;
    
    switch (*opcode)
    {
        case 0x00: //BRK
            printf("%04x\tBRK\n", state->pc);
            break;
        case 0x01: //ORA - X-Indexed Zero Page Indirect
            printf("%04x\tORA ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x05: //ORA - Zero Page
            printf("%04x\tORA $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x06: //ASL - Zero Page
            printf("%04x\tASL $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x08: //PHP
            printf("%04x\tPHP\n", state->pc);
            break;
        case 0x09: //ORA - Immediate
            printf("%04x\tORA #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0x0a: //ASL - Accumulator
            printf("%04x\tASL A\n", state->pc);
            break;
        case 0x0d: //ORA - Absolute
            printf("%04x\tORA $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x0e: //ASL - Absolute
            printf("%04x\tASL $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x10: //BPL
            printf("%04x\tBPL $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0x11: //ORA - Zero Page Indirect Y-Indexed
            printf("%04x\tORA ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x15: //ORA - X-Indexed Zero Page
            printf("%04x\tORA $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x16: //ASL - X-Indexed Zero Page
            printf("%04x\tASL $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x18: //CLC
            printf("%04x\tCLC\n", state->pc);
            break;
        case 0x19: //ORA - Y-Indexed Absolute
            printf("%04x\tORA $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x1d: //ORA - X-Indexed Absolute
            printf("%04x\tORA $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x1e: //ASL - X-Indexed Absolute
            printf("%04x\tASL $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x20: //JSR - Absolute
            printf("%04x\tJSR $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x21: //AND - X-Indexed Zero Page Indirect
            printf("%04x\tAND ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x24: //BIT- Zero Page
            printf("%04x\tBIT $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x25: //AND - Zero Page
            printf("%04x\tAND $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x26: //ROL - Zero Page
            printf("%04x\tROL $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x28: //PLP
            printf("%04x\tPLP\n", state->pc);
            break;
        case 0x29: //AND - Immediate
            printf("%04x\tAND #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0x2a: //ROL - Accumulator
            printf("%04x\tROL A\n", state->pc);
            break;
        case 0x2c: //BIT - Absolute
            printf("%04x\tBIT $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x2d: //AND - Absolute
            printf("%04x\tAND $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x2e: //ROL - Absolute
            printf("%04x\tROL $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x30: //BMI
            printf("%04x\tBMI $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0x31: //AND - Zero Page Indirect Y-Indexed
            printf("%04x\tAND ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x35: //AND - X-Indexed Zero Page
            printf("%04x\tAND $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x36: //ROL - X-Indexed Zero Page
            printf("%04x\tROL $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x38: //SEC
            printf("%04x\tSEC\n", state->pc);
            break;
        case 0x39: //AND - Y-Indexed Absolute
            printf("%04x\tAND $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x3d: //AND - X-Indexed Absolute
            printf("%04x\tAND $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x3e: //ROL - X-Indexed Absolute
            printf("%04x\tROL $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x40: //RTI
            printf("%04x\tRTI\n", state->pc);
            break;
        case 0x41: //EOR - X-Indexed Zero Page Indirect
            printf("%04x\tEOR ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x45: //EOR - Zero Page
            printf("%04x\tEOR $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x46: //LSR - Zero Page
            printf("%04x\tLSR $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x48: //PHA
            printf("%04x\tPHA\n", state->pc);
            break;
        case 0x49: //EOR - Immediate
            printf("%04x\tEOR #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0x4a: //LSR - Accumulator
            printf("%04x\tLSR A\n", state->pc);
            break;
        case 0x4c: //JMP - Absolute
            printf("%04x\tJMP $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x4d: //EOR - Absolute
            printf("%04x\tEOR $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x4e: //LSR - Absolute
            printf("%04x\tLSR $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x50: //BVC
            printf("%04x\tBVC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0x51: //EOR - Zero Page Indirect Y-Indexed
            printf("%04x\tEOR ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x55: //EOR - X-Indexed Zero Page
            printf("%04x\tEOR $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x56: //LSR - X-Indexed Zero Page
            printf("%04x\tLSR $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x58: //CLI
            printf("%04x\tCLI\n", state->pc);
            break;
        case 0x59: //EOR - Y-Indexed Absolute
            printf("%04x\tEOR $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x5d: //EOR - X-Indexed Absolute
            printf("%04x\tEOR $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x5e: //LSR - X-Indexed Absolute
            printf("%04x\tLSR $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x60: //RTS
            printf("%04x\tRTS\n", state->pc);
            break;
        case 0x61: //ADC - X-Indexed Zero Page Indirect
            printf("%04x\tADC ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x65: //ADC - Zero Page
            printf("%04x\tADC $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x66: //ROR - Zero Page
            printf("%04x\tROR $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x68: //PLA
            printf("%04x\tPLA\n", state->pc);
            break;
        case 0x69: //ADC - Immediate
            printf("%04x\tADC #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0x6a: //ROR - Accumulator
            printf("%04x\tROR A\n", state->pc);
            break;
        case 0x6c: //JMP - Absolute Indirect
            printf("%04x\tJMP ($%02x%02x)\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE_INDIRECT);
            break;
        case 0x6d: //ADC - Absolute
            printf("%04x\tADC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x6e: //ROR - Absolute
            printf("%04x\tROR $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x70: //BVS
            printf("%04x\tBVS $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0x71: //ADC - Zero Page Indirect Y-Indexed
            printf("%04x\tADC ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x75: //ADC - X-Indexed Zero Page
            printf("%04x\tADC $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x76: //ROR - X-Indexed Zero Page
            printf("%04x\tROR $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x78: //SEI
            printf("%04x\tSEI\n", state->pc);
            break;
        case 0x79: //ADC - Y-Indexed Absolute
            printf("%04x\tADC $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x7d: //ADC - X-Indexed Absolute
            printf("%04x\tADC $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x7e: //ROR - X-Indexed Absolute
            printf("%04x\tROR $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0x81: //STA - X-Indexed Zero Page Indirect
            printf("%04x\tSTA ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0x84: //STY - Zero Page
            printf("%04x\tSTY $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x85: //STA - Zero Page
            printf("%04x\tSTA $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x86: //STX - Zero Page
            printf("%04x\tSTX $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0x88: //DEY - Implied
            printf("%04x\tDEY\n", state->pc);
            break;
        case 0x8a: //TXA - Implied
            printf("%04x\tTXA\n", state->pc);
            break;
        case 0x8c: //STY - Absolute
            printf("%04x\tSTY $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x8d: //STA - Absolute
            printf("%04x\tSTA $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x8e: //STX - Absolute
            printf("%04x\tSTX $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0x90: //BCC - Relative
            printf("%04x\tBCC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0x91: //STA - Zero Page Indirect Y-Indexed
            printf("%04x\tSTA ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0x94: //STY - X-Indexed Zero Page
            printf("%04x\tSTY $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x95: //STA - X-Indexed Zero Page
            printf("%04x\tSTA $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0x96: //STX - Y-Indexed Zero Page
            printf("%04x\tSTX $%02x,Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ZERO_PAGE);
            break;
        case 0x98: //TYA - Implied
            printf("%04x\tTYA\n", state->pc);
            break;
        case 0x99: //STA - Y-Indexed Absolute
            printf("%04x\tSTA $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0x9a: //TXS - Implied
            printf("%04x\tTXS\n", state->pc);
            break;
        case 0x9d: //STA - X-Indexed Absolute
            printf("%04x\tSTA $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xa0: //LDY - Immediate
            printf("%04x\tLDY #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xa1: //LDA - X-Indexed Zero Page Indirect
            printf("%04x\tLDA ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xa2: //LDX - Immediate
            printf("%04x\tLDX #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xa4: //LDY - Zero Page
            printf("%04x\tLDY $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xa5: //LDA - Zero Page
            printf("%04x\tLDA $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xa6: //LDX - Zero Page
            printf("%04x\tLDX $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xa8: //TAY
            printf("%04x\tTAY\n", state->pc);
            break;
        case 0xa9: //LDA - Immediate
            printf("%04x\tLDA #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xaa: //TAX
            printf("%04x\tTAX\n", state->pc);
            break;
        case 0xac: //LDY - Absolute
            printf("%04x\tLDY $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xad: //LDA - Absolute
            printf("%04x\tLDA $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xae: //LDX - Absolute
            printf("%04x\tLDX $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xb0: //BCS - Relative
            printf("%04x\tBCS $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0xb1: //LDA - Zero Page Indirect Y-Indexed
            printf("%04x\tLDA ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xb4: //LDY - X-Indexed Zero Page
            printf("%04x\tLDY $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xb5: //LDA - X-Indexed Zero Page
            printf("%04x\tLDA $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xb6: //LDX - Y-Indexed Zero Page
            printf("%04x\tLDX $%02x,Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ZERO_PAGE);
            break;
        case 0xb8: //CLV
            printf("%04x\tCLV\n", state->pc);
            break;
        case 0xb9: //LDA - Y-Indexed Absolute
            printf("%04x\tLDA $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xba: //TSX
            printf("%04x\tTSX\n", state->pc);
            break;
        case 0xbc: //LDY - X-Indexed Absolute
            printf("%04x\tLDY $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xbd: //LDA - X-Indexed Absolute
            printf("%04x\tLDA $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xbe: //LDX - Y-Indexed Absolute
            printf("%04x\tLDX $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xc0: //CPY - Immediate
            printf("%04x\tCPY #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xc1: //CMP - X-Indexed Zero Page Indirect
            printf("%04x\tCMP ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xc4: //CPY - Zero Page
            printf("%04x\tCPY $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xc5: //CMP - Zero Page
            printf("%04x\tCMP $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xc6: //DEC - Zero Page
            printf("%04x\tDEC $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xc8: //INY - Implied
            printf("%04x\tINY\n", state->pc);
            break;
        case 0xc9: //CMP - Immediate
            printf("%04x\tCMP #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xca: //DEX - Implied
            printf("%04x\tDEX\n", state->pc);
            break;
        case 0xcc: //CPY - Absolute
            printf("%04x\tCPY $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xcd: //CMP - Absolute
            printf("%04x\tCMP $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xce: //DEC - Absolute
            printf("%04x\tDEC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xd0: //BNE - Relative
            printf("%04x\tBNE $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0xd1: //CMP - Zero Page Indirect Y-Indexed
            printf("%04x\tCMP ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xd5: //CMP - X-Indexed Zero Page
            printf("%04x\tCMP $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xd6: //DEC - X-Indexed Zero Page
            printf("%04x\tDEC $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xd8: //CLD
            printf("%04x\tCLD\n", state->pc);
            break;
        case 0xd9: //CMP - Y-Indexed Absolute
            printf("%04x\tCMP $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xdd: //CMP - X-Indexed Absolute
            printf("%04x\tCMP $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xde: //DEC - X-Indexed Absolute
            printf("%04x\tDEC $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xe0: //CPX - Immediate
            printf("%04x\tCPX #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xe1: //SBC - X-Indexed Zero Page Indirect
            printf("%04x\tSBC ($%02x,X)\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE_INDIRECT);
            break;
        case 0xe4: //CPX - Zero Page
            printf("%04x\tCPX $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xe5: //SBC - Zero Page
            printf("%04x\tSBC $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xe6: //INC - Zero Page
            printf("%04x\tINC $%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE);
            break;
        case 0xe8: //INX
            printf("%04x\tINX\n", state->pc);
            break;
        case 0xe9: //SBC - Immediate
            printf("%04x\tSBC #%02x\n", state->pc, opcode[1]);
            state->pc += pcByMode(IMMEDIATE);
            break;
        case 0xea: //NOP
            printf("%04x\tNOP\n", state->pc);
            break;
        case 0xec: //CPX - Absolute
            printf("%04x\tCPX $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xed: //SBC - Absolute
            printf("%04x\tSBC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xee: //INC - Absolute
            printf("%04x\tINC $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(ABSOLUTE);
            break;
        case 0xf0: //BEQ
            printf("%04x\tBEQ $%02x%02x\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(RELATIVE);
            break;
        case 0xf1: //SBC - Zero Page Indirect Y-Indexed
            printf("%04x\tSBC ($%02x),Y\n", state->pc, opcode[1]);
            state->pc += pcByMode(ZERO_PAGE_INDIRECT_Y_INDEXED);
            break;
        case 0xf5: //SBC - X-Indexed Zero Page
            printf("%04x\tSBC $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xf6: //INC - X-Indexed Zero Page
            printf("%04x\tINC $%02x,X\n", state->pc, opcode[1]);
            state->pc += pcByMode(X_INDEXED_ZERO_PAGE);
            break;
        case 0xf8: //SED
            printf("%04x\tSED\n", state->pc);
            break;
        case 0xf9: //SBC - Y-Indexed Absolute
            printf("%04x\tSBC $%02x%02x,Y\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(Y_INDEXED_ABSOLUTE);
            break;
        case 0xfd: //SBC - X-Indexed Absolute
            printf("%04x\tSBC $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
        case 0xfe: //INC - X-Indexed Absolute
            printf("%04x\tINC $%02x%02x,X\n", state->pc, opcode[2], opcode[1]);
            state->pc += pcByMode(X_INDEXED_ABSOLUTE);
            break;
    }
}



