#include <ntddk.h>
#include <utils.h>
#include <var.h>
#include <wdf.h>

NTSTATUS on_device_added(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    // we dont use devices here
    print("on_device_added\n");
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    print("hello from kernel\n");

    if (init_globals() == 0) return STATUS_UNSUCCESSFUL;

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, on_device_added);

    NTSTATUS status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

    return status;
}