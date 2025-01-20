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

// LIB includes
#include "../util/cpumem.hpp"

enum class InterruptType { BRK, IRQ, RESET, NMI };

/*
 Physical 6502 CPU class
 
 Contains registers, program counters, and regular 6502 CPU functionality
 */
class cpu6502 {
    
    /// Wrapper class to control how memory in the cpu is read/written
    class MemWrapper
    {
        Memory& memory;
        std::variant<uint8_t*, uint16_t> m_val;
        bool m_accessesMem;
        
    public:
        
        MemWrapper(Memory& mem, uint8_t* val, bool accessesMem);
        
        /// Point to a new value to wrap
        void pointTo(std::variant<uint8_t*, uint16_t>, bool accessesMem);
        
        /// If the piece of memory accesses memory
        const uint16_t getAddress() const;
        
        operator uint8_t() const;
        MemWrapper& operator=(uint8_t value);
    };
    
public:
    uint8_t a;  //Accumulator
    uint8_t x;  //X index register
    uint8_t y;  //Y index register
    uint8_t s;  //Stack pointer
    
    Memory& memory; // Memory
    MemWrapper wrapper; // Wrapper for controlling the reading/writing of memory
    
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
    
    cpu6502(Memory& mem);
    
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
