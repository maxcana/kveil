#include <ntddk.h>
#include <wdf.h>

void print(PCSTR format) {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	NTSTATUS status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG config;
	print("hello from kernel\n");
}