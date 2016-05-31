#include "busenum.h"
#include <wdmsec.h>
#include <usbioctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Bus_CreatePdo)
#pragma alloc_text(PAGE, Bus_EvtDeviceListCreatePdo)
#pragma alloc_text(PAGE, Bus_EvtDevicePrepareHardware)
#endif

NTSTATUS Bus_EvtDeviceListCreatePdo(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    PWDFDEVICE_INIT ChildInit)
{
    PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    PAGED_CODE();

    pDesc = CONTAINING_RECORD(IdentificationDescription, PDO_IDENTIFICATION_DESCRIPTION, Header);

    return Bus_CreatePdo(WdfChildListGetDevice(DeviceList), ChildInit, pDesc->SerialNo, pDesc->TargetType, pDesc->OwnerProcessId);
}

BOOLEAN Bus_EvtChildListIdentificationDescriptionCompare(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription)
{
    PPDO_IDENTIFICATION_DESCRIPTION lhs, rhs;

    UNREFERENCED_PARAMETER(DeviceList);

    lhs = CONTAINING_RECORD(FirstIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);
    rhs = CONTAINING_RECORD(SecondIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);

    return (lhs->SerialNo == rhs->SerialNo) ? TRUE : FALSE;
}

NTSTATUS Bus_CreatePdo(
    _In_ WDFDEVICE Device,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_ ULONG SerialNo,
    _In_ VIGEM_TARGET_TYPE TargetType,
    _In_ DWORD OwnerProcessId)
{
    NTSTATUS status;
    PPDO_DEVICE_DATA pdoData;
    WDFDEVICE hChild = NULL;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES powerCaps;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES pdoAttributes;
    WDF_IO_QUEUE_CONFIG  defaultPdoQueueConfig;
    WDFQUEUE defaultPdoQueue;
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"Virtual Gamepad Emulation Bus");
    DECLARE_UNICODE_STRING_SIZE(buffer, MAX_INSTANCE_ID_LEN);
    // reserve space for device id
    DECLARE_UNICODE_STRING_SIZE(deviceId, MAX_INSTANCE_ID_LEN);
    // reserve space for device description
    DECLARE_UNICODE_STRING_SIZE(deviceDescription, MAX_DEVICE_DESCRIPTION_LEN);


    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);

    KdPrint(("Entered Bus_CreatePdo\n"));

    // set device type
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

    // enter RAW device mode
    {
        status = WdfPdoInitAssignRawDevice(DeviceInit, &GUID_DEVCLASS_VIGEM_RAWPDO);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfPdoInitAssignRawDevice failed status 0x%x\n", status));
            return status;
        }

        WdfDeviceInitSetCharacteristics(DeviceInit, FILE_AUTOGENERATED_DEVICE_NAME, TRUE);

        status = WdfDeviceInitAssignSDDLString(DeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfDeviceInitAssignSDDLString failed status 0x%x\n", status));
            return status;
        }
    }

    // set parameters matching desired target device
    switch (TargetType)
    {
        //
        // A Xbox 360 device was requested
        // 
    case Xbox360Wired:
    {
        // set device description
        {
            // prepare device description
            status = RtlUnicodeStringPrintf(&deviceDescription, L"Virtual Xbox 360 Controller");
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        // set hardware ids
        {
            RtlUnicodeStringPrintf(&buffer, L"USB\\VID_045E&PID_028E&REV_0114");

            RtlUnicodeStringCopy(&deviceId, &buffer);

            status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            RtlUnicodeStringPrintf(&buffer, L"USB\\VID_045E&PID_028E");

            status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        // set compatible ids
        {
            RtlUnicodeStringPrintf(&buffer, L"USB\\MS_COMP_XUSB10");

            status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            RtlUnicodeStringPrintf(&buffer, L"USB\\Class_FF&SubClass_5D&Prot_01");

            status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            RtlUnicodeStringPrintf(&buffer, L"USB\\Class_FF&SubClass_5D");

            status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            RtlUnicodeStringPrintf(&buffer, L"USB\\Class_FF");

            status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }
    break;
    //
    // A Sony DualShock 4 device was requested
    // 
    case DualShock4Wired:
    {
        // set device description
        {
            // prepare device description
            status = RtlUnicodeStringPrintf(&deviceDescription, L"Virtual DualShock 4 Controller");
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        // set hardware ids
        {
            RtlUnicodeStringPrintf(&buffer, L"USB\\VID_054C&PID_05C4&REV_0100");

            RtlUnicodeStringCopy(&deviceId, &buffer);

            status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            RtlUnicodeStringPrintf(&buffer, L"USB\\VID_054C&PID_05C4");

            status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }
    break;
    }

    // set device id
    status = WdfPdoInitAssignDeviceID(DeviceInit, &deviceId);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // prepare instance id
    status = RtlUnicodeStringPrintf(&buffer, L"%02d", SerialNo);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // set instance id
    status = WdfPdoInitAssignInstanceID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // set device description (for English operating systems)
    status = WdfPdoInitAddDeviceText(DeviceInit, &deviceDescription, &deviceLocation, 0x409);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // default locale is English
    WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

    // PNP/Power event callbacks
    {
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

        pnpPowerCallbacks.EvtDevicePrepareHardware = Bus_EvtDevicePrepareHardware;

        WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);
    }

    WdfPdoInitAllowForwardingRequestToParent(DeviceInit);

    // Create PDO
    {
        // Add common device data context
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, PDO_DEVICE_DATA);

        status = WdfDeviceCreate(&DeviceInit, &pdoAttributes, &hChild);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // Add XUSB-specific device data context
        if (TargetType == Xbox360Wired)
        {
            PXUSB_DEVICE_DATA xusbData = NULL;
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, XUSB_DEVICE_DATA);

            status = WdfObjectAllocateContext(hChild, &pdoAttributes, (PVOID)&xusbData);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("WdfObjectAllocateContext failed status 0x%x\n", status));
                return status;
            }
        }
    }

    // Expose USB interface
    {
        status = WdfDeviceCreateDeviceInterface(Device, (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE, NULL);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfDeviceCreateDeviceInterface failed status 0x%x\n", status));
            return status;
        }
    }

    // Set PDO properties
    {
        pdoData = PdoGetData(hChild);

        pdoData->SerialNo = SerialNo;
        pdoData->TargetType = TargetType;
        pdoData->OwnerProcessId = OwnerProcessId;

        // Initialize additional contexts (if available)
        PXUSB_DEVICE_DATA xusb = XusbGetData(hChild);
        if (xusb != NULL)
        {
            RtlZeroMemory(xusb, sizeof(XUSB_DEVICE_DATA));

            xusb->LedNumber = (UCHAR)SerialNo;

            // I/O Queue for pending IRPs
            WDF_IO_QUEUE_CONFIG pendingUsbQueueConfig, notificationsQueueConfig;
            WDFQUEUE pendingUsbQueue, notificationsQueue;

            WDF_IO_QUEUE_CONFIG_INIT(&pendingUsbQueueConfig, WdfIoQueueDispatchManual);

            status = WdfIoQueueCreate(hChild, &pendingUsbQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pendingUsbQueue);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
                return status;
            }

            xusb->PendingUsbRequests = pendingUsbQueue;

            WDF_IO_QUEUE_CONFIG_INIT(&notificationsQueueConfig, WdfIoQueueDispatchManual);

            status = WdfIoQueueCreate(hChild, &notificationsQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &notificationsQueue);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
                return status;
            }

            xusb->PendingNotificationRequests = notificationsQueue;

            // Reset report buffer
            RtlZeroMemory(xusb->Report, XUSB_REPORT_SIZE);
        }
    }

    // Default I/O queue setup
    {
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&defaultPdoQueueConfig, WdfIoQueueDispatchParallel);

        defaultPdoQueueConfig.EvtIoInternalDeviceControl = Pdo_EvtIoInternalDeviceControl;

        status = WdfIoQueueCreate(hChild, &defaultPdoQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &defaultPdoQueue);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
            return status;
        }
    }

    // PNP capabilities
    {
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);

        pnpCaps.Removable = WdfTrue;
        pnpCaps.EjectSupported = WdfTrue;
        pnpCaps.SurpriseRemovalOK = WdfTrue;

        pnpCaps.Address = SerialNo;
        pnpCaps.UINumber = SerialNo;

        WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);
    }

    // Power capabilities
    {
        WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);

        powerCaps.DeviceD1 = WdfTrue;
        powerCaps.WakeFromD1 = WdfTrue;
        powerCaps.DeviceWake = PowerDeviceD1;

        powerCaps.DeviceState[PowerSystemWorking] = PowerDeviceD0;
        powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
        powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemShutdown] = PowerDeviceD3;

        WdfDeviceSetPowerCapabilities(hChild, &powerCaps);
    }

    return status;
}

