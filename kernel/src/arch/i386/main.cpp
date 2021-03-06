//
// Created by Martin Miralles-Cordal on 8/14/2013.
//

#include <arch/i386/cpu/multiboot.h>
#include <arch/i386/cpu/PageFaultPanicISR.hpp>
#include <arch/i386/cpu/X86.hpp>
#include <arch/i386/cpu/X86RealTimeClock.hpp>
#include <arch/i386/device/input/PS2KeyboardISR.hpp>
#include <arch/i386/device/pit/PITIRQ.hpp>
#include <arch/i386/device/storage/X86AtaDevice.hpp>
#include <arch/i386/X86Kernel.hpp>
#include <device/input/KeyboardInputStream.hpp>
#include <fs/iso9660/Iso9660.hpp>
#include <proc/elf/Executable.hpp>

#include <system/Debug.hpp>

#include <util/StaticList.hpp>

#include <cstdio>
#include <cstring>
#include <new>

using sys::New;

#define STATIC_ALLOC(type_, name_) alignas(type_) std::byte name_[sizeof(type_)]

// ====================================================
// Globals
// ====================================================
extern uint32_t kernel_end;
VGA4BitColor defaultTextColor = COLOR_LIGHT_GREY;

Kernel *kernel = nullptr;
X86Kernel *x86Kernel = nullptr;
STATIC_ALLOC(X86Kernel, kern_mem);
PageFaultISR kPageFaultIsr;

