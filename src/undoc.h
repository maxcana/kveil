#pragma once
#include <aux_klib.h>
#include <ntddk.h>
#include <sal.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>

NTKERNELAPI PVOID NTAPI RtlPcToFileHeader(_In_ PVOID PcValue, _Out_ PVOID* BaseOfImage);

// original from decompilation:
// NTSTATUS __fastcall ExpQuerySystemInformation(int a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, unsigned int a3, _QWORD* a4, unsigned
// int Length, ULONG* a6)

// note: __fastcall and __stdcall etc does absolutely nothing on MSVC x64 builds
typedef NTSTATUS ExpQuerySystemInformation_t(int32_t a1, struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* a2, uint32_t a3, uint64_t* a4, uint32_t Length, uint32_t* a6);

// original from decompilation; wait, it literally just adds 584 to it. but idk of this changes depending on windows version:
// __int64 __fastcall KeQueryPrimaryGroupThread(__int64 a1)
// {
//   return *(unsigned __int16 *)(a1 + 584);
// }
typedef USHORT KeQueryPrimaryGroupThread_t(int64_t a1);

/**
 * The NtQuerySystemInformation routine queries information about the system.
 *
 * \param SystemInformationClass The type of information to be retrieved.
 * \param SystemInformation A pointer to a buffer that receives the requested information.
 * \param SystemInformationLength The size of the buffer pointed to by SystemInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/sysinfo/zwquerysysteminformation
 */
typedef NTSTATUS NtQuerySystemInformation_t(int32_t SystemInformationClass, void* SystemInformation, uint32_t SystemInformationLength, uint32_t* ReturnLength);

