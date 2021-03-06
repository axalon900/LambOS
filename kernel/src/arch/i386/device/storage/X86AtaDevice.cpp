#include <arch/i386/device/storage/X86AtaDevice.hpp>
#include <system/asm.h>
#include <Kernel.hpp>

#include <cstdio>

X86AtaDevice::Type X86AtaDevice::typeOf(bool primary, bool master)
{
    // stack initialization is cheap, so IDGAF
    return X86AtaDevice(primary, master).type();
}

bool X86AtaDevice::isAttached() const
{
    softReset(); /* waits until master drive is ready again */
    outb(static_cast<std::uint16_t>(_ioPort + kAtaRegisterDriveSelect), static_cast<std::uint8_t>(0xA0 | _slaveBit << 4u));
    outb(static_cast<std::uint16_t>(_ioPort + kAtaRegisterLbaLow), 0x0);
    outb(static_cast<std::uint16_t>(_ioPort + kAtaRegisterLbaMid), 0x0);
    outb(static_cast<std::uint16_t>(_ioPort + kAtaRegisterLbaHigh), 0x0);
    outb(static_cast<std::uint16_t>(_ioPort + kAtaRegisterCommand), 0xEC);
    return waitForStatus(-1) != 0;
}

char const *X86AtaDevice::model()
{
    return descriptor().model();
}

char const *X86AtaDevice::serial()
{
    return descriptor().serial();
}

char const *X86AtaDevice::firmware()
{
    return descriptor().firmware();
}

X86AtaDevice::Type X86AtaDevice::type() const
{
    softReset(); /* waits until master drive is ready again */
    outb(std::uint16_t(_ioPort + kAtaRegisterDriveSelect), std::uint8_t(0xA0 | _slaveBit << 4));
    ioWait(); /* wait 400ns for drive select to work */
    uint8_t cl = inb(std::uint16_t(_ioPort + kAtaRegisterCylinderLow));	/* get the "signature bytes" */
    uint8_t ch = inb(std::uint16_t(_ioPort + kAtaRegisterCylinderHigh));

    /* differentiate ATA, ATAPI, SATA and SATAPI */
    if (cl==0x14 && ch==0xEB) return Type::PATAPI;
    if (cl==0x69 && ch==0x96) return Type::SATAPI;
    if (cl==0 && ch == 0) return Type::PATA;
    if (cl==0x3c && ch==0xc3) return Type::SATA;
    return Type::Unknown;
}

bool X86AtaDevice::read(uint64_t address, uint16_t *buf, size_t sectors)
{
    if (sectors == 0) return false;

    if (sectors > 10) {
        size_t const bufIncr = sectorSize() / sizeof(uint16_t);

        bool success = true;
        for (size_t offset = 0; offset < sectors; offset += 10) {
            size_t const bufOffset = bufIncr * offset;
            size_t sectorsToRead = sectors - offset < 10 ? sectors - offset : 10;
            success = readInternal(address + offset, buf + bufOffset, sectorsToRead);
            if (!success) {
                break;
            }
        }

        return success;
    } else {
        return readInternal(address, buf, sectors);
    }

}

bool X86AtaDevice::readInternal(uint64_t address, uint16_t *buf, size_t sectors)
{
    const size_t maxByteCount = sectorSize() * sectors;
    outb(_ioPort + kAtaRegisterDriveSelect, std::uint8_t(0xA0 | _slaveBit << 4));
    ioWait();

    switch (type()) {
        case Type::PATAPI:
        case Type::SATAPI:
            // send READ(12)
            return performPioAtapiOperation(AtapiCommand::read12Command(uint32_t(address), sectors), buf, maxByteCount);
        case Type::PATA:
        case Type::SATA:
            // should send something, but for now...
            kernel->panic("Attempted to read from unsupported ATA device.");
            return false;
        default:
            return false;
    }
}

uint8_t X86AtaDevice::waitForStatus(int timeout) const
{
    uint8_t status;
    if (timeout > 0) {
        int i = 0;
        while ((status = inb(_ioPort + kAtaRegisterStatus)) & kAtaStatusBitBusy && (i < timeout)) i++;
    } else {
        while ((status = inb(_ioPort + kAtaRegisterStatus)) & kAtaStatusBitBusy);
    }
    return status;
}

