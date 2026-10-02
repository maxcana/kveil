#include <ntddk.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <undoc.h>
#include <var.h>
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
// WARNING: if another CPU is currently executing inside those first 12 bytes, this MIGHT tear instructions?
NTSTATUS kmemcpy(VOID UNALIGNED* Destination, CONST VOID UNALIGNED* Source, ULONG Length)
{
    const KIRQL Irql = KeRaiseIrqlToDpcLevel();

    PMDL Mdl = IoAllocateMdl(Destination, Length, 0, 0, NULL);
    if (Mdl == NULL)
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
    const PVOID Mapped = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmCached, NULL, FALSE, HighPagePriority);
    if (Mapped == NULL)
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
    vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, argList);
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
uint32_t find_a_session1_process_pid()
{
    uint32_t bufSize = 4096;
    NTSTATUS status;

    void* procs = NULL;
    do
    {
        if (procs != NULL) ExFreePoolWithTag(procs, 'BOOM');
        procs = ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, 'BOOM');
        if (procs == NULL) return 0;
        status = loc_NtQuerySystemInformation(0x5, procs, bufSize, &bufSize);
    }
    while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(procs, 'BOOM');
        return 0;
    }

    for (SYSTEM_PROCESS_INFORMATION* p = procs;;)
    {
        // first SessionId 1 process (or other positive SessionId ig)
        if (p->SessionId != 0)
        {
            uint32_t pid = p->UniqueProcessId;
            print("find_session1_process_pid: FOUND! %wZ, PID %d", &p->ImageName, (uint32_t)pid);
            ExFreePoolWithTag(procs, 'BOOM');
            return pid;
        }

        if (!p->NextEntryOffset) break;
        p = (SYSTEM_PROCESS_INFORMATION*)((UCHAR*)p + p->NextEntryOffset);
    }

    // cleanup
    ExFreePoolWithTag(procs, 'BOOM');
    return 0;
}
