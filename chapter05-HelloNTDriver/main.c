

#include <ntddk.h>

/// @brief PsGetProcessImageFileName可以获得进程名，微软没有归档，但导出了，声明一下就可以使用
/// @param[in] pEProcess 进程指针
/// @return 进程名
NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS pEProcess);

/// @brief 设备扩展结构
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject;
    UNICODE_STRING DeviceName; ///< 设备名称
    UNICODE_STRING SymLinkName; ///< 符号链接名

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/// @brief 显示当前进程名称
void DisplayProcessName()
{
    PEPROCESS pEProcess = PsGetCurrentProcess();

    PTSTR processName = (PTSTR)PsGetProcessImageFileName(pEProcess);
    KdPrint(("Process Name: %s\n", processName));
}

/// @brief 对IRP进行处理
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

/// @brief 创建设备对象
/// @param[in] pDriverObject 驱动对象
/// @return 状态值
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

    // 创建设备对象1
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

    
    // 创建设备对象2
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

// @brief 驱动程序卸载操作
/// @param[in] pDriverObject 驱动对象
void HelloNTDriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
    PDEVICE_OBJECT pNextObj = NULL;
    KdPrint(("Enter HelloNTDriverUnload\n"));

    pNextObj = pDriverObject->DeviceObject;

    while (pNextObj != NULL)
    {
        PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

        // 删除符号链接
        UNICODE_STRING linkName = pDevExt->SymLinkName;
        IoDeleteSymbolicLink(&linkName);
        pNextObj = pNextObj->NextDevice;

        // 删除设备对象
        IoDeleteDevice(pDevExt->PDeviceObject);
    }

    KdPrint(("Leave HelloNTDriverUnload\n"));
}

/// @brief 驱动程序入口函数
/// @param[in] pDriverObject 从I/O管理器中传进来的驱动对象
/// @param[in] pRegPath 驱动程序在注册表中的路径
/// @return 初始化驱动状态
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath)
{
    NTSTATUS status = STATUS_SUCCESS;

    KdPrint(("Enter DriverEntry\n"));

    KdPrint(("DriverName: %ws\n", pDriverObject->DriverName.Buffer));
    KdPrint(("HardwareDatabase: %ws\n", pDriverObject->HardwareDatabase->Buffer));
    KdPrint(("RegPath: %ws\n", pRegPath->Buffer));

    DisplayProcessName();

    UNREFERENCED_PARAMETER(pRegPath);

    // 注册驱动调用函数入口
    // 这些函数不是由驱动程序本身负责调用, 而是由操作系统负责调用
    pDriverObject->DriverUnload = HelloNTDriverUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloNTDriverDispatchRoutine;
    pDriverObject->MajorFunction[IRP_MJ_READ] = HelloNTDriverDispatchRoutine;

    // 创建驱动设备对象
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));

    return status;
}