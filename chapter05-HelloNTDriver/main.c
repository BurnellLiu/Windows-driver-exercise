

#include <ntddk.h>

/// @brief PsGetProcessImageFileName���Ի�ý�������΢��û�й鵵���������ˣ�����һ�¾Ϳ���ʹ��
/// @param[in] pEProcess ����ָ��
/// @return ������
NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS pEProcess);

/// @brief �豸��չ�ṹ
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject;
    UNICODE_STRING DeviceName; ///< �豸����
    UNICODE_STRING SymLinkName; ///< ����������

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/// @brief ��ʾ��ǰ��������
void DisplayProcessName()
{
    PEPROCESS pEProcess = PsGetCurrentProcess();

    PTSTR processName = (PTSTR)PsGetProcessImageFileName(pEProcess);
    KdPrint(("Process Name: %s\n", processName));
}

/// @brief ��IRP���д���
/// @param[in] pDriverObject
/// @param[in] pIrp
/// @return
NTSTATUS HelloNTDriverDispatchRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    KdPrint(("Enter HelloNTDriverDispatchRoutine\n"));
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pDevObject);
    DisplayProcessName();

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloNTDriverDispatchRoutine\n"));

    return status;
}

/// @brief �����豸����
/// @param[in] pDriverObject ��������
/// @return ״ֵ̬
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING devName1;
    UNICODE_STRING devName2;
    UNICODE_STRING symLinkName1;
    UNICODE_STRING symLinkName2;
    PDEVICE_OBJECT pDevObj1 = NULL;
    PDEVICE_OBJECT pDevObj2 = NULL;
    PDEVICE_EXTENSION pDevExt1 = NULL;
    PDEVICE_EXTENSION pDevExt2 = NULL;
    NTSTATUS status;

    // �����豸����1
    RtlInitUnicodeString(&devName1, L"\\Device\\HelloNTDriverDevice1");
    status = IoCreateDevice(
        pDriverObject,
        sizeof(DEVICE_EXTENSION),
        &devName1,
        FILE_DEVICE_UNKNOWN,
        0,
        TRUE,
        &pDevObj1);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    pDevObj1->Flags |= DO_BUFFERED_IO;
    pDevExt1 = (PDEVICE_EXTENSION)pDevObj1->DeviceExtension;
    pDevExt1->PDeviceObject = pDevObj1;
    pDevExt1->DeviceName = devName1;

    RtlInitUnicodeString(&symLinkName1, L"\\??\\HelloNTDriver1");
    pDevExt1->SymLinkName = symLinkName1;
    status = IoCreateSymbolicLink(&symLinkName1, &devName1);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(pDevObj1);
        return status;
    }

    
    // �����豸����2
    RtlInitUnicodeString(&devName2, L"\\Device\\HelloNTDriverDevice2");
    status = IoCreateDevice(
        pDriverObject,
        sizeof(DEVICE_EXTENSION),
        &devName2,
        FILE_DEVICE_UNKNOWN,
        0,
        TRUE,
        &pDevObj2);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    pDevObj2->Flags |= DO_BUFFERED_IO;
    pDevExt2 = (PDEVICE_EXTENSION)pDevObj2->DeviceExtension;
    pDevExt2->PDeviceObject = pDevObj2;
    pDevExt2->DeviceName = devName2;

    RtlInitUnicodeString(&symLinkName2, L"\\??\\HelloNTDriver2");
    pDevExt2->SymLinkName = symLinkName2;
    status = IoCreateSymbolicLink(&symLinkName2, &devName2);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(pDevObj2);
        return status;
    }


    return STATUS_SUCCESS;

}

// @brief ��������ж�ز���
/// @param[in] pDriverObject ��������
void HelloNTDriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
    PDEVICE_OBJECT pNextObj = NULL;
    KdPrint(("Enter HelloNTDriverUnload\n"));

    pNextObj = pDriverObject->DeviceObject;

    while (pNextObj != NULL)
    {
        PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

        // ɾ����������
        UNICODE_STRING linkName = pDevExt->SymLinkName;
        IoDeleteSymbolicLink(&linkName);
        pNextObj = pNextObj->NextDevice;

        // ɾ���豸����
        IoDeleteDevice(pDevExt->PDeviceObject);
    }

    KdPrint(("Leave HelloNTDriverUnload\n"));
}

/// @brief ����������ں���
/// @param[in] pDriverObject ��I/O�������д���������������
/// @param[in] pRegPath ����������ע����е�·��
/// @return ��ʼ������״̬
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath)
{
    NTSTATUS status = STATUS_SUCCESS;

    KdPrint(("Enter DriverEntry\n"));

    KdPrint(("DriverName: %ws\n", pDriverObject->DriverName.Buffer));
    KdPrint(("HardwareDatabase: %ws\n", pDriverObject->HardwareDatabase->Buffer));
    KdPrint(("RegPath: %ws\n", pRegPath->Buffer));

    DisplayProcessName();

    UNREFERENCED_PARAMETER(pRegPath);

    // ע���������ú������
    // ��Щ�������������������������, �����ɲ���ϵͳ�������
    pDriverObject->DriverUnload = HelloNTDriverUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_READ] = HelloNTDriverDispatchRoutine;

    // ���������豸����
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));

    return status;
}