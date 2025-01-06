//
//  instructions.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/4/25.
//

#ifndef instructions_hpp
#define instructions_hpp

#include <stdio.h>
#include "6502emu.hpp"

namespace AddressingModeFuncs
{

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

    uint16_t AbsoluteOffset(const uint8_t *const opcode, const uint8_t index = 0);
    uint16_t ZPOffset(const uint8_t *const opcode, const uint8_t index = 0);
    uint16_t XIndexZPIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode);
    uint16_t YIndexZPIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode);
    uint16_t AbsoluteIndirectOffset(const cpu6502 *const cpu, const uint8_t *const opcode);
    uint8_t* offsetByMode(cpu6502 *const cpu, uint8_t *const opcode, const AddressingMode mode);
    uint8_t pcByMode(const AddressingMode mode);

}

namespace Instructions
{
    /* ---------- Logic Instructions ---------- */

    int ORA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void BIT(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int AND(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int EOR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);

    /* ---------- Shift (bit) Instructions ---------- */

    void ASL(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void LSR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void ROL(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void ROR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);

    /* ---------- Arithmetic Instructions ---------- */

    int ADC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int SBC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int CMP_INDEX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode, const uint8_t index);

    /* ---------- Store/Load Instructions ---------- */

    void STA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void STX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void STY(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int LDA(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int LDX(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    int LDY(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);

    /* ---------- Increment/Decrement Instructions ---------- */

    void DEC_INDEX(cpu6502 *const cpu, uint8_t *const index);
    void DEC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void INC_INDEX(cpu6502 *const cpu, uint8_t *const index);
    void INC(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);

    /* ---------- Branch Instructions ---------- */

    /*
     Condition branches (similar to if cpuments, but tests bits on processor status)
     Returns extra number of cycles if page crossed or branch is taken
     Covers BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS instructions
     */
    int BRANCH(uint16_t *const pc, uint8_t *const opcode, const bool test_set);

    /* ---------- Transfer Instructions ---------- */

    /*
     Transfer values from one variable to another.
     Covers transfer instructions TAX, TAY, TSX, TXA, TXS. TYA
     */
    void TRANSFER(cpu6502 *const cpu, const uint8_t value, uint8_t *const var, const bool affect_flags = true);

    /* ---------- Stack Instructions ---------- */

    void PHA(cpu6502 *const cpu);
    void PHP(cpu6502 *const cpu);
    void PLA(cpu6502 *const cpu);
    void PLP(cpu6502 *const cpu);

    /* ---------- Control Instructions ---------- */

    // BRK instruction handled in cpu6502::interrupt_handler()
    void RTI(cpu6502 *const cpu);
    void RTS(cpu6502 *const cpu);
    void JMP(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
    void JSR(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);
}

/* ---------- Helper Functions ---------- */
int detectPageCross(cpu6502 *const cpu, uint8_t *const opcode, const AddressingModeFuncs::AddressingMode mode);

/* ---------- Flag functions ---------- */
void bitwiseOpFlags(cpu6502 *const cpu, uint8_t comp);

#endif /* instructions_hpp */