//
//  6502emu.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 10/16/24.
//

#ifndef _502emu_hpp
#define _502emu_hpp

#include <stdint.h>
#include <string>

enum class InterruptType { BRK, IRQ, RESET, NMI };

/*
 Physical 6502 CPU class
 
 Contains registers, program counters, and regular 6502 CPU functionality
 */
class cpu6502 {
    // Nested memory class specific to the CPU
    
    class Memory
    {
        std::unique_ptr<uint8_t[]> data;
        
    public:
        Memory();
        
        // Access operator
        uint8_t& operator[](size_t index);

        // Const access operator
        const uint8_t& operator[](size_t index) const;
        
        /*
         Memory mirroring mimicker:
         It mimics the act of memory mirroring by returning the mirrored value from the base position than actual position
         
         For example:
            CPU RAM memory is mirrored every 2KB (0x0000 - 0x07FF, 0x0800 - 0x0FFF , ... , 0x17FF - 0x1FFF)
            A read to address 0x1FFF will yield the value of 0x07FF
            A read to address 0x0800 will yield the value of 0x0000, etc.
         */
        uint16_t mirroredAddress(uint16_t address) const;
        
        /*
         Helper function that grabs the memory address at data[0]
         */
        uint8_t* getBaseAddress();
    };
    
public:
    uint8_t a;  //Accumulator
    uint8_t x;  //X index register
    uint8_t y;  //Y index register
    uint8_t s;  //Stack pointer
    
    Memory memory; // Memory
    
    union //Program counter
    {
        struct
        {
            unsigned lo:8;  //Low and
            unsigned hi:8;  //High byte for easy manipulation
        };
        uint16_t val;
    } pc;
    
    union // Processor Status codes
    {
        struct
        {
            unsigned c:1; //0 - carry bit        1 = carry
            unsigned z:1; //1 - zero bit         1 = zero
            unsigned i:1; //2 - Irq disable      1 = disable
            unsigned d:1; //3 - Decimal mode     1 = True
            unsigned b:1; //4 - Break command    1 = BRK
            unsigned _:1; //5 - (SKIPPED BIT)
            unsigned v:1; //6 - Overflow         1 = True
            unsigned n:1; //7 - Negative         1 = NEG
        };
        uint8_t val;
    } ps;
    
    /* ---------- CONSTRUCTORS AND DESTRUCTORS ----------*/
    
    cpu6502();
    
    /* ---------- FUNCTIONS ---------- */
    
    int emulate();
    void disassemble();
    int interrupt_handler(InterruptType type);
    
    /* ---------- ACCESS FUNCTIONS ---------- */
    
    void incStack();    // Handles increment of stack (Make sure it doesn't go outta bounds)
    void decStack();    // Handles decrement of stack (Make sure it doesn't go outta bounds)
    
    uint8_t parseProcessorStatus();
    void unparseProcessorStatus(const uint8_t status);
};

#endif /* _502emu_hpp */