uint8_t X86AtaDevice::waitUntilReady(bool errorCheck) const
{
    uint8_t status = 0;

    ioWait();

    status = waitForStatus(-1);

    if (errorCheck) {
        status = inb(_ioPort + kAtaRegisterStatus);
        if (status & kAtaStatusBitError) return 1;
        if (status & kAtaStatusBitDriveFault) return 1;
        if (!(status & kAtaStatusBitDRQ)) return 1;
    }

    return 0;
}

void X86AtaDevice::ioWait() const
{
    inb(_ioPort + 0x0C);
    inb(_ioPort + 0x0C);
    inb(_ioPort + 0x0C);
    inb(_ioPort + 0x0C);
}

void X86AtaDevice::softReset() const
{
    outb(_controlPort, 0x04);
    ioWait();
    outb(_controlPort, 0x00);
}

void X86AtaDevice::identify() const
{
    // Set features register. 0 = PIO, 1 = DMA.
    outb(_ioPort + 1, 0);
    outb(_controlPort, 0);

    // select device
    outb(_ioPort + kAtaRegisterDriveSelect, std::uint8_t(0xA0 | _slaveBit << 4u));
    ioWait();

    switch (type()) {
        case Type::PATAPI:
        case Type::SATAPI:
            // send IDENTIFY PACKET DEVICE
            outb(_ioPort + 0x07, 0xA1); break;
        case Type::PATA:
        case Type::SATA:
            // send IDENTIFY DEVICE
            outb(_ioPort + 0x07, 0xEC); break;
        case Type::Unknown:
            return;
    }
    ioWait();

    // read in status
    waitUntilReady(false);

    // read in device information
    uint16_t identity[256];
    pioRead(identity, 256);

    _descriptor.readIdentity(identity);
    _identified = true;

    if (type() == Type::PATAPI) {
        uint16_t data[4];
        performPioAtapiOperation(AtapiCommand::readCapacityCommand(), data, sizeof(data));
        _descriptor.readAtapiCapacity(data);
    }
}

bool X86AtaDevice::performPioAtapiOperation(const AtapiCommand &cmd, uint16_t *buf, size_t bufSize) const
{
    uint8_t status = 0;

    // Set PIO
    outb(_ioPort + kAtaRegisterFeatureInfo, 0x00);

    // set maximum size for requested data
    outb(_ioPort + kAtaRegisterLbaMid, static_cast<uint8_t>(bufSize & 0xFF));
    outb(_ioPort + kAtaRegisterLbaHigh, static_cast<uint8_t>(bufSize >> 8));

    // send PACKET cmd
    outb(_ioPort + kAtaRegisterCommand, 0xA0);

    // poll
    while (1) {
        status = inb(_ioPort + kAtaRegisterStatus);
        if ((status & kAtaStatusBitError)) { puts("ATAPI early error; unsure"); return false; }
        if (!(status & kAtaStatusBitBusy) && (status & kAtaStatusBitReady)) break;
    }

    // send the ATAPI command packet
    for (int i = 0; i < 6; ++i) {
        outw(_ioPort, cmd.packet().words[i]);
    }

    // poll
    while (1) {
        status = inb(_ioPort + kAtaRegisterStatus);
        if ((status & kAtaStatusBitError)) { puts("ATAPI error; no medium?"); return false; }
        if (!(status & kAtaStatusBitBusy) && (status & kAtaStatusBitReady)) break;
        if ((status & kAtaStatusBitDRQ)) break;
    }

    // check if there's data to read
    if ((status & kAtaStatusBitDRQ)) {
        // get actual size of data. We could optimize our buffer size a bit here, but whatever.
        auto readSizeBytes = uint16_t(inb(_ioPort + kAtaRegisterLbaHigh) << 8);
        readSizeBytes = uint16_t(readSizeBytes | inb(_ioPort + kAtaRegisterLbaMid));
        pioRead(buf, readSizeBytes);
    } else if (bufSize) {
        puts("No data to read but we made a buffer. Really makes you think...");
    }

    return true;
}

void X86AtaDevice::pioRead(uint16_t *buf, uint32_t bytesToRead) const
{
    uint32_t wordCount = bytesToRead / 2;
    for (size_t i = 0; i < wordCount; ++i) {
        buf[i] = inw(_ioPort);
    }
}

void X86AtaDevice::pioWrite(uint16_t *buf, uint32_t bytesToRead) const
{
    uint32_t wordCount = bytesToRead / 2;
    for (size_t i = 0; i < wordCount; ++i) {
        outw(_ioPort, buf[i]);
    }
}