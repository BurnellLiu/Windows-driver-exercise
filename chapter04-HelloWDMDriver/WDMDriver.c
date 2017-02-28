
#include <wdm.h>


/// @brief 设备扩展结构
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT PDeviceObject; ///< 设备对象
    PDEVICE_OBJECT PNextStackDevice; ///< 下层设备对象指针
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/// @brief 对PNP IRP进行默认处理
/// @param[in] pDeviceExt 设备对象扩展
/// @param[in] pIrp I/O请求包
/// @return 状态
NTSTATUS PnpDefaultHandler(PDEVICE_EXTENSION pDeviceExt, PIRP pIrp)
{
    KdPrint(("Enter DefaultPnpHandler\n"));

    // 略过当前堆栈
    IoSkipCurrentIrpStackLocation(pIrp);

    KdPrint(("Leave DefaultPnpHandler\n"));

    // 用下层堆栈的驱动设备处理此IRP
    return IoCallDriver(pDeviceExt->PNextStackDevice, pIrp);
}
  
/// @brief PNP移除设备处理函数
/// @param[in] pDeviceExt 设备扩展对象
/// @param[in] pIrp 请求包
/// @return 状态
NTSTATUS PnpRemoveDevice(PDEVICE_EXTENSION pDeviceExt, PIRP pIrp)
{
    KdPrint(("Enter HandleRemoveDevice\n"));

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    NTSTATUS status = PnpDefaultHandler(pDeviceExt, pIrp);

    //调用IoDetachDevice()把设备对象从设备栈中脱开：  
    if (pDeviceExt->PNextStackDevice != NULL)
        IoDetachDevice(pDeviceExt->PNextStackDevice);

    //删除设备对象：  
    IoDeleteDevice(pDeviceExt->PDeviceObject);

    KdPrint(("Leave HandleRemoveDevice\n"));

    return status;
}

/// @brief 添加新设备
/// @param[in] pDriverObject 从I/O管理器传进来的驱动对象
/// @param[in] pPhysicalDeviceObject 从I/O管理器传进来的物理设备对象
/// @return 添加新设备状态
NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
{
    KdPrint(("Enter HelloWDMAddDevice\n"));

    NTSTATUS status = STATUS_SUCCESS;

    PDEVICE_OBJECT pDeviceObject = NULL; // 创建的设备对象
    PDEVICE_EXTENSION pDeviceExt = NULL; // 设备扩展对象

    // 创建设备
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

    // 讲设备对象挂接在设备堆栈上
    pDeviceExt->PNextStackDevice = IoAttachDeviceToDeviceStack(pDeviceObject, pPhysicalDeviceObject);

   
    pDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    KdPrint(("Leave HelloWDMAddDevice\n"));

    return status;
}

/// @brief 对即插即用IPR进行处理
/// @param[in] pDeviceObject 功能设备对象
/// @param[in] pIrp 请求包
/// @return 状态
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


/// @brief 对默认IPR进行处理
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

/// @brief 驱动程序卸载操作
void HelloWDMUnload(IN PDRIVER_OBJECT pDriverObject)
{
    UNREFERENCED_PARAMETER(pDriverObject);

    KdPrint(("Enter HelloWDMUnload\n"));
    KdPrint(("Leave HelloWDMUnload\n"));
}


/// @brief 初始化驱动程序
/// @param[in] pDriverObject 驱动对象
/// @param[in] pRegPath 驱动程序在注册表中的路径
/// @return 初始化驱动状态
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