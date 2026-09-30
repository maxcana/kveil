#include <ntddk.h>
#include <string.h>
#include <utils.h>

static NTKERNELAPI PVOID RtlPcToFileHeader(PVOID PcValue, PVOID* BaseOfImage);

// original from decompilation:
// NTSTATUS __fastcall ExpQuerySystemInformation(int a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, unsigned int a3, _QWORD* a4, unsigned
// int Length, ULONG* a6)

typedef NTSTATUS __fastcall ExpQuerySystemInformation_t(int32_t a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, uint32_t a3, uint64_t* a4,
                                                        uint32_t Length, uint32_t* a6);

// original from decompilation; wait, it literally just adds 584 to it. but idk of this changes depending on windows version:
// __int64 __fastcall KeQueryPrimaryGroupThread(__int64 a1)
// {
//   return *(unsigned __int16 *)(a1 + 584);
// }
typedef USHORT __fastcall KeQueryPrimaryGroupThread(int64_t a1);