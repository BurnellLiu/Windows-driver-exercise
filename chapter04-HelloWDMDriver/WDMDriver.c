
#include <wdm.h>


/// @brief �豸��չ�ṹ
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject; ///< �豸����
    PDEVICE_OBJECT PNextStackDevice; ///< �²��豸����ָ��
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/// @brief ��PNP IRP����Ĭ�ϴ���
/// @param[in] pDeviceExt �豸������չ
/// @param[in] pIrp I/O�����
/// @return ״̬
NTSTATUS PnpDefaultHandler(PDEVICE_EXTENSION pDeviceExt, PIRP pIrp)
{
    KdPrint(("Enter DefaultPnpHandler\n"));

    // �Թ���ǰ��ջ
    IoSkipCurrentIrpStackLocation(pIrp);

    KdPrint(("Leave DefaultPnpHandler\n"));

    // ���²��ջ�������豸�����IRP
    return IoCallDriver(pDeviceExt->PNextStackDevice, pIrp);
}
  
/// @brief PNP�Ƴ��豸������
/// @param[in] pDeviceExt �豸��չ����
/// @param[in] pIrp �����
/// @return ״̬
NTSTATUS PnpRemoveDevice(PDEVICE_EXTENSION pDeviceExt, PIRP pIrp)
{
    KdPrint(("Enter HandleRemoveDevice\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    NTSTATUS status = PnpDefaultHandler(pDeviceExt, pIrp);

    //����IoDetachDevice()���豸������豸ջ���ѿ���  
    if (pDeviceExt->PNextStackDevice != NULL)
        IoDetachDevice(pDeviceExt->PNextStackDevice);

    //ɾ���豸����  
    IoDeleteDevice(pDeviceExt->PDeviceObject);

    KdPrint(("Leave HandleRemoveDevice\n"));

    return status;
}

/// @brief ������豸
/// @param[in] pDriverObject ��I/O����������������������
/// @param[in] pPhysicalDeviceObject ��I/O�������������������豸����
/// @return ������豸״̬
NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
{
    KdPrint(("Enter HelloWDMAddDevice\n"));

    NTSTATUS status = STATUS_SUCCESS;

    PDEVICE_OBJECT pDeviceObject = NULL; // �������豸����
    PDEVICE_EXTENSION pDeviceExt = NULL; // �豸��չ����

    // �����豸
    status = IoCreateDevice(
        pDriverObject,
        sizeof(DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("Create Decive Fail\n"));
        return status;
    }

    pDeviceExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
    pDeviceExt->PDeviceObject = pDeviceObject;

    // ���豸����ҽ����豸��ջ��
    pDeviceExt->PNextStackDevice = IoAttachDeviceToDeviceStack(pDeviceObject, pPhysicalDeviceObject);

   
    pDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    KdPrint(("Leave HelloWDMAddDevice\n"));

    return status;
}

/// @brief �Լ��弴��IPR���д���
/// @param[in] pDeviceObject �����豸����
/// @param[in] pIrp �����
/// @return ״̬
NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
    KdPrint(("Enter HelloWDMPnp\n"));

    PDEVICE_EXTENSION pDeviceExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION pStackLoc = IoGetCurrentIrpStackLocation(pIrp);
    unsigned long func = pStackLoc->MinorFunction;

    KdPrint(("PNP Request (%u)\n", func));

    NTSTATUS status = STATUS_SUCCESS;
    switch (func)
    {
    case IRP_MN_REMOVE_DEVICE:
        status = PnpRemoveDevice(pDeviceExt, pIrp);
        break;
    default:
        status = PnpDefaultHandler(pDeviceExt, pIrp);
        break;
    }

    KdPrint(("Leave HelloWDMPnp\n"));

    return status;
}


/// @brief ��Ĭ��IPR���д���
NTSTATUS HelloWDMDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    KdPrint(("Enter HelloWDMDispatchRoutine\n"));
   
    NTSTATUS status = STATUS_SUCCESS;

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave WDMDriverDispatchRoutine\n"));
    return STATUS_SUCCESS;
}

/// @brief ��������ж�ز���
void HelloWDMUnload(IN PDRIVER_OBJECT pDriverObject)
{
    UNREFERENCED_PARAMETER(pDriverObject);

    KdPrint(("Enter HelloWDMUnload\n"));
    KdPrint(("Leave HelloWDMUnload\n"));
}


/// @brief ��ʼ����������
/// @param[in] pDriverObject ��������
/// @param[in] pRegPath ����������ע����е�·��
/// @return ��ʼ������״̬
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath)
{
    UNREFERENCED_PARAMETER(pRegPath);
    KdPrint(("Enter DriverEntry\n"));

    pDriverObject->DriverExtension->AddDevice = HelloWDMAddDevice;
    pDriverObject->DriverUnload = HelloWDMUnload;

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        pDriverObject->MajorFunction[i] = HelloWDMDispatchRoutine;
    }

    pDriverObject->MajorFunction[IRP_MJ_PNP] = HelloWDMPnp;

    KdPrint(("Leave DriverEntry\n"));

    return STATUS_SUCCESS;
}