#include <ntifs.h>
#include <stdint.h>
#include <utils.h>
#include <var.h>
uint64_t hooked_GreProtectSpriteContent(uint64_t gre_context, PVOID hwnd, int use_dwm_path, uint8_t new_affinity);
NTSTATUS __fastcall hooked_NtQuerySystemInformation(int32_t SystemInformationClass, uint64_t* SystemInformation, uint32_t SystemInformationLength, uint32_t* ReturnLength);

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

    return ok == STATUS_SUCCESS ? 1 : 0;
}

int hook_all()
{
    print("hooking ntoskrnl!NtQuerySystemInformation...\n");
    if (jmp_hook(loc_NtQuerySystemInformation, hooked_NtQuerySystemInformation) == 0) return 0;

    // i need to be in session space (session=1, not 0) so i have the right page tables to resolve win32kfull.sys
    // see: https://github.com/GetRektBoy724/Win32kHooker/tree/master
    {
        print("getting session-specific page tables for hooking session-space drivers...\n");
        uint32_t ses1_pid = find_a_session1_process_pid();
        if (ses1_pid == 0)
        {
            print("failed to find a session 1 PID.\n");
            return 0;
        }
        PEPROCESS ep;
        if (!NT_SUCCESS(PsLookupProcessByProcessId(ses1_pid, &ep))) return 0;

        KAPC_STATE apc;
        KeStackAttachProcess(ep, &apc); // now ffff9800'... resolves through session 1's tables
        // perform session-1-dependent hooks
        print("hooking win32kfull!GreProtectSpriteContent...\n");
        if (jmp_hook(loc_GreProtectSpriteContent, hooked_GreProtectSpriteContent) == 0)
        {
            KeUnstackDetachProcess(&apc);
            ObDereferenceObject(ep);
            return 0;
        }

        KeUnstackDetachProcess(&apc);
        ObDereferenceObject(ep);
    }

    print("hooks all succeeded.\n");
    return 1;
}