//
// Exposes necessary interfaces on PDO power-up.
// 
NTSTATUS Bus_EvtDevicePrepareHardware(
    _In_ WDFDEVICE    Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    PPDO_DEVICE_DATA pdoData;
    WDF_QUERY_INTERFACE_CONFIG ifaceCfg;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    KdPrint(("Bus_EvtDevicePrepareHardware: 0x%p\n", Device));

    pdoData = PdoGetData(Device);

    // Expose XUSB interfaces
    if (pdoData->TargetType == Xbox360Wired)
    {
        INTERFACE dummyIface;

        dummyIface.Size = sizeof(INTERFACE);
        dummyIface.Version = 1;
        dummyIface.Context = (PVOID)Device;

        dummyIface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        dummyIface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        //
        // Dummy 0
        // 
        WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_0, NULL);

        status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data1,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data2,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data3,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[0],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[1],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[2],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[3],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[4],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[5],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[6],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[7],
                status));

            return status;
        }

        //
        // Dummy 1
        // 
        WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_1, NULL);

        status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data1,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data2,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data3,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[0],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[1],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[2],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[3],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[4],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[5],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[6],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[7],
                status));

            return status;
        }

        //
        // Dummy 2
        // 
        WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_2, NULL);

        status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data1,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data2,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data3,
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[0],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[1],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[2],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[3],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[4],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[5],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[6],
                GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[7],
                status));

            return status;
        }

        USB_BUS_INTERFACE_USBDI_V1 xusbInterface;

        xusbInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
        xusbInterface.Version = USB_BUSIF_USBDI_VERSION_1;
        xusbInterface.BusContext = (PVOID)Device;

        xusbInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        xusbInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        xusbInterface.SubmitIsoOutUrb = UsbPdo_SubmitIsoOutUrb;
        xusbInterface.GetUSBDIVersion = UsbPdo_GetUSBDIVersion;
        xusbInterface.QueryBusTime = UsbPdo_QueryBusTime;
        xusbInterface.QueryBusInformation = UsbPdo_QueryBusInformation;
        xusbInterface.IsDeviceHighSpeed = UsbPdo_IsDeviceHighSpeed;

        WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&xusbInterface, &USB_BUS_INTERFACE_USBDI_GUID, NULL);

        status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfDeviceAddQueryInterface failed status 0x%x\n", status));
            return status;
        }
    }

    return status;
}

