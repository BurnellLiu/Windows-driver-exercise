

#include <ntddk.h>

/// @brief �豸��չ�ṹ
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject;
    UNICODE_STRING DeviceName; ///< �豸����
    UNICODE_STRING SymLinkName; ///< ����������

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/// @brief ��IRP���д���
/// @param[in] pDriverObject
/// @param[in] pIrp
/// @return
NTSTATUS HelloNTDriverDispatchRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    KdPrint(("Enter HelloNTDriverDispatchRoutine\n"));
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pDevObject);

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
    UNICODE_STRING devName;
    UNICODE_STRING symLinkName;
    PDEVICE_OBJECT pDevObj = NULL;
    PDEVICE_EXTENSION pDevExt = NULL;

    NTSTATUS status;

    // �����豸����
    RtlInitUnicodeString(&devName, L"\\Device\\HelloNTDriverDevice");

    // �����豸
    status = IoCreateDevice(
        pDriverObject,
        sizeof(DEVICE_EXTENSION),
        &devName,
        FILE_DEVICE_UNKNOWN,
        0,
        TRUE,
        &pDevObj);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    pDevObj->Flags |= DO_BUFFERED_IO;
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    pDevExt->PDeviceObject = pDevObj;
    pDevExt->DeviceName = devName;


    // ������������
    // �豸����ֻ���ں�̬�пɼ�
    // ��������, ����Ӧ�ó�����豸����
    RtlInitUnicodeString(&symLinkName, L"\\??\\HelloNTDriver");
    pDevExt->SymLinkName = symLinkName;
    status = IoCreateSymbolicLink(&symLinkName, &devName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(pDevObj);
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