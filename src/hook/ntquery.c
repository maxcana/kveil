#include <ntddk.h>
#include <undoc.h>
#include <var.h>
#include <wdm.h>

NTSTATUS __fastcall hooked_NtQuerySystemInformation(int32_t SystemInformationClass, uint64_t* SystemInformation, uint32_t SystemInformationLength, uint32_t* ReturnLength)
{
    _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* grpBuf = nullptr;
    unsigned int grpBufSize = 0;
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
            grpBuf = (_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)&primaryGroup;
            grpBufSize = sizeof(primaryGroup);
            break;

        // All other classes: no processor-group scoping needed
        default:
            // grpBuf stays nullptr, grpBufSize stays 0
            break;
    }

    NTSTATUS exp_status = loc_ExpQuerySystemInformation(SystemInformationClass, grpBuf, grpBufSize, SystemInformation, SystemInformationLength, ReturnLength);
    if (!NTSTATUS(exp_status)) return exp_status;

    // output: addr=SystemInformation, length=*ReturnLength

    switch (SystemInformationClass)
    {
        //* process enumeration
        //  all of these are pretty much the same

        // SystemProcessInformation
        case 0x05: {
            // busywork
            uint64_t size = 131072; // 128 KiB initial guess
            PVOID buf;
            NTSTATUS s;
            do
            {
                buf = ExAllocatePool2(POOL_FLAG_PAGED, size, 'NQSI');
                if (!buf) return;

                s = loc_ExpQuerySystemInformation(0x05, grpBuf, grpBufSize, buf, size, &size);
                if (!NT_SUCCESS(s)) ExFreePoolWithTag(buf, 'NQSI');
            }
            while (s == STATUS_INFO_LENGTH_MISMATCH); // ReturnLength is written by the kernel on STATUS_INFO_LENGTH_MISMATCH
            if (!NT_SUCCESS(s)) return s;

            // the real stuff
            _SYSTEM_PROCESS_INFORMATION* lastp = NULL;
            for (_SYSTEM_PROCESS_INFORMATION* p = buf;;) // huh, C allows implicit void* -> Any* casts in declaration, but only for void*. cool
            {
                // ImageName: filename of the binary, ex. "python.exe", "python3.exe", "pythonw.exe"
                // UniqueProcessId: the real PID, ex. 1234
                if (p->ImageName != NULL && lastp != NULL && should_hide(p->ImageName))
                {
                    lastp->NextEntryOffset += p->NextEntryOffset;
                }

                // no more processes; final linked list entry
                if (!p->NextEntryOffset) break;

                lastp = p;
                p = (SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
            }
            memcpy(SystemInformation, grpBuf, grpBufSize, buf, size);
            ExFreePoolWithTag(buf, 'NQSI');
            *ReturnLength = (uint32_t)size;
            return STATUS_SUCCESS;
        }
        // SystemSessionProcessInformation
        case 0x35: {
            // busywork
            SYSTEM_SESSION_PROCESS_INFORMATION q = {0};
            NTSTATUS s;

            q.SessionId = session_id;
            q.SizeOfBuf = 131072;
            do
            {
                q.Buffer = ExAllocatePool2(POOL_FLAG_PAGED, size, 'NQSI');
                if (!q.Buffer) return;

                // TODO review this; it's concerning. do i really pass in q and not q.Buffer?
                s = loc_ExpQuerySystemInformation(0x35, &q, sizeof(q), NULL);
                if (!NT_SUCCESS(s)) ExFreePoolWithTag(buf, 'NQSI');
            }
            while (s == STATUS_INFO_LENGTH_MISMATCH);
            if (!NT_SUCCESS(s)) return s;

            // the real stuff
            _SYSTEM_PROCESS_INFORMATION* lastp = NULL;
            for (_SYSTEM_PROCESS_INFORMATION* p = q.Buffer;;)
            {
                if (p->ImageName != NULL && lastp != NULL && should_hide(p->ImageName))
                {
                    lastp->NextEntryOffset += p->NextEntryOffset;
                }

                if (!p->NextEntryOffset) break;

                lastp = p;
                p = (SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
            }
            memcpy(SystemInformation, grpBuf, grpBufSize, buf, size);
            ExFreePoolWithTag(buf, 'NQSI');
            *ReturnLength = (uint32_t)size;
            return STATUS_SUCCESS;
        }
        // SystemExtendedProcessInformation
        case 0x39: {
            // busywork
            uint64_t size = 262144;
            PVOID buf;
            NTSTATUS s;
            do
            {
                buf = ExAllocatePool2(POOL_FLAG_PAGED, size, 'NQSI');
                if (!buf) return;

                s = loc_ExpQuerySystemInformation(0x39, buf, size, &size);
                if (!NT_SUCCESS(s)) ExFreePoolWithTag(buf, 'NQSI');
            }
            while (s == STATUS_INFO_LENGTH_MISMATCH);
            if (!NT_SUCCESS(s)) return s;

            // the real stuff
            _SYSTEM_PROCESS_INFORMATION* lastp = NULL;
            for (_SYSTEM_PROCESS_INFORMATION* p = buf;;)
            {
                if (p->ImageName != NULL && lastp != NULL && should_hide(p->ImageName))
                {
                    lastp->NextEntryOffset += p->NextEntryOffset;
                }

                if (!p->NextEntryOffset) break;

                lastp = p;
                p = (SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
            }
            memcpy(SystemInformation, grpBuf, grpBufSize, buf, size);
            ExFreePoolWithTag(buf, 'NQSI');
            *ReturnLength = (uint32_t)size;
            return STATUS_SUCCESS;
        }
        // SystemFullProcessInformation
        case 0x94: {
            // busywork
            uint64_t size = 262144;
            PVOID buf;
            NTSTATUS s;
            do
            {
                buf = ExAllocatePool2(POOL_FLAG_PAGED, size, 'NQSI');
                if (!buf) return;

                s = loc_ExpQuerySystemInformation(0x94, buf, size, &size);
                if (!NT_SUCCESS(s)) ExFreePoolWithTag(buf, 'NQSI');
            }
            while (s == STATUS_INFO_LENGTH_MISMATCH);
            if (!NT_SUCCESS(s)) return s;

            // the real stuff
            _SYSTEM_PROCESS_INFORMATION* lastp = NULL;
            for (_SYSTEM_PROCESS_INFORMATION* p = buf;;)
            {
                if (p->ImageName != NULL && lastp != NULL && should_hide(p->ImageName))
                {
                    lastp->NextEntryOffset += p->NextEntryOffset;
                }

                if (!p->NextEntryOffset) break;

                lastp = p;
                p = (_SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
            }
            memcpy(SystemInformation, grpBuf, grpBufSize, buf, size);
            ExFreePoolWithTag(buf, 'NQSI');
            *ReturnLength = (uint32_t)size;
            return STATUS_SUCCESS;
        }
        // SystemBasicProcessInformation
        case 0xFC: {
            // busywork
            uint64_t size = 262144;
            PVOID buf;
            NTSTATUS s;
            do
            {
                buf = ExAllocatePool2(POOL_FLAG_PAGED, size, 'NQSI');
                if (!buf) return;

                s = loc_ExpQuerySystemInformation(0xFC, buf, size, &size);
                if (!NT_SUCCESS(s)) ExFreePoolWithTag(buf, 'NQSI');
            }
            while (s == STATUS_INFO_LENGTH_MISMATCH);
            if (!NT_SUCCESS(s)) return s;

            // the real stuff
            _SYSTEM_BASICPROCESS_INFORMATION* lastp = NULL;
            for (_SYSTEM_BASICPROCESS_INFORMATION* p = buf;;)
            {
                if (p->ImageName != NULL && lastp != NULL && should_hide(p->ImageName))
                {
                    lastp->NextEntryOffset += p->NextEntryOffset;
                }

                if (!p->NextEntryOffset) break;

                lastp = p;
                p = (_SYSTEM_BASICPROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
            }
            memcpy(SystemInformation, grpBuf, grpBufSize, buf, size);
            ExFreePoolWithTag(buf, 'NQSI');
            *ReturnLength = (uint32_t)size;
            return STATUS_SUCCESS;
        }

        //* ci
        // SystemCodeIntegrityInformation
        case 0x67: {
            // 0x0001: ENABLED: If this is NOT set, code integrity is fully disabled
            // 0x0002: TESTSIGN
            // 0x0004: UMCI_ENABLED
            // 0x0008: UMCI_AUDITMODE_ENABLED
            // 0x0010: UMCI_EXCLUSIONPATHS_ENABLED
            // 0x0020: TEST_BUILD: Indicates the OS itself is a Microsoft-internal test build.
            // 0x0040: PREPRODUCTION_BUILD: A pre-release internal Windows. Also not a retail OS.
            // 0x0080: DEBUGMODE_ENABLED: bcdedit /debug on
            // 0x0100: FLIGHT_BUILD: The machine is running a Windows Insider preview (less stable). Vanguard blocks you if this is on.
            // 0x0200: FLIGHTING_ENABLED: Same as above.
            // 0x0400: HVCI_KMCI_ENABLED: HVCI is on.
            // 0x0800: HVCI_KMCI_AUDITMODE_ENABLED: HVCI isn't really on, it just logs violations but doesn't block them.
            // 0x1000: HVCI_KMCI_STRICTMODE_ENABLE: HVCI is on, and on strict mode. Windows Settings "Memory Integrity" turns this on.
            // 0x2000: HVCI_IUM_ENABLED: IUM, which requires VBS, is on. VBS (Virtualization-based security) is on. You cannot have HVCI without VBS.
            NTSTATUS s = loc_ExpQuerySystemInformation(0x67, grpBuf, grpBufSize, SystemInformation, SystemInformationLength, ReturnLength);
            if (!NT_SUCCESS(s)) return s;

            _SYSTEM_CODEINTEGRITY_INFORMATION ci = {0};
            ci.Length = sizeof(ci);
            // basic spoof HVCI on; if this matters to you, also hook SystemIsolatedUserModeInformation etc
            // the important part is that the ENABLED bit is faked
            ci.CodeIntegrityOptions = 0x0400 + 0x1000 + 0x0001;
            memcpy(SystemInformation, &ci, sizeof(ci));

            return STATUS_SUCCESS;
        }
    }

    return exp_status;
}

// NTSTATUS __fastcall hooked_NtQuerySystemInformationEx(int32_t InfoClass, _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* InputBuffer, uint32_t InputBufferLength,
//                                                       uint64_t* SystemInformation, unsigned int SystemInformationLength, uint32_t* ReturnLength)
// {
//     ULONG requiredAlignment;

//     if (InputBuffer == NULL || InputBufferLength == 0) return STATUS_INVALID_PARAMETER;

//     switch (InfoClass)
//     {
//         // Input buffer contains WORD-sized fields (2-byte alignment required)
//         case 8:
//         case 23:
//         case 42:
//         case 61:
//         case 73:
//         case 83:
//         case 100:
//         case 108:
//         case 121:
//         case 141:
//         case 160:
//             requiredAlignment = 2;
//             break;

//         // Input buffer contains DWORD-sized fields (4-byte alignment required)
//         case 72:
//         case 107:
//         case 180:
//         case 194:
//         case 210:
//         case 222:
//         case 231:
//         case 232:
//         case 239:
//         case 240:
//         case 256:
//             requiredAlignment = 4;
//             break;

//         // Input buffer contains QWORD-sized fields (8-byte alignment required)
//         case 165:
//         case 175:
//         case 178:
//         case 181:
//         case 209:
//         case 211:
//         case 223:
//         case 230:
//         case 238:
//         case 254:
//             requiredAlignment = 8;
//             break;

//         default:
//             return STATUS_INVALID_INFO_CLASS;
//     }

//     // Alignment is only enforced for calls arriving from user mode
//     if (KeGetCurrentThread()->PreviousMode != KernelMode && ((requiredAlignment - 1) & (ULONG_PTR)InputBuffer) != 0) ExRaiseDatatypeMisalignment();

//     return ExpQuerySystemInformation(InfoClass, InputBuffer, InputBufferLength, SystemInformation, SystemInformationLength, ReturnLength);
// }