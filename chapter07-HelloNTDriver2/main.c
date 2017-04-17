

#include <ntddk.h>

#define MAX_FILE_LENGTH 1024

#define IOCTL_TEST2 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

/// @brief 设备扩展结构
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject;
    UNICODE_STRING DeviceName; ///< 设备名称
    UNICODE_STRING SymLinkName; ///< 符号链接名

    unsigned char* PFileBuffer; ///< 模拟的文件缓冲区
    ULONG FileLength; ///< 文件长度

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/// @brief 设备IoControl例程
/// @param[in] pDevObject
/// @param[in] pIrp
/// @return
NTSTATUS DeviceIoControlRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    KdPrint(("\nEnter DeviceIOControlRoutine\n"));
    UNREFERENCED_PARAMETER(pDevObject);

    PIO_STACK_LOCATION pIOStack = IoGetCurrentIrpStackLocation(pIrp);

    // 得到输入输出缓冲区大小
    ULONG inputBufferLen = pIOStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLen = pIOStack->Parameters.DeviceIoControl.OutputBufferLength;

    // 得到IOCTL码
    ULONG ctlCode = pIOStack->Parameters.DeviceIoControl.IoControlCode;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG infor = 0;
    switch (ctlCode)
    {
    case IOCTL_TEST2:
    {
        KdPrint(("IOCTL_TEST2\n"));
        
        UCHAR* inputBuffer = (UCHAR*)pIrp->AssociatedIrp.SystemBuffer;
        UCHAR* outPutBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
        for (ULONG i = 0; i < inputBufferLen; i++)
        {
            KdPrint(("%X  ", inputBuffer[i]));
        }

        memset(outPutBuffer, 0xEF, outputBufferLen);

        infor = outputBufferLen;
        status = STATUS_SUCCESS;
        break;
    }
    default:
        status = STATUS_INVALID_VARIANT;
        infor = 0;
        break;
    }

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = infor;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave DeviceIOControlRoutine\n"));

    return status;

}

/// @brief 直接写例程
/// @param[in] pDevObject
/// @param[in] pIrp
/// @return
NTSTATUS DirectWriteRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    KdPrint(("\nEnter DirectWriteRoutine\n"));

    NTSTATUS status = STATUS_SUCCESS;

    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObject->DeviceExtension;
    PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);

    // 获取要写入的字节数以及偏移
    ULONG writeLength = pIoStack->Parameters.Write.Length;
    ULONG writeOffset = (ULONG)pIoStack->Parameters.Write.ByteOffset.QuadPart;
    ULONG completeLength = 0;

    if ((writeLength + writeOffset) > MAX_FILE_LENGTH)
    {
        status = STATUS_FILE_INVALID;
        completeLength = 0;
        goto SAFE_EXIT;
    }

    // 获取MDL地址
    PMDL pMdl = pIrp->MdlAddress;
    PVOID mdlAddress = (PVOID)((PCHAR)(pMdl->StartVa) + pMdl->ByteOffset);
    ULONG mdlLength = pMdl->ByteCount;
    if (mdlLength < writeLength)
    {
        status = STATUS_FILE_INVALID;
        completeLength = 0;
        goto SAFE_EXIT;
    }

    // 写入数据
    memcpy(pDevExt->PFileBuffer + writeOffset, mdlAddress, writeLength);
    if ((writeLength + writeOffset) > pDevExt->FileLength)
    {
        pDevExt->FileLength = writeOffset + writeLength;
    }
    status = STATUS_SUCCESS;
    completeLength = writeLength;

SAFE_EXIT:

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = completeLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave DirectWriteRoutine\n"));
    return status;
}