extern "C" {

// ====================================================
// Function prototypes
// ====================================================
void init_system();
Volume *read_ata();
void read_multiboot(multiboot_info_t *info);

extern "C++"
template <typename Callable>
int log_task(char const *printstr, Callable &&c);

int check_flag(multiboot_info_t *info, char const *printstr, uint32_t flag);
int log_test(char const *printstr, int success);

// ====================================================
// Functions
// ====================================================
extern "C++"
void perform_task(char const *desc, auto &&task)
{
    sys::debug_print(desc);
    task();
    sys::debug_println("DONE.");
}

void kernel_main(multiboot_info_t *info, uint32_t magic)
{
    perform_task("Constructing kernel...", [] {
        // Get this party started
        x86Kernel = new(kern_mem) X86Kernel;
        kernel = x86Kernel;
    });

    log_task("Reading multiboot info...", [&] {
        if (magic != 0x2BADB002) {
            kernel->panic("Operating system not loaded by multiboot compliant bootloader.");
        }

        read_multiboot(info);
    });


    log_task("Installing CPU descriptor tables...", [] { x86Kernel->cpu().install(); });
    log_task("Setting up memory management unit...", [&] { x86Kernel->installMMU(info->mmap_addr, info->mmap_length); });
    log_task("Installing interrupt handlers...", [] {
        x86Kernel->cpu().idt()->setISR(InterruptNumber::kPageFault, &kPageFaultIsr);
        x86Kernel->installSyscalls();
        x86Kernel->cpu().enableInterrupts();
    });
    init_system();
}

void init_system()
{
    // prepare stdin
    auto kb = New<PS2Keyboard>();
    PS2KeyboardISR::install(x86Kernel->cpu(), kb);
    PITIRQ::install(x86Kernel->cpu());
    kernel->setIn(New<KeyboardInputStream>(kb));
    auto cd = read_ata();

    kernel->console()->setForegroundColor(COLOR_WHITE);
    puts("\n* * *");
    DateTime now = X86RealTimeClock::currentTime();
    printf("The current date and time is %d/%d/%d", now.month, now.monthday, now.year + (now.century * 100));
    printf(" %d", now.hours);
    uint8_t minsec[2] {now.minutes, now.seconds};
    for (auto t : minsec) {
        printf(":");
        if (t < 10) printf("0");
        printf("%d", t);
    }
    puts("\n* * *");

    sys::debug_println("Finding /bin/proc-test...");
    auto proctestEntry = cd->find("/bin/proc-test");
    if (proctestEntry) {
        auto proctest = elf::Executable(*proctestEntry);
        printf("Running `proc-test A`...\n");
        kernel->console()->setCursorVisible(true);
        proctest("A");
        printf("Running `proc-test B`...\n");
        kernel->console()->setCursorVisible(true);
        proctest("B");
    } else {
        puts("Unable to find proc-test! You might want to look into that.");
    }
    // run shell
    kernel->console()->setForegroundColor(defaultTextColor);
    puts("");

    auto cvshEntry = cd->find("/bin/kvshell");
    if (cvshEntry) {
        auto kvshell = elf::Executable(*cvshEntry);
        printf("Running kvshell...\n");
        kernel->console()->setCursorVisible(true);
        kvshell.exec();
    } else {
        puts("Unable to find kvshell! You might want to look into that.");
    }

    // mock developer
    kernel->console()->setForegroundColor(COLOR_LIGHT_RED);
    printf("Kernel exited. Maybe you should write the rest of the operating system?");
    kernel->console()->setForegroundColor(defaultTextColor);
    putchar('\n');

    // show keyboard input indefinitely
    while (true) {
        kernel->in()->read();
    }
}

Volume *read_ata()
{
    Volume *cdVolume = nullptr;
    kernel->console()->setForegroundColor(COLOR_WHITE);
    kernel->console()->writeString("\nPATA Device Information\n");
    kernel->console()->setForegroundColor(defaultTextColor);
    X86AtaDevice devices[4] { {true, true}, {true, false}, {false, true}, {false, false} };
    for (auto &device : devices) {
        printf(" * Device %s -- %s\n",
               device.isPrimary() ? "Primary" : "Secondary",
               device.isMaster() ? "Master" : "Slave");

        if (!device.isAttached()) {
            puts("    [Not attached]");
            continue;
        }

        printf("    Name: %s\n", device.model());
        printf("    Type: ");
        switch (device.type()) {
            case X86AtaDevice::Type::PATA:
                puts("PATA");
                break;
            case X86AtaDevice::Type::PATAPI:
                puts("PATAPI");
                break;
            case X86AtaDevice::Type::SATA:
                puts("SATA");
                break;
            case X86AtaDevice::Type::SATAPI:
                puts("SATAPI");
                break;
            case X86AtaDevice::Type::Unknown:
                puts("Unknown");
                break;
        }
        printf("    Serial: %s\n", device.serial());
        printf("    FW: %s\n", device.firmware());
        switch (device.type()) {
            case X86AtaDevice::Type::PATAPI:
            case X86AtaDevice::Type::SATAPI: {
                printf("    Sector size: %u bytes\n", device.sectorSize());
                printf("    Checking for ISO9660... ");
                bool isIso9660 = Iso9660::instance().hasFileSystem(device);
                puts(isIso9660 ? "yes!" : "no");
                if (isIso9660) {
                    auto heapDevice = New<X86AtaDevice>(device);
                    cdVolume = Iso9660::instance().createVolume(heapDevice);
                    auto entry = cdVolume->find("/bin/elf-test");
                    printf("    Found '/bin/elf-test'? %s\n", entry ? "yes!" : "no");
                    if (!entry) break;
                    bool isElf = elf::Executable::isElf(*entry);
                    printf("    Recognized ELF? %s\n", isElf ? "yes!" : "no");
                    puts  ("    Executing entry point...");
                    puts  ("    ======");
                    int retval = elf::Executable(*entry)();
                    puts  ("    ======");
                    printf("    Returning from entry point... -> %x\n", retval);
                }

                break;
            }
            default:
                break;
        }
    }

    return cdVolume;
}

void read_multiboot(multiboot_info_t *info)
{
    int result;

    result = check_flag(info, "Checking for reported memory size...", MULTIBOOT_INFO_MEMORY);
    if (result) {
        printf("%dKiB lower memory.\n", info->mem_lower);
        printf("%dKiB upper memory.\n", info->mem_upper);
    }

    result = check_flag(info, "Checking for reported memory map...", MULTIBOOT_INFO_MEM_MAP);

    if (!result) {
        kernel->panic("Fatal ERROR: Cannot install paging due to missing memory map.");
    } else {
        multiboot_memory_map_t *mmap;
        for (mmap = (multiboot_memory_map_t *) info->mmap_addr;
             (uint32_t) mmap < info->mmap_addr + info->mmap_length;
             mmap = (multiboot_memory_map_t * )((uint32_t) mmap + mmap->size + sizeof(mmap->size))) {
            printf("block: [%x-%x) (len: %x) type: %x\n", (unsigned int) mmap->addr, (unsigned int)(mmap->addr+mmap->len), (unsigned int) mmap->len,
                   (unsigned int) mmap->type);
        }
    }
}

size_t log_task_begin(char const *printstr)
{
    sys::debug_print(printstr);
    kernel->console()->moveTo(kernel->console()->row(), 0);
    for (uint32_t i = 0; i < kernel->console()->width(); ++i) {
        kernel->console()->putChar(' ');
    }
    kernel->console()->moveTo(kernel->console()->row() - 1, 0);

    kernel->console()->setForegroundColor(COLOR_WHITE);
    kernel->console()->writeString(printstr);

    if (kernel->console()->column() > kernel->console()->width() - 6) {
        kernel->console()->writeString("\n");
    }

    kernel->console()->setForegroundColor(defaultTextColor);

    return kernel->console()->row();
}

int log_result(int success, char const *ackstr, char const *nakstr)
{
    if (success) {
        sys::debug_println(ackstr);
        kernel->console()->moveTo(kernel->console()->row(), kernel->console()->width() - (strlen(ackstr) + 3));
        kernel->console()->writeString("[");
        kernel->console()->setForegroundColor(COLOR_GREEN);
        kernel->console()->writeString(ackstr);
    } else {
        sys::debug_println(nakstr);
        kernel->console()->moveTo(kernel->console()->row(), kernel->console()->width() - (strlen(nakstr) + 3));
        kernel->console()->writeString("[");
        kernel->console()->setForegroundColor(COLOR_RED);
        kernel->console()->writeString(nakstr);
    }
    kernel->console()->setForegroundColor(COLOR_WHITE);
    kernel->console()->writeString("]\n");

    kernel->console()->setForegroundColor(defaultTextColor);

    return success;
}

extern "C++"
template <typename Callable>
int log_task(char const *printstr, Callable &&c)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        log_task_begin(printstr);
        c();
        return log_result(true, "DONE", "FAIL");
    }
    else {
        log_task_begin(printstr);
        auto result = c();
        return log_result(static_cast<int>(result), "DONE", "FAIL");
    }
}

int check_flag(multiboot_info_t *info, char const *printstr, uint32_t flag)
{
    log_task_begin(printstr);
    return log_result(int(info->flags & flag), "FOUND", "FAIL");
}

int log_test(char const *printstr, int success)
{
    log_task_begin(printstr);
    return log_result(success, "PASS", "FAIL");
}

} // extern C