//
// Responds to IRP_MJ_INTERNAL_DEVICE_CONTROL requests sent to PDO.
// 
VOID Pdo_EvtIoInternalDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    // Regular buffers not used in USB communication
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_INVALID_PARAMETER;
    WDFDEVICE hDevice;
    PIRP irp;
    PURB urb;
    PPDO_DEVICE_DATA pdoData;
    PIO_STACK_LOCATION irpStack;

    hDevice = WdfIoQueueGetDevice(Queue);

    KdPrint(("Pdo_EvtIoInternalDeviceControl: 0x%p\n", hDevice));

    pdoData = PdoGetData(hDevice);

    KdPrint(("Pdo_EvtIoInternalDeviceControl PDO: 0x%p\n", pdoData));

    // No help from the framework available from here on
    irp = WdfRequestWdmGetIrp(Request);
    irpStack = IoGetCurrentIrpStackLocation(irp);

    switch (IoControlCode)
    {
    case IOCTL_INTERNAL_USB_SUBMIT_URB:

        KdPrint((">> IOCTL_INTERNAL_USB_SUBMIT_URB\n"));

        urb = (PURB)URB_FROM_IRP(irp);

        switch (urb->UrbHeader.Function)
        {
        case URB_FUNCTION_CONTROL_TRANSFER:

            KdPrint((">> >> URB_FUNCTION_CONTROL_TRANSFER\n"));

            // Control transfer can safely be ignored
            status = STATUS_UNSUCCESSFUL;

            break;

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:

            KdPrint((">> >> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER\n"));

            status = UsbPdo_BulkOrInterruptTransfer(urb, hDevice, Request);

            break;

        case URB_FUNCTION_SELECT_CONFIGURATION:

            KdPrint((">> >> URB_FUNCTION_SELECT_CONFIGURATION\n"));

            status = UsbPdo_SelectConfiguration(urb);

            break;

        case URB_FUNCTION_SELECT_INTERFACE:

            KdPrint((">> >> URB_FUNCTION_SELECT_INTERFACE\n"));

            status = UsbPdo_SelectInterface(urb);

            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:

            KdPrint((">> >> URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n"));

            switch (urb->UrbControlDescriptorRequest.DescriptorType)
            {
            case USB_DEVICE_DESCRIPTOR_TYPE:

                KdPrint((">> >> >> USB_DEVICE_DESCRIPTOR_TYPE\n"));

                status = UsbPdo_GetDeviceDescriptorType(urb);

                break;

            case USB_CONFIGURATION_DESCRIPTOR_TYPE:

                KdPrint((">> >> >> USB_CONFIGURATION_DESCRIPTOR_TYPE\n"));

                status = UsbPdo_GetConfigurationDescriptorType(urb);

                break;

            case USB_STRING_DESCRIPTOR_TYPE:

                KdPrint((">> >> >> USB_STRING_DESCRIPTOR_TYPE\n"));

                break;

            case USB_INTERFACE_DESCRIPTOR_TYPE:

                KdPrint((">> >> >> USB_INTERFACE_DESCRIPTOR_TYPE\n"));

                break;

            case USB_ENDPOINT_DESCRIPTOR_TYPE:

                KdPrint((">> >> >> USB_ENDPOINT_DESCRIPTOR_TYPE\n"));

                break;

            default:
                KdPrint((">> >> >> Unknown descriptor type\n"));
                break;
            }

            KdPrint(("<< <<\n"));

            break;

        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:

            KdPrint((">> >> URB_FUNCTION_GET_STATUS_FROM_DEVICE\n"));

            // Defaults always succeed
            status = STATUS_SUCCESS;

            break;

        case URB_FUNCTION_ABORT_PIPE:

            KdPrint((">> >> URB_FUNCTION_ABORT_PIPE\n"));

            status = UsbPdo_AbortPipe(hDevice);

            break;

        default:
            KdPrint((">> >> Unknown function: 0x%X\n", urb->UrbHeader.Function));
            break;
        }

        KdPrint(("<<\n"));

        break;

    case IOCTL_INTERNAL_USB_GET_PORT_STATUS:

        KdPrint((">> IOCTL_INTERNAL_USB_GET_PORT_STATUS\n"));

        // We report the (virtual) port as always active
        *(unsigned long *)irpStack->Parameters.Others.Argument1 = USBD_PORT_ENABLED | USBD_PORT_CONNECTED;

        status = STATUS_SUCCESS;

        break;

    case IOCTL_INTERNAL_USB_RESET_PORT:

        KdPrint((">> IOCTL_INTERNAL_USB_RESET_PORT\n"));

        // Sure, why not ;)
        status = STATUS_SUCCESS;

        break;

    default:
        KdPrint((">> Unknown I/O control code\n"));
        break;
    }

    if (status != STATUS_PENDING)
    {
        WdfRequestComplete(Request, status);
    }
}

