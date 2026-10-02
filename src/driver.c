#include <hooks.h>
#include <ntddk.h>
#include <stdint.h>
#include <utils.h>
#include <var.h>
#include <wdf.h>

void driver_unload(PDRIVER_OBJECT DriverObject)
{
    //! YOU CANNOT UNLOAD THE DRIVER. THE HOOKS WILL POINT TO GARBAGE!
    print("UNLOAD UNSUPPORTED!!!!!1s\n");
}

NTSTATUS on_device_added(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    // we dont use devices here
    print("on_device_added\n");
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    print("hello from kernel\n");

    // DriverObject->DriverUnload = driver_unload;

    print("init_globals()\n");
    if (init_globals() == 0)
    {
        print("init_globals failed\n");
        return STATUS_SUCCESS;
    }

    print("hook_all()\n");
    if (hook_all() == 0)
    {
        print("hook_all failed\n");
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
    // WDF_DRIVER_CONFIG config;
    // WDF_DRIVER_CONFIG_INIT(&config, on_device_added);

    // NTSTATUS status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

    // return status;
}