// thx ntdoc
// dont need this but i need it for other definitions below
typedef enum _KTHREAD_STATE
{
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
    GateWaitObsolete,
    WaitingForProcessInSwap,
    MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;

// also dont need this
// The SYSTEM_THREAD_INFORMATION structure contains information about a thread running on a system.
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/e82d73e4-cedb-4077-9099-d58f3459722f
typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;  // Number of 100-nanosecond intervals spent executing kernel code.
    LARGE_INTEGER UserTime;    // Number of 100-nanosecond intervals spent executing user code.
    LARGE_INTEGER CreateTime;  // The date and time when the thread was created.
    ULONG WaitTime;            // The current time spent in ready queue or waiting (depending on the thread state).
    PVOID StartAddress;        // The initial start address of the thread.
    CLIENT_ID ClientId;        // The identifier of the thread and the process owning the thread.
    KPRIORITY Priority;        // The dynamic priority of the thread.
    KPRIORITY BasePriority;    // The starting priority of the thread.
    ULONG ContextSwitches;     // The total number of context switches performed.
    KTHREAD_STATE ThreadState; // The current state of the thread.
    KWAIT_REASON WaitReason;   // The current reason the thread is waiting.
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

// The SYSTEM_PROCESS_INFORMATION structure contains information about a process running on a system.
// source: https://ntdoc.m417z.com/system_process_information
_Struct_size_bytes_(NextEntryOffset) typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;                // The address of the previous item plus the value in the NextEntryOffset member. For the last item in the array, NextEntryOffset is 0.
    ULONG NumberOfThreads;                // The NumberOfThreads member contains the number of threads in the process.
    ULONGLONG WorkingSetPrivateSize;      // The total private memory that a process currently has allocated and is physically resident in memory. // since VISTA
    ULONG HardFaultCount;                 // The total number of hard faults for data from disk rather than from in-memory pages. // since WIN7
    ULONG NumberOfThreadsHighWatermark;   // The peak number of threads that were running at any given point in time, indicative of potential performance bottlenecks related to
                                          // thread management.
    ULONGLONG CycleTime;                  // The sum of the cycle time of all threads in the process.
    LARGE_INTEGER CreateTime;             // Number of 100-nanosecond intervals since the creation time of the process. Not updated during system timezone changes.
    LARGE_INTEGER UserTime;               // Number of 100-nanosecond intervals the process has executed in user mode.
    LARGE_INTEGER KernelTime;             // Number of 100-nanosecond intervals the process has executed in kernel mode.
    UNICODE_STRING ImageName;             // The file name of the executable image.
    KPRIORITY BasePriority;               // The starting priority of the process.
    HANDLE UniqueProcessId;               // The identifier of the process.
    HANDLE InheritedFromUniqueProcessId;  // The identifier of the process that created this process. Not updated and incorrectly refers to processes with recycled identifiers.
    ULONG HandleCount;                    // The current number of open handles used by the process.
    ULONG SessionId;                      // The identifier of the Remote Desktop Services session under which the specified process is running.
    ULONG_PTR UniqueProcessKey;           // since VISTA (requires SystemExtendedProcessInformation)
    SIZE_T PeakVirtualSize;               // The peak size, in bytes, of the virtual memory used by the process.
    SIZE_T VirtualSize;                   // The current size, in bytes, of virtual memory used by the process.
    ULONG PageFaultCount;                 // The total number of page faults for data that is not currently in memory. The value wraps around to zero on average 24 hours.
    SIZE_T PeakWorkingSetSize;            // The peak size, in kilobytes, of the working set of the process.
    SIZE_T WorkingSetSize;                // The number of pages visible to the process in physical memory. These pages are resident and available for use without triggering a page fault.
    SIZE_T QuotaPeakPagedPoolUsage;       // The peak quota charged to the process for pool usage, in bytes.
    SIZE_T QuotaPagedPoolUsage;           // The quota charged to the process for paged pool usage, in bytes.
    SIZE_T QuotaPeakNonPagedPoolUsage;    // The peak quota charged to the process for nonpaged pool usage, in bytes.
    SIZE_T QuotaNonPagedPoolUsage;        // The current quota charged to the process for nonpaged pool usage.
    SIZE_T PagefileUsage;                 // The total number of bytes of page file storage in use by the process.
    SIZE_T PeakPagefileUsage;             // The maximum number of bytes of page-file storage used by the process.
    SIZE_T PrivatePageCount;              // The number of memory pages allocated for the use by the process.
    LARGE_INTEGER ReadOperationCount;     // The total number of read operations performed.
    LARGE_INTEGER WriteOperationCount;    // The total number of write operations performed.
    LARGE_INTEGER OtherOperationCount;    // The total number of I/O operations performed other than read and write operations.
    LARGE_INTEGER ReadTransferCount;      // The total number of bytes read during a read operation.
    LARGE_INTEGER WriteTransferCount;     // The total number of bytes written during a write operation.
    LARGE_INTEGER OtherTransferCount;     // The total number of bytes transferred during operations other than read and write operations.
    SYSTEM_THREAD_INFORMATION Threads[1]; // This type is not defined in the structure but was added for convenience.
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

// The SYSTEM_SESSION_PROCESS_INFORMATION structure contains system session process information.
// courtesy of: https://ntdoc.m417z.com/system_session_process_information
typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG BufferSize;
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION, *PSYSTEM_SESSION_PROCESS_INFORMATION;

// The SYSTEM_BASICPROCESS_INFORMATION structure describes basic process information returned when enumerating processes.
// special thanks: https://ntdoc.m417z.com/system_basicprocess_information
_Struct_size_bytes_(NextEntryOffset) typedef struct _SYSTEM_BASICPROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG64 SequenceNumber;
    UNICODE_STRING ImageName;
} SYSTEM_BASICPROCESS_INFORMATION, *PSYSTEM_BASICPROCESS_INFORMATION;

// The SYSTEM_CODEINTEGRITY_INFORMATION structure contains the current Code Integrity policy options.
// i think this one is pretty well-known
typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

// Module information
typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    PVOID Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    _Field_size_(NumberOfModules) RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;