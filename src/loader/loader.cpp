//
//  loader.cpp
//  emulator_6502
//
//  Created by Kyle Chiem on 1/13/25.
//

#include "loader.hpp"

#include <memory>

Loader::Loader() : m_romLoaded(false) {}

// In 16 kiB units
int Loader::getPrgRomSize() const
{
    return getRomSize(m_header->prgRomSize, 16, (m_header->prgRomSize >> 8) == 0x0F) ? (m_romLoaded) : 0;
}

int Loader::getChrRomSize() const
{
    return getRomSize(m_header->chrRomSize, 16, (m_header->chrRomSize >> 8) == 0x0F) ? (m_romLoaded) : 0;
}

const bool Loader::isLoaded() const
{
    return m_romLoaded;
}

void Loader::loadRom(const char* filename)
{
    m_romLoaded = true;
    
    // Open the file in binary mode to read byte by byte.
    try
    {
        file.open(filename, std::ios::binary);
        
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file: " + std::string(filename));
        }
        
        loadHeader();
        loadRomData();
        
        if (!m_romLoaded)
        {
            throw std::runtime_error("Incorrect file.");
        }
        
        std::cout << "Loaded ROM successfully" << "\n";
        
        file.close();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Runtime exception occured: " << e.what() << "\n";
        
        if (!file.is_open())
        {
            file.close();
        }
        
        // Clear all the pointers to avoid any unwanted access
        clearRom();
    }
    catch (...)
    {
        std::cerr << "Error occured while loading rom." << "\n";
        
        // Clear all the pointers to avoid any unwanted access
        clearRom();
    }
}

void Loader::clearRom()
{
    m_header.reset();
    m_romData.reset();
    m_romLoaded = false;
}

/* ---------- PRIVATE FUNCTIONS ---------- */

int Loader::getRomSize(uint16_t romSize, uint8_t sizeMultiplier, bool exponent) const
{
    if (exponent)
    {
        uint8_t multiplier = (romSize & 0x03) * 2 + 1;
        uint8_t exponent = ((romSize >> 2) & 0x3F);
        
        return static_cast<int>(pow(2, exponent)) * multiplier;
    }
    else
    {
        return static_cast<int>(romSize) * sizeMultiplier;
    }
}

void Loader::loadHeader()
{
    m_header = std::make_unique<Header>();
    uint8_t* headerBytes = new uint8_t[16];
    
    for (int i = 0; i < 16; ++i)
    {
        headerBytes[i] = readByte();
    }
    
    // ------ Determine format of rom file
    if (headerBytes[0] == 'N' && headerBytes[1] == 'E' && headerBytes[2] == 'S' && headerBytes[3] == 0x1A)
        m_header->format.iNES = 1;
    if (m_header->format.iNES && (headerBytes[7] & 0x0C) == 0x08)
        m_header->format.nes2_0 = 1;
    
    // Neither iNES or NES 2.0; invalid file
    if (m_header->format.val == 0)
    {
        m_romLoaded = false;
        return;
    }
    
    // ------ Set PRG and CHR Rom & Ram sizes
    m_header->prgRomSize = (static_cast<uint16_t>(headerBytes[9] & 0x0F) << 8) | headerBytes[4];
    m_header->chrRomSize = (static_cast<uint16_t>(headerBytes[9] >> 4) << 8) | headerBytes[5];
    m_header->prgRamSize.val = headerBytes[10];
    m_header->chrRamSize.val = headerBytes[11];
    
    // ------ Set the mapper & submapper number - Found in header bytes 6 - 8
    m_header->mapperNumber = (static_cast<uint16_t>(headerBytes[8] & 0x0F) << 8 ) | (headerBytes[6] >> 4 | (headerBytes[7] & 0xF0));
    
    m_header->submapperNumber = headerBytes[8] >> 4;
    
    // ------ Populate Header Struct with given params
    m_header->flags.val = (headerBytes[7] << 4) | (headerBytes[6] & 0x0F); // 7th byte has high bits, 6th byte has low bits
    
    // ------ Set Console type Header params
    if (m_header->flags.consoleType == 1) // NINTENDO VS. SYSTEM
        m_header->systemType.val = headerBytes[13];
    else if (m_header->flags.consoleType == 3) // EXTENDED CONSOLE TYPE
        m_header->extConsoleType = headerBytes[13];
    
    // ------ Set other MISC header flags
    m_header->timingMode = headerBytes[12];
    m_header->miscRoms = headerBytes[14];
    m_header->defaultExpansionDevice = headerBytes[15];
    
    delete[] headerBytes;
}

void Loader::loadRomData()
{
    m_romData = std::make_unique<RomData>();
    
    if (!m_romLoaded) // Invalid header file described by above function
        return;
    
    if (m_header->flags.T) // If there is a trainer
    {
        // Load the 512 bytes into the trainer array
        for (int i = 0; i < 512; ++i)
        {
            m_romData->trainer[i] = readByte();
        }
    }
    
    // Load the prgRomSize number of bytes into PRG ROM memory
    m_romData->prgRom.reserve(getPrgRomSize()); // Reserve the memory first to avoid constant reallocation
    
    for (int i = 0; i < getPrgRomSize(); ++i)
    {
        m_romData->prgRom.push_back(readByte());
    }
    
    // Load the x number of bytes into CHR ROM memory
    m_romData->chrRom.reserve(getChrRomSize()); // Reserve the memory first to avoid constant reallocation
    
    for (int i = 0; i < getChrRomSize(); ++i)
    {
        m_romData->chrRom.push_back(readByte());
    }
}

/* ---------- HELPER FUNCTION FOR LOADER ---------- */

uint8_t Loader::readByte()
{
    char byte;
    if (file.read(&byte, sizeof(byte))) 
    {
        return static_cast<uint8_t>(static_cast<unsigned char>(byte));
    } 
    else
    {
        throw std::runtime_error("Failed to read from the file or end of file reached.");
    }
}
