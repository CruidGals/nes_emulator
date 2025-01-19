//
//  instructions.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/4/25.
//

#include "instructions.hpp"

#include <stdint.h>

namespace AddressingModeFuncs
{
    //Addressing Mode Functions -------------------------------------------

    //Returns memory offset for Absolute Addressing Mode; index is used if X or Y indexed
    uint16_t AbsoluteOffset(const uint8_t *const opcode, const uint8_t index)
    {
        return ((static_cast<uint16_t>(opcode[2]) << 8) | static_cast<uint16_t>(opcode[1])) + index;
    }

    //Returns Zero-Page offset for Absolute Addressing Mode; index is used if X or Y indexed
    uint16_t ZPOffset(const uint8_t *const opcode, const uint8_t index)
    {
        return static_cast<uint16_t>(opcode[1] + index);
    }

    //Returns memory offset for X-Indexed Zero Page Indirect Addressing Mode
    uint16_t XIndexZPIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode)
    {
        return static_cast<uint16_t>(cpu->memory[static_cast<uint16_t>(opcode[1] + cpu->x + 1)]) << 8 | static_cast<uint16_t>(cpu->memory[static_cast<uint16_t>(opcode[1] + cpu->x)]);
    }

    //Returns memory offset for Zero Page Indirect Y-Indexed Addressing Mode
    uint16_t YIndexZPIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode)
    {
        return (static_cast<uint16_t>(cpu->memory[opcode[1] + 1]) << 8 | static_cast<uint16_t>(cpu->memory[opcode[1]])) + static_cast<uint16_t>(cpu->y);
    }

    //Specific function for Absolute Indirect Addressing Mode
    uint16_t AbsoluteIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode)
    {
        const uint8_t lowByte = cpu->memory[static_cast<uint16_t>(opcode[2]) << 8 | static_cast<uint16_t>(opcode[1])];
        
        //Preserves quirk that page is not crossed during Absolute Indirect Addressing (meaning high byte of address is unchanged)
        const uint8_t highByte = cpu->memory[static_cast<uint16_t>(opcode[2]) << 8 | static_cast<uint16_t>(opcode[1] + 1)];
        
        return static_cast<uint16_t>(highByte) << 8 | static_cast<uint16_t>(lowByte);
    }

    /*
     *  Important change:
     *
     *  Previous ex code:   case ABSOLUTE: return &cpu->memory[AbsoluteOffset(opcode)]
     *  New code:           case ABSOLUTE: cpu->memory.getBaseAddress() + AbsoluteOffset(opcode)
     */
    uint8_t& offsetByMode(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        switch (mode) {
            case ACCUMULATOR: return cpu->a;
            case IMMEDIATE: return opcode[1];
            case ABSOLUTE: return *cpu->memory.getAbsoluteAddress(AbsoluteOffset(opcode));
            case ABSOLUTE_INDIRECT: return *cpu->memory.getAbsoluteAddress(AbsoluteIndirectOffset(cpu, opcode));
            case X_INDEXED_ABSOLUTE: return *cpu->memory.getAbsoluteAddress(AbsoluteOffset(opcode, cpu->x));
            case Y_INDEXED_ABSOLUTE: return *cpu->memory.getAbsoluteAddress(AbsoluteOffset(opcode, cpu->y));
            case ZERO_PAGE: return *cpu->memory.getAbsoluteAddress(ZPOffset(opcode));
            case X_INDEXED_ZERO_PAGE: return *cpu->memory.getAbsoluteAddress(ZPOffset(opcode, cpu->x));
            case Y_INDEXED_ZERO_PAGE: return *cpu->memory.getAbsoluteAddress(ZPOffset(opcode, cpu->y));
            case X_INDEXED_ZERO_PAGE_INDIRECT: return *cpu->memory.getAbsoluteAddress(XIndexZPIndirectOffset(cpu, opcode));
            case ZERO_PAGE_INDIRECT_Y_INDEXED: return *cpu->memory.getAbsoluteAddress(YIndexZPIndirectOffset(cpu, opcode));
                
            case RELATIVE: return opcode[1];
            case IMPLIED: return opcode[1]; // Unused
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
}

/* ---------- Helper Functions ---------- */

int detectPageCross(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode)
{
    using namespace AddressingModeFuncs;
    
    uint8_t prev_high = opcode[2];
    uint8_t new_high;
    
    switch (mode)
    {
        case X_INDEXED_ABSOLUTE:
            //Get high bit of new address
            new_high = static_cast<uint8_t>(AbsoluteOffset(opcode, cpu->x) >> 8);
            break;
        case Y_INDEXED_ABSOLUTE:
            //Get high bit of new address
            new_high = static_cast<uint8_t>(AbsoluteOffset(opcode, cpu->y) >> 8);
            break;
        case ZERO_PAGE_INDIRECT_Y_INDEXED:
            prev_high = static_cast<uint8_t>((static_cast<uint16_t>(cpu->memory[opcode[1] + 1] << 8) | static_cast<uint16_t>(cpu->memory[opcode[1]])) >> 8);
            new_high = static_cast<uint8_t>(YIndexZPIndirectOffset(cpu, opcode) >> 8);
            break;
        default:
            return 0;
    }
    
    return prev_high != new_high;
}

//Flags -----------------------------------------

void bitwiseOpFlags(cpu6502 *const cpu, uint8_t comp)
{
    cpu->ps.z = comp == 0x00;
    cpu->ps.n = 0x80 == (comp & 0x80);
}

//Opcode Instructions ---------------------------
namespace Instructions
{
    using namespace AddressingModeFuncs; // So I don't have to write it out every single time 

    /* ---------------- LOGIC INSTRUCTIONS ---------------- */

    //Returns extra cycle if page crossing detected
    int ORA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->a = offsetByMode(cpu, opcode, mode) | cpu->a;
        bitwiseOpFlags(cpu, cpu->a);
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    void BIT(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t offset = offsetByMode(cpu, opcode, mode);
        uint8_t result = cpu->a & offset;
        
        cpu->ps.v = (0x40 == (offset & 0x40));
        cpu->ps.z = result == 0x00;
        cpu->ps.n = 0x80 == (offset & 0x80);
        cpu->pc.val += pcByMode(mode);
    }

    //Returns extra cycle if page crossing detected
    int AND(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->a = offsetByMode(cpu, opcode, mode) & cpu->a;
        bitwiseOpFlags(cpu, cpu->a);
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    //Returns extra cycle if page crossing detected
    int EOR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->a = offsetByMode(cpu, opcode, mode) ^ cpu->a;
        bitwiseOpFlags(cpu, cpu->a);
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    /* ---------------- SHIFT (BIT) INSTRUCTIONS ---------------- */

    void ASL(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t& p = offsetByMode(cpu, opcode, mode);
        
        //Set carry bit before lost
        cpu->ps.c = 0x80 == (p & 0x80);
        
        p = p << 1;
        
        //Flags
        cpu->ps.z = p == 0x00;
        cpu->ps.n = 0x80 == (p & 0x80);
        cpu->pc.val += pcByMode(mode);
    }

    void LSR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t& p = offsetByMode(cpu, opcode, mode);
        
        //Set carry bit before lost
        cpu->ps.c = 0x01 == (p & 0x01);
        
        p = p >> 1;
        
        //Rest of Flags
        cpu->ps.z = p == 0x00;
        cpu->ps.n = 0;
        cpu->pc.val += pcByMode(mode);
    }

    /*
     Shifts memory or accumulator left by one bit, with carry bit rotating into the 0th bit and the 7th bit rotating into the carry bit
     Affects carry, zero, negative bit
     */
    void ROL(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t& offset = offsetByMode(cpu, opcode, mode);
        uint8_t result = offset << 1 | cpu->ps.c;
        
        //Set carry bit before lost
        cpu->ps.c = 0x80 == (offset & 0x80);
        
        offset = result;
        
        cpu->ps.z = offset == 0x00;
        cpu->ps.n = 0x80 == (offset & 0x80);
        cpu->pc.val += pcByMode(mode);
    }

    /*
     Shifts memory or accumulator right by one bit, with carry bit rotating into the 7th bit and the 0th bit rotating into the carry bit
     Affects carry, zero, negative bit
     */
    void ROR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t& offset = offsetByMode(cpu, opcode, mode);
        uint8_t result = offset >> 1 | cpu->ps.c << 7;
        
        //Set carry bit before lost by bit maneuver
        cpu->ps.c = 0x01 == (offset & 0x01);
        
        offset = result;
        cpu->ps.z = offset == 0x00;
        cpu->ps.n = 0x80 == (offset & 0x80);
        cpu->pc.val += pcByMode(mode);
    }

    /* ---------------- ARITHMETIC INSTRUCTIONS ---------------- */

    //Returns extra cycle if page crossing detected
    int ADC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t offset = offsetByMode(cpu, opcode, mode);
        uint16_t result = static_cast<uint16_t>(cpu->a) + static_cast<uint16_t>(offset) + cpu->ps.c;
        
        //Set overflow flag before cpu->a is updated
        cpu->ps.v = (((cpu->a ^ offset) & (cpu->a ^ result) & 0x80) != 0); //Overflow if num both negative/positive (given by 7th bit)
        
        cpu->a = static_cast<uint8_t>(result);
        
        //Flags
        cpu->ps.c = 0x0100 == (result & 0x0100);
        cpu->ps.z = cpu->a == 0x00;
        cpu->ps.n = 0x80 == (cpu->a & 0x80);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    //Returns extra cycle if page crossing detected
    int SBC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t offset = offsetByMode(cpu, opcode, mode);
        uint16_t result = static_cast<uint16_t>(cpu->a) - static_cast<uint16_t>(offset) - (0x0001 - cpu->ps.c);
        
        //Set overflow flag before cpu->a is updated
        cpu->ps.v = (((cpu->a ^ result) & (cpu->a ^ offset) & 0x80) != 0); //Overflow if num both negative/positive (given by 7th bit)
        
        cpu->a = static_cast<uint8_t>(result);
        
        //Flags
        cpu->ps.c = result <= UINT8_MAX;
        cpu->ps.z = cpu->a == 0x00;
        cpu->ps.n = 0x80 == (cpu->a & 0x80);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    /*
     Covers CMP, CPX, and CPY
     Returns extra cycle if page crossing detected
     */
    int CMP_INDEX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode, const uint8_t index)
    {
        uint8_t offset = offsetByMode(cpu, opcode, mode);
        uint8_t result = index - offset;
        
        //Flags
        cpu->ps.c = index >= offset;
        bitwiseOpFlags(cpu, result);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    /* ---------------- LOAD INSTRUCTIONS ---------------- */

    void STA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        offsetByMode(cpu, opcode, mode) = cpu->a;
        cpu->pc.val += pcByMode(mode);
    }

    void STX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        offsetByMode(cpu, opcode, mode) = cpu->x;
        cpu->pc.val += pcByMode(mode);
    }

    void STY(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        offsetByMode(cpu, opcode, mode) = cpu->y;
        cpu->pc.val += pcByMode(mode);
    }

    //Returns extra cycle if page crossing detected
    int LDA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->a = offsetByMode(cpu, opcode, mode);
        bitwiseOpFlags(cpu, cpu->a);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    //Returns extra cycle if page crossing detected
    int LDX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->x = offsetByMode(cpu, opcode, mode);
        bitwiseOpFlags(cpu, cpu->x);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    //Returns extra cycle if page crossing detected
    int LDY(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        cpu->y = offsetByMode(cpu, opcode, mode);
        bitwiseOpFlags(cpu, cpu->y);
        
        cpu->pc.val += pcByMode(mode);
        
        return detectPageCross(cpu, opcode, mode);
    }

    /* ---------------- TRANSFER INSTRUCTIONS ---------------- */

    /*
     Transfer values from one variable to another.
     Covers transfer instructions TAX, TAY, TSX, TXA, TXS. TYA
     */
    void TRANSFER(cpu6502 *const cpu, const uint8_t value, uint8_t *const var, const bool affect_flags)
    {
        //Transfer value to variable
        *var = value;
        
        //Only TXS doesn't affect flags
        if (affect_flags) bitwiseOpFlags(cpu, *var);
    }

    /* ---------------- INCREMENT/DECREMENT INSTRUCTIONS ---------------- */

    /*
     Covers decrements instructions DEY and DEX
     */
    void DEC_INDEX(cpu6502 *const cpu, uint8_t& index)
    {
        index--;
        bitwiseOpFlags(cpu, index);
    }

    void DEC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t& offset = offsetByMode(cpu, opcode, mode);
        offset--;
        bitwiseOpFlags(cpu, offset);
        cpu->pc.val += pcByMode(mode);
    }

    /*
     Covers decrements instructions INY and INX
     */
    void INC_INDEX(cpu6502 *const cpu, uint8_t& index)
    {
        index++;
        bitwiseOpFlags(cpu, index);
    }

    void INC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        uint8_t offset = offsetByMode(cpu, opcode, mode);
        offset++;
        bitwiseOpFlags(cpu, offset);
        cpu->pc.val += pcByMode(mode);
    }

    /* ---------------- BRANCH INSTRUCTIONS ---------------- */

    /*
     Condition branches (similar to if cpuments, but tests bits on processor status)
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

    void PHA(cpu6502 *const cpu)
    {
        cpu->memory[0x100 | cpu->s] = cpu->a;
        cpu->decStack();
    }

    void PHP(cpu6502 *const cpu)
    {
        //Parse Processor Status into uin8_t
        uint8_t status = cpu->parseProcessorStatus();
        cpu->memory[0x100 | cpu->s] = status;
        cpu->decStack();
    }

    void PLA(cpu6502 *const cpu)
    {
        cpu->incStack();
        cpu->a = cpu->memory[0x100 | cpu->s];
        bitwiseOpFlags(cpu, cpu->a);
    }

    void PLP(cpu6502 *const cpu)
    {
        cpu->incStack();
        uint8_t status = cpu->memory[0x100 | cpu->s];
        
        //Unparse Processor Status
        cpu->unparseProcessorStatus(status);
        
        //Break flag not restored though (lowk ignored)
        cpu->ps.b = 0;
    }

    /* ---------------- CONTROL INSTRUCTIONS ---------------- */

    // BRK Function is handled in cpu6502 in its interrupt handler

    void RTI(cpu6502 *const cpu)
    {
        // Get processor status
        cpu->incStack();
        cpu->unparseProcessorStatus(cpu->memory[0x100 | (cpu->s)]);
        
        //Load program counter (subtract by one to get exact memory address next time through)
        cpu->incStack();
        cpu->pc.lo = cpu->memory[0x100 | (cpu->s)];
        cpu->incStack();
        cpu->pc.hi = cpu->memory[0x100 | (cpu->s)];
        
        cpu->pc.val--;
    }

    void RTS(cpu6502 *const cpu)
    {
        //Load program counter (add by one)
        cpu->incStack();
        cpu->pc.lo = cpu->memory[0x100 | (cpu->s)];
        cpu->incStack();
        cpu->pc.hi = cpu->memory[0x100 | (cpu->s)];
    }

    void JMP(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        //In order to return correct address, subtract by cpu->memory to eliminate arbitrary memory placement
        cpu->pc.val = static_cast<uint16_t>(&offsetByMode(cpu, opcode, mode) - cpu->memory.getBaseAddress());
    }

    void JSR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode)
    {
        //PC incremented in emulate(), need to store (pc + 2) into stack
        cpu->pc.val++;
        
        //Gets correct address by subtracting where cpu->memory begins
        uint16_t offset = (&offsetByMode(cpu, opcode, mode) - cpu->memory.getBaseAddress());
        
        //Load program counter onto stack
        cpu->memory[cpu->s] = static_cast<uint8_t>(cpu->pc.hi);
        cpu->decStack();
        cpu->memory[cpu->s] = static_cast<uint8_t>(cpu->pc.lo);
        cpu->decStack();
        
        //Redirect program counter
        cpu->pc.val = offset;
    }

    /* ---------------- FLAG INSTRUCTIONS ---------------- */
    /* Already written within switch cpument (one liner) */

}
