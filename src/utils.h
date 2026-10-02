#include <ntddk.h>
#include <stdlib.h>
#include <string.h>
#include <wdf.h>

// MARK: misc

void* memmem(const void* hay, size_t hlen, const void* ned, size_t nlen)
{
    if (nlen == 0) return (void*)hay;
    if (hlen < nlen) return NULL;

    const unsigned char* h = hay;
    const unsigned char* n = ned;
    size_t last = hlen - nlen;

    for (size_t pos = 0; pos <= last; pos++)
    {
        const unsigned char* p = memchr(h + pos, n[0], last - pos + 1);
        if (!p) return NULL;
        pos = (size_t)(p - h);
        if (memcmp(p, n, nlen) == 0) return (void*)p;
    }
    return NULL;
}

// returns a pointer to the same thing; doesn't copy the buffer
UNICODE_STRING uniencode(const wchar_t* in)
{
    UNICODE_STRING uni;
    RtlInitUnicodeString(&uni, in);
    return uni;
}

char* where(const wchar_t* routine_name)
{
    UNICODE_STRING uni = uniencode(routine_name);
    return MmGetSystemRoutineAddress(&uni);
}

/*
thanks to https://github.com/mrexodia/TitanHide for original function "RtlSuperCopyMemory"
---
The MIT License (MIT)

Copyright (c) 2015 TitanHide

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// copy memory even to write-protected regions
NTSTATUS kmemcpy(VOID UNALIGNED* Destination, CONST VOID UNALIGNED* Source, ULONG Length)
{
    const KIRQL Irql = KeRaiseIrqlToDpcLevel();

    PMDL Mdl = IoAllocateMdl(Destination, Length, 0, 0, nullptr);
    if (Mdl == nullptr)
    {
        KeLowerIrql(Irql);
        return STATUS_NO_MEMORY;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    // Hack: prevent bugcheck from Driver Verifier and possible future versions of Windows
    const CSHORT OriginalMdlFlags = Mdl->MdlFlags;
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
    Mdl->MdlFlags &= ~MDL_SOURCE_IS_NONPAGED_POOL;

    // Map pages and do the copy
    const PVOID Mapped = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmCached, nullptr, FALSE, HighPagePriority);
    if (Mapped == nullptr)
    {
        Mdl->MdlFlags = OriginalMdlFlags;
        IoFreeMdl(Mdl);
        KeLowerIrql(Irql);
        return STATUS_NONE_MAPPED;
    }

    RtlCopyMemory(Mapped, Source, Length);

    MmUnmapLockedPages(Mapped, Mdl);
    Mdl->MdlFlags = OriginalMdlFlags;

    IoFreeMdl(Mdl);
    KeLowerIrql(Irql);

    return STATUS_SUCCESS;
}

void print(PCSTR format, ...)
{
    va_list argList;
    va_start(argList, format);
    vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format, argList);
    va_end(argList);
    return;
}

void bsod()
{
    WdfVerifierKeBugCheck(0xE2, 0, 0, 0, 0);
    return;
}


// if it returns PID 0 (that's the System Idle Process; obviously a SessionId=0 process), we failed.
// REQUIRES loc_NtQuerySystemInformation to be filled in; var.c must be initialized!!!!
DWORD find_a_session1_process_pid()
{
    ULONG bufSize = 0;
    NTSTATUS status;
    PVOID result = NULL;

    status = ZwQuerySystemInformation(0x5, NULL, 0, &bufSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH || bufSize == 0) return NULL;

    PRTL_PROCESS_MODULES procs = (PRTL_PROCESS_MODULES)ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, 'BOOM');
    if (!procs) return NULL;

    status = ZwQuerySystemInformation(0x5, procs, bufSize, &bufSize);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(mods, 'BOOM');
        return 0;
    }

    for (_SYSTEM_PROCESS_INFORMATION* p = procs;;)
    {
        // first SessionId 1 process (or other positive SessionId ig)
        if (p->SessionId != 0)
        {
            DWORD pid = p->UniqueProcessId;
            print("find_session1_process_pid: FOUND! %wZ, PID %d", p->ImageName, (uint32_t)pid);
            ExFreePoolWithTag(mods, 'BOOM');
            return pid;
        }

        if (!p->NextEntryOffset) break;
        p = (_SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
    }

    // cleanup
    ExFreePoolWithTag(mods, 'BOOM');
    return 0;
}
