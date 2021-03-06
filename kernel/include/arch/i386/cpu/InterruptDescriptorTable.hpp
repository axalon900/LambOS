//
// Created by Martin Miralles-Cordal on 8/12/2013.
//

#pragma once

#include <arch/i386/cpu/RegisterTable.h>
#include <arch/i386/cpu/InterruptNumber.hpp>
#include <system/asm.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

extern "C" void set_idt(void *idt, size_t size);

struct InterruptServiceRoutine
{
    static inline void endOfInterrupt() { outb(0x20, 0x20); }
    virtual ~InterruptServiceRoutine() {}
    virtual void operator()(RegisterTable &) = 0;
};

class IDTEntry
{
  public:
    IDTEntry() : _offset(0), _selector(0), _typeAttr(0) {}
    IDTEntry(uint32_t offset, uint8_t type_attr) : _offset(offset), _selector(0x08), _typeAttr(type_attr) {}
    IDTEntry(uint16_t selector, uint32_t offset, uint8_t type_attr)
            : _offset(offset), _selector(selector), _typeAttr(type_attr) {}

    IDTEntry(uint16_t selector, void(*isrFn)(), uint8_t type_attr) : IDTEntry(selector, uint32_t(isrFn), type_attr) {}

    friend class InterruptDescriptorTable;

  private:
    uint32_t _offset;
    uint16_t _selector;
    uint8_t _typeAttr;
};

class InterruptDescriptorTable
{
    static constexpr int kIdtSize = 256;
    InterruptServiceRoutine *isr_[kIdtSize];
    uint64_t idt_[kIdtSize];
  public:
    static std::uint64_t EncodeEntry(IDTEntry source);

    void encodeEntry(uint8_t entryNumber, IDTEntry source);
    void encodeISRs();
    void install() { set_idt(this->idt_, sizeof(uint64_t) * kIdtSize); }

    void callISR(InterruptNumber interruptNumber, RegisterTable &registers)
    {
        (*isr_[static_cast<int>(interruptNumber)])(registers);
    }

    void setISR(InterruptNumber interruptNumber, InterruptServiceRoutine *routine)
    {
        isr_[static_cast<int>(interruptNumber)] = routine;
    }
};
