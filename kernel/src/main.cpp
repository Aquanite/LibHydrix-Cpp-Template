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

extern void kernel_main() { 
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
    InitializeHeap(hhdm->offset);
    graphics.Init((uint32_t*)framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch, framebuffer->bpp, framebuffer->red_mask_shift, framebuffer->green_mask_shift, framebuffer->blue_mask_shift, framebuffer->red_mask_size, framebuffer->green_mask_size, framebuffer->blue_mask_size);
    console.Init(&graphics, 16, false);
    SetISRConsole(&console);
    KeyboardInit(&console);
    SetMouseConsole(&console);
    InitializeSyscall(&console);
    SetPCIConsole(&console);
    InitializeGDT();
    InitializeISR();
    EnableInterrupts();
    SetPITFrequency(1000); // 1000 Hz (1ms)
    InitializeTime();
    InitializePCI();
    FPU::Enable();
    rand_seed = (long int)framebuffer + (long int)memmap + (long int)hhdm;
    uint64_t mem = RetrieveTotalMemory(memmap);
    char* memstrbyte = ToString(mem);
    char* memstrkb = ToString(mem / 1024);
    char* memstrmb = ToString(mem / 1024 / 1024);
    uint32_t* drawingcanvas = new uint32_t[graphics.Width * graphics.Height];
    BMPI canvas = {(int*)drawingcanvas, (int) graphics.Width, (int) graphics.Height};
    Graphics canvasclass;
    canvasclass.Init((uint32_t*)drawingcanvas, framebuffer->width, framebuffer->height, framebuffer->pitch, framebuffer->bpp, framebuffer->red_mask_shift, framebuffer->green_mask_shift, framebuffer->blue_mask_shift, framebuffer->red_mask_size, framebuffer->green_mask_size, framebuffer->blue_mask_size);
    int brushSize = 10;
    int prevMouseX = GetMouseXPos();
    int prevMouseY = GetMouseYPos();
    MouseState LastState = GetCurrentMouseState();
    while (true)
    {
        console.Clear();
        canvasclass.Display();
        graphics.DrawImage(0, 0, canvas);
        console.WriteLine("Welcome to LibHydrix!", IColor::RGB(0, 255, 0));
        console.WriteLine("Left Click to Draw!", IColor::RGB(255, 255, 0));
        console.WriteLine("Right Click to Erase!", IColor::RGB(0, 255, 255));
        console.WriteLine("Middle Click to Change Brush Size!", IColor::RGB(255, 0, 255));
        //current brush size
        console.WriteLine(StringConcatenate("Current Brush Size: ", ToString(brushSize)), IColor::RGB(255, 0, 0));

        // Get the current mouse position
        int currMouseX = GetMouseXPos();
        int currMouseY = GetMouseYPos();

        // Check if the mouse is moved or clicked
        if (GetCurrentMouseState() == MouseState::MOUSE_LEFT || GetCurrentMouseState() == MouseState::MOUSE_RIGHT)
        {
            // Only interpolate if the mouse has moved
            if (currMouseX != prevMouseX || currMouseY != prevMouseY)
            {
                // Determine the number of interpolation steps
                int steps = MathI::Maximum(MathI::Absolute(currMouseX - prevMouseX), MathI::Absolute(currMouseY - prevMouseY));

                for (int i = 0; i <= steps; i++)
                {
                    // Interpolate the positions
                    float t = i / (float)steps;
                    int interpolatedX = (int)(prevMouseX + t * (currMouseX - prevMouseX));
                    int interpolatedY = (int)(prevMouseY + t * (currMouseY - prevMouseY));

                    // Draw the interpolated circle
                    int color = (GetCurrentMouseState() == MouseState::MOUSE_LEFT) ? IColor::RGB(255, 255, 255) : IColor::RGB(0, 0, 0);
                    canvasclass.DrawFilledCircle(interpolatedX, interpolatedY, brushSize, color);
                }
            }
            else
            {
                // If the mouse hasn't moved, draw directly
                int color = (GetCurrentMouseState() == MouseState::MOUSE_LEFT) ? IColor::RGB(255, 255, 255) : IColor::RGB(0, 0, 0);
                canvasclass.DrawFilledCircle(currMouseX, currMouseY, brushSize, color);
            }
        }

        if (GetCurrentMouseState() == MouseState::MOUSE_MIDDLE && LastState != MouseState::MOUSE_MIDDLE)
        {
            brushSize++;
            if (brushSize > 10)
            {
                brushSize = 1;
            }
        }
        //draw 50x20 rect on the top right corner
        graphics.DrawFilledRectangle(graphics.Width - 50, 0, 50, 20, IColor::RGB(127, 127, 157));
        graphics.DrawRectangle(graphics.Width - 50, 0, 50, 20, 0x6655FF);
        //draw word "CLEAR" in the rect center
        graphics.DrawString("CLEAR", graphics.Width - 54 + 5, 2, IColor::RGB(12, 0, 0));
        //if pressed, clear the canvas
        if (GetCurrentMouseState() == MouseState::MOUSE_LEFT && LastState != MouseState::MOUSE_LEFT && currMouseX > graphics.Width - 50 && currMouseY < 20)
        {
            canvasclass.Clear();
        }
        // Draw the current mouse position for debugging purposes
        graphics.DrawFilledCircle(currMouseX, currMouseY, brushSize, IColor::RGB(0, 0, 255));
        graphics.Display();

        // Update the previous mouse position
        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

        canvas.data = (int*)drawingcanvas;
        LastState = GetCurrentMouseState();
    }

    halt(); 
}
