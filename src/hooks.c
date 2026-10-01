#include <hook/ntquery.c>
#include <hook/greprotect.c>
#include <utils.h>
#include <var.h>

// naked mov rax, imm64; jmp rax. does not push/pop the original value of rax. as long as we don't hook a *variadic* function, this is fine per the Win64 ABI.
int jmp_hook(void* src, void* dest)
{
    // clang-format off
    uint8_t* bytes = (uint8_t[]){
        0x48, 0xB8, // mov rax,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // imm64
        0xFF, 0xE0 // jmp rax
    };
    // clang-format on

    memcpy(&bytes[2], &dest, sizeof(void*));
    NTSTATUS ok = kmemcpy(src, bytes, 12);

    return ok == STATUS_SUCCESS ? 0 : 1;
}

int hook_all()
{
    print("hook_all()\n");
    int fails = 0;
    
    print("hooking NtQuerySystemInformation...\n");
    fails += jmp_hook(loc_NtQuerySystemInformation, hooked_NtQuerySystemInformation);
    // print("hooking NtQuerySystemInformationEx...\n");
    // fails += jmp_hook(loc_NtQuerySystemInformationEx, hooked_NtQuerySystemInformationEx);

    // TODO this will probably not work. i think i need to be in session space for the right page tables, but idk.
    print("hooking GreProtectSpriteContent...\n");
    fails += jmp_hook(loc_GreProtectSpriteContent, hooked_GreProtectSpriteContent);

    print("hooks failed %d times.\n", fails);
    return fails;
}