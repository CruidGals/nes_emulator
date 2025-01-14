//
//  ppumem.hpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/9/25.
//

#pragma once

#include "abstract/memory.h"

enum class NametableMirroring { NONE, SINGLE, HORIZONTAL, VERTICAL };

class PPUMemory : public Memory
{

public:
    PPUMemory(NametableMirroring type);

    uint16_t mirroredAddress(uint16_t address) const override;
    
    NametableMirroring mirroringType;
};
