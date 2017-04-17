

#include <ntddk.h>

/// @brief 设备扩展结构
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject;
    UNICODE_STRING DeviceName; ///< 设备名称
    UNICODE_STRING SymLinkName; ///< 符号链接名

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/// @brief 对IRP进行处理
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

/// @brief 创建设备对象
/// @param[in] pDriverObject 驱动对象
/// @return 状态值
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING devName;
    UNICODE_STRING symLinkName;
    PDEVICE_OBJECT pDevObj = NULL;
    PDEVICE_EXTENSION pDevExt = NULL;

    NTSTATUS status;

    // 创建设备名称
    RtlInitUnicodeString(&devName, L"\\Device\\HelloNTDriverDevice");

    // 创建设备
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


    // 创建符号链接
    // 设备名称只在内核态中可见
    // 符号链接, 链接应用程序和设备名称
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