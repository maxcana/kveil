#include <ntddk.h>
#include <string.h>
#include <utils.h>

static NTKERNELAPI PVOID RtlPcToFileHeader(PVOID PcValue, PVOID* BaseOfImage);

// original from decompilation:
// NTSTATUS __fastcall ExpQuerySystemInformation(int a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, unsigned int a3, _QWORD* a4, unsigned
// int Length, ULONG* a6)

typedef NTSTATUS __fastcall ExpQuerySystemInformation_t(int32_t a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, uint32_t a3, uint64_t* a4,
                                                        uint32_t Length, uint32_t* a6);

ExpQuerySystemInformation_t* try_find_ExpQuerySystemInformation()
{
    void* ntoskrnl = NULL;
    RtlPcToFileHeader(loc_NtQuerySystemInformation, &ntoskrnl);

    char* the_first_call_instruction = (char*)memchr(ntQuery, 0xE8, 256);
    if (the_first_call_instruction == NULL) return NULL;

    int32_t displacement;
    memcpy(&displacement, the_first_call_instruction + 1, 4);
    char* final_address = the_first_call_instruction + 5 + displacement;

    if (final_address < ntoskrnl || final_address > loc_NtQuerySystemInformation + 1000) return NULL;

    return (ExpQuerySystemInformation_t*)final_address;
}