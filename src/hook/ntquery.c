#include <ntddk.h>
#include <undoc.h>
#include <var.h>
#include <wdm.h>

NTSTATUS __fastcall hooked_NtQuerySystemInformation(int32_t SystemInformationClass, uint64_t* SystemInformation, uint32_t SystemInformationLength,
                                                 uint32_t* ReturnLength)
{
    _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* groupBuffer = nullptr;
    unsigned int groupBufferSize = 0;
    __int16 primaryGroup = 0; // BYREF — passed by pointer to ExpQuerySystemInformation

    switch (SystemInformationClass)
    {
        // Explicitly unsupported / reserved class values — reject immediately
        case 107: // reserved (84–141 range)
        case 121: // reserved (84–141 range)
        case 180:
        case 210:
        case 211:
        case 222:
        case 231:
        case 238:
        case 239:
        case 240:
        case 254:
            return STATUS_INVALID_INFO_CLASS;

        // Per-processor-group classes: scope the query to the caller's primary processor group before dispatching
        case 8:   // SystemProcessorPerformanceInformation
        case 23:  // SystemInterruptInformation
        case 42:  // SystemProcessorIdleInformation
        case 61:  // SystemProcessorPowerInformation
        case 83:  // SystemProcessorIdleCycleTimeInformation
        case 100: // SystemProcessorPerformanceDistribution
        case 108: // SystemProcessorCycleTimeInformation
        case 141:
            primaryGroup = KeQueryPrimaryGroupThread(KeGetCurrentThread());
            [[fallthrough]];

        // Class 73 shares the group-scoped dispatch path but uses the initialized-to-zero primaryGroup (the "all processors / legacy" view) instead
        // of querying the current thread's actual group.
        case 73: // SystemLogicalProcessorInformation
            groupBuffer = (_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)&primaryGroup;
            groupBufferSize = sizeof(primaryGroup);
            break;

        // All other classes: no processor-group scoping needed
        default:
            // groupBuffer stays nullptr, groupBufferSize stays 0
            break;
    }

    return ExpQuerySystemInformation(SystemInformationClass, groupBuffer, groupBufferSize, SystemInformation, SystemInformationLength, ReturnLength);
}

NTSTATUS __fastcall hooked_NtQuerySystemInformationEx(int32_t InfoClass, _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* InputBuffer,
                                                   uint32_t InputBufferLength, uint64_t* SystemInformation, unsigned int SystemInformationLength,
                                                   uint32_t* ReturnLength)
{
    ULONG requiredAlignment;

    if (InputBuffer == NULL || InputBufferLength == 0) return STATUS_INVALID_PARAMETER;

    switch (InfoClass)
    {
        // Input buffer contains WORD-sized fields (2-byte alignment required)
        case 8:
        case 23:
        case 42:
        case 61:
        case 73:
        case 83:
        case 100:
        case 108:
        case 121:
        case 141:
        case 160:
            requiredAlignment = 2;
            break;

        // Input buffer contains DWORD-sized fields (4-byte alignment required)
        case 72:
        case 107:
        case 180:
        case 194:
        case 210:
        case 222:
        case 231:
        case 232:
        case 239:
        case 240:
        case 256:
            requiredAlignment = 4;
            break;

        // Input buffer contains QWORD-sized fields (8-byte alignment required)
        case 165:
        case 175:
        case 178:
        case 181:
        case 209:
        case 211:
        case 223:
        case 230:
        case 238:
        case 254:
            requiredAlignment = 8;
            break;

        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    // Alignment is only enforced for calls arriving from user mode
    if (KeGetCurrentThread()->PreviousMode != KernelMode && ((requiredAlignment - 1) & (ULONG_PTR)InputBuffer) != 0) ExRaiseDatatypeMisalignment();

    return ExpQuerySystemInformation(InfoClass, InputBuffer, InputBufferLength, SystemInformation, SystemInformationLength, ReturnLength);
}