/// @brief 直接读例程
/// @param[in] pDevObject
/// @param[in] pIrp
/// @return
NTSTATUS DirectReadRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    KdPrint(("\nEnter DirectReadRoutine\n"));

    NTSTATUS status = STATUS_SUCCESS;

    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObject->DeviceExtension;

    // 获取需要读设备的字节数和偏移
    ULONG readLength = pStack->Parameters.Read.Length;
    ULONG readOffset = (ULONG)pStack->Parameters.Read.ByteOffset.QuadPart;
    ULONG completeLength = 0;

    if (readOffset + readLength > MAX_FILE_LENGTH)
    {
        status = STATUS_FILE_INVALID;
        completeLength = 0;
        goto SAFE_EXIT;
    }

    // 获取MDL地址
    PMDL pMdl = pIrp->MdlAddress;
    PVOID mdlAddress = (PVOID)((PCHAR)(pMdl->StartVa) + pMdl->ByteOffset);
    ULONG mdlLength = pMdl->ByteCount;
    if (mdlLength < readLength)
    {
        status = STATUS_FILE_INVALID;
        completeLength = 0;
        goto SAFE_EXIT;
    }

    memcpy(mdlAddress, pDevExt->PFileBuffer + readOffset, readLength);
    status = STATUS_SUCCESS;
    completeLength = readLength;

SAFE_EXIT:

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = completeLength;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave DirectReadRoutine\n"));

    return STATUS_SUCCESS;
}

/// @brief 对IRP进行处理
/// @param[in] pDriverObject
/// @param[in] pIrp
/// @return
NTSTATUS HelloNTDriverDispatchRoutine(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    static char* s_IRPName[] =
    {
        "IRP_MJ_CREATE               ",
        "IRP_MJ_CREATE_NAMED_PIPE    ",
        "IRP_MJ_CLOSE                ",
        "IRP_MJ_READ                 ",
        "IRP_MJ_WRITE                ",
        "IRP_MJ_QUERY_INFORMATION    ",
        "IRP_MJ_SET_INFORMATION      ",
        "IRP_MJ_QUERY_EA             ",
        "IRP_MJ_SET_EA               ",
        "IRP_MJ_FLUSH_BUFFERS        ",
        "IRP_MJ_QUERY_VOLUME_INFORMAT",
        "IRP_MJ_SET_VOLUME_INFORMATIO",
        "IRP_MJ_DIRECTORY_CONTROL    ",
        "IRP_MJ_FILE_SYSTEM_CONTROL  ",
        "IRP_MJ_DEVICE_CONTROL       ",
        "IRP_MJ_INTERNAL_DEVICE_CONTR",
        "IRP_MJ_SHUTDOWN             ",
        "IRP_MJ_LOCK_CONTROL         ",
        "IRP_MJ_CLEANUP              ",
        "IRP_MJ_CREATE_MAILSLOT      ",
        "IRP_MJ_QUERY_SECURITY       ",
        "IRP_MJ_SET_SECURITY         ",
        "IRP_MJ_POWER                ",
        "IRP_MJ_SYSTEM_CONTROL       ",
        "IRP_MJ_DEVICE_CHANGE        ",
        "IRP_MJ_QUERY_QUOTA          ",
        "IRP_MJ_SET_QUOTA            ",
        "IRP_MJ_PNP                  "
    };

    KdPrint(("Enter HelloNTDriverDispatchRoutine\n"));
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pDevObject);

    PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR irpType = pStack->MajorFunction;
    KdPrint((s_IRPName[irpType]));

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

    // 创建设备对象
    RtlInitUnicodeString(&devName, L"\\Device\\HelloNTDriverDevice");
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

    pDevObj->Flags |= DO_DIRECT_IO;
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    pDevExt->PDeviceObject = pDevObj;
    pDevExt->DeviceName = devName;
    pDevExt->PFileBuffer = (unsigned char*)ExAllocatePool(NonPagedPool, MAX_FILE_LENGTH);
    pDevExt->FileLength = 0;

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

        if (NULL != pDevExt->PFileBuffer)
        {
            ExFreePool(pDevExt->PFileBuffer);
            pDevExt->PFileBuffer = NULL;
        }

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
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = DirectWriteRoutine;
    pDriverObject->MajorFunction[IRP_MJ_READ] = DirectReadRoutine;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlRoutine;

    // 创建驱动设备对象
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));

    return status;
}