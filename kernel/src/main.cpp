#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <libhydrix/libhydrix.h>

#pragma region "Limine Setup" 
namespace {__attribute__((used, section(".requests")))volatile LIMINE_BASE_REVISION(2);}
namespace {__attribute__((used, section(".requests_start_marker")))volatile LIMINE_REQUESTS_START_MARKER;__attribute__((used, section(".requests_end_marker")))volatile LIMINE_REQUESTS_END_MARKER;}
namespace { void halt() { for (;;) asm ("hlt"); } void hcf() { asm ("cli"); for (;;) { asm ("hlt"); } } }
extern "C" { int __cxa_atexit(void (*)(void *), void *, void *) { return 0; } void __cxa_pure_virtual() { hcf(); } }
extern void (*__init_array[])(); extern void (*__init_array_end[])();
#pragma endregion
#pragma region "Limine Requests"
namespace {
__attribute__((used, section(".requests")))
volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};
__attribute__((used, section(".requests")))
volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};
__attribute__((used, section(".requests")))
volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};
}
#pragma endregion

Graphics graphics;
Console console;

extern "C" void _start() { 
    #pragma region "Limine Final Setup"
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    for (size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }
    if (framebuffer_request.response == nullptr
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    #pragma endregion
    // Fetch the first framebuffer.
    limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    limine_memmap_response *memmap = memmap_request.response;
    limine_hhdm_response *hhdm = hhdm_request.response;
    heap_init(hhdm->offset);
    graphics.Init((uint32_t*)framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch);
    console.Init(&graphics, 16, true);
    set_isr_console(&console);
    Keyboard_Init(&console);
    set_mouse_console(&console);
    syscall_init(&console);
    gdt_init();
    isr_install();
    enable_interrupts();
    set_pit_freq(1000); // 1000 Hz (1ms)
    DisableKeyboard();
    FPU::Enable();
    rand_seed = (long int)framebuffer + (long int)memmap + (long int)hhdm;
    uint64_t mem = _retrieve_total_memory(memmap);
    char* memstrbyte = to_string(mem);
    char* memstrkb = to_string(mem / 1024);
    char* memstrmb = to_string(mem / 1024 / 1024);
    while (true)
    {
        console.Clear();
        graphics.put_line(rand() % graphics.width, 0, rand() % graphics.width, graphics.height, rgb(255, 0, 0));
        graphics.put_line(0, rand() % graphics.height, graphics.width, rand() % graphics.height, rgb(0, 255, 0));
        console.WriteLine("Welcome to LibHydrix!", rgb(0, 255, 0));
        console.WriteLine(strcat("Total Memory  (B): ", memstrbyte), rgb(255, 255, 0));
        console.WriteLine(strcat("Total Memory (KB): ", memstrkb), rgb(0, 255, 255));
        console.WriteLine(strcat("Total Memory (MB): ", memstrmb), rgb(255, 0, 255));
        for (uint32_t i = 0; i < 0xFFFFFFFF; i++);
        graphics.swap();
    }
    halt(); 
}
