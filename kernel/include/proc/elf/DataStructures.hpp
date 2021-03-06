//
// Created by Martin Miralles-Cordal on 5/18/2018.
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace elf {

enum class Bitness : uint8_t { kNone = 0, k32Bit = 1, k64Bit = 2 };
enum class Endianness : uint8_t { LittleEndian = 1, BigEndian = 2 };
enum class Type : uint16_t { Relocatable = 1, Executable = 2, Shared = 3, Core = 4 };

enum class InstructionSet : uint16_t
{
    Nonspecific = 0,
    Sparc = 2,
    x86 = 3,
    MIPS = 8,
    PowerPC = 0x14,
    ARM = 0x28,
    SuperH = 0x2A,
    IA_64 = 0x32,
    x86_64 = 0x3E,
    AArch64 = 0xB7
};

struct [[gnu::packed]] ElfHeader32
{
    static constexpr uint8_t kMagicNumber = 0x7F;
    static constexpr uint32_t kCurrentVersion = 1;

    bool checkMagic() const
    {
        bool const magicMatches = magic == ElfHeader32::kMagicNumber;
        bool const elfMatches = elf[0] == 'E' && elf[1] == 'L' && elf[2] == 'F';
        return magicMatches && elfMatches;
    }

    uint8_t magic;
    char elf[3]; // == "ELF"
    Bitness bitness;
    Endianness endianness;
    uint8_t version8;
    uint8_t abi;
    uint64_t unused;
    Type type;
    InstructionSet instructionSet;
    uint32_t version32;
    uint32_t entry;
    uint32_t programHeaderOffset;
    uint32_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t size;
    uint16_t programEntrySize;
    uint16_t programEntryCount;
    uint16_t sectionEntrySize;
    uint16_t sectionEntryCount;
    uint16_t sectionNameEntryIndex;
};

struct [[gnu::packed]] ProgramHeader32
{
    enum class Type : uint32_t
    {
        Null = 0,
        Load = 1,
        Dynamic = 2,
        Interpreter = 3,
        Note = 4,
        Shlib = 5,
        ProgramHeader = 6,
        LoProc = 0x70000000,
        HiProc = 0x7fffffff
    };

    Type type;
    uint32_t offset;
    uint32_t virtAddress;
    uint32_t physAddress;
    uint32_t sizeInFile;
    uint32_t sizeInMemory;
    uint32_t flags;
    uint32_t alignment;
};

struct [[gnu::packed]] SectionHeader32
{
    enum Flags
    {
        Write = 0x01,
        Alloc = 0x02,
        Exec = 0x04,
        MaskProc = 0xF0000000
    };

    enum class Type : uint32_t
    {
        Null = 0,
        ProgBits = 1,
        SymbolTable = 2,
        StringTable = 3,
        RelocAddend = 4,
        HashTable = 5,
        Dynamic = 6,
        Note = 7,
        NoBits = 8,
        Relocation = 9,
        Shlib = 10,
        Dynsym = 11,
        LoProc = 0x70000000,
        HiProc = 0x7fffffff,
        LoUser = 0x80000000,
        HiUser = 0xffffffff
    };

    enum class SpecialIndex
    {
        Undefined = 0,
        LoReserve = 0xff00,
        LoProc = 0xff00,
        HiProc = 0xff1f,
        Absolute = 0xfff1,
        Common = 0xfff2,
        HiReserve = 0xffff
    };

    uint32_t nameIndex;
    Type type;
    uint32_t flags;
    uint32_t address;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t alignment;
    uint32_t entrySize;
};

struct [[gnu::packed]] SymbolTableEntry
{
    // Binding and Type are not enum classes because we can't bit pack those
    struct Binding
    {
        static constexpr uint8_t Local = 0;
        static constexpr uint8_t Global = 1;
        static constexpr uint8_t Weak = 2;
        static constexpr uint8_t LoProc = 13;
        static constexpr uint8_t HiProc = 15;
    };

    struct Type
    {
        static constexpr uint8_t NoType = 0;
        static constexpr uint8_t Object = 1;
        static constexpr uint8_t Func = 2;
        static constexpr uint8_t Section = 3;
        static constexpr uint8_t File = 4;
        static constexpr uint8_t LoProc = 13;
        static constexpr uint8_t HiProc = 15;
    };

    uint32_t nameIndex;
    uint32_t value;
    uint32_t size;
    uint8_t binding:4; // Binding
    uint8_t type:4; // Type
    uint8_t other;
    uint16_t sectionHeaderIndex; // may be a SpecialIndex
};

} // namespace elf
