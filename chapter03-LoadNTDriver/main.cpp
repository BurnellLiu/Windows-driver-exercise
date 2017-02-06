

#include <cstdio>
#include <cstdlib>


#include <Windows.h>

/// @brief 加载NT驱动程序
/// 需要管理员权限, 否则会加载失败
/// 32位系统需要加载32位驱动, 64位系统需要加载64位驱动
/// @param[in] pDriverName 驱动名称
/// @param[in] pDriverPath 驱动文件路径
/// @return 成功返回true, 失败返回false
bool LoadNTDriver(IN const char* pDriverName, IN const char* pDriverPath)
{
    bool bRet = false;
    DWORD dwRet = FALSE;
    char* pFullPathBuffer = NULL;
    DWORD bufferSize = 256;
    DWORD fullPathSize = 0;

    SC_HANDLE hServiceManager = NULL; // 服务控制管理器句柄
    SC_HANDLE hDriverService = NULL; // 驱动服务句柄

    // 获取驱动程序文件全路径
    pFullPathBuffer = new char[bufferSize];
    ZeroMemory(pFullPathBuffer, bufferSize);
    fullPathSize = GetFullPathNameA(pDriverPath, bufferSize, pFullPathBuffer, NULL);
    if (fullPathSize > bufferSize)
    {
        delete[] pFullPathBuffer;
        pFullPathBuffer = NULL;
        pFullPathBuffer = new char[fullPathSize];
        ZeroMemory(pFullPathBuffer, fullPathSize);
        GetFullPathNameA(pDriverPath, bufferSize, pFullPathBuffer, NULL);
    }

    // 打开SCM管理器
    // 需要管理员权限
    hServiceManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hServiceManager)
    {
        bRet = false;
        printf("OpenSCManager Fail: %d\n", GetLastError());
        goto SAFE_EXIT;
    }

    // 创建服务
    hDriverService = CreateServiceA(
        hServiceManager,
        pDriverName,
        pDriverName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        pFullPathBuffer,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

    if (NULL == hDriverService)
    {
         dwRet = GetLastError();
        if (dwRet != ERROR_IO_PENDING && dwRet != ERROR_SERVICE_EXISTS)
        {
            printf("CreateService Fail: %d\n", dwRet);
            bRet = false;
            goto SAFE_EXIT;
        }
        else
        {
            printf("Service Is Exist\n");
        }

        // 如果服务已经存在则打开服务
        hDriverService = OpenServiceA(hServiceManager, pDriverName, SERVICE_ALL_ACCESS);
        if (NULL == hDriverService)
        {
            printf("Open Service Fail: %d\n", GetLastError());
            bRet = false;
            goto SAFE_EXIT;
        }
    }

    // 启动服务
    dwRet = StartServiceA(hDriverService, NULL, NULL);
    if (FALSE == dwRet)
    {
        dwRet = GetLastError();
        if (dwRet != ERROR_SERVICE_ALREADY_RUNNING)
        {
            printf("Start Service Fail: %d\n", dwRet);
            bRet = false;
            goto SAFE_EXIT;
        }
        printf("Service Had Been Started\n");
    }

    bRet = true;

SAFE_EXIT:

    if (NULL != hDriverService)
    {
        CloseServiceHandle(hDriverService);
        hDriverService = NULL;
    }

    if (NULL != hServiceManager)
    {
        CloseServiceHandle(hServiceManager);
        hServiceManager = NULL;
    }

    if (pFullPathBuffer != NULL)
    {
        delete[] pFullPathBuffer;
        pFullPathBuffer = NULL;
    }

    return bRet;
};

/// @brief 卸载NT驱动程序
/// 需要管理员权限, 否则会卸载失败
/// @param[in] pDriverName 驱动程序名称
/// @return 成功返回true, 失败返回false
bool UnLoadNTDriver(const char* pDriverName)
{
    bool bRet = true;
    DWORD dwRet = FALSE;

    SC_HANDLE hServiceManager = NULL; // 服务控制管理器句柄
    SC_HANDLE hDriverService = NULL; // 驱动服务句柄
    SERVICE_STATUS serviceStatus;

    // 打开SCM管理器
    // 需要管理员权限
    hServiceManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hServiceManager)
    {
        bRet = false;
        printf("OpenSCManager Fail: %d\n", GetLastError());
        goto SAFE_EXIT;
    }

    // 打开服务
    hDriverService = OpenServiceA(hServiceManager, pDriverName, SERVICE_ALL_ACCESS);
    if (NULL == hDriverService)
    {
        printf("Open Service Fail: %d\n", GetLastError());
        bRet = false;
        goto SAFE_EXIT;
    }

    // 停止服务
    dwRet = ControlService(hDriverService, SERVICE_CONTROL_STOP, &serviceStatus);
    if (dwRet == FALSE)
    {
        printf("Control Service Stop Fail: %d\n", GetLastError());
    }

    // 删除服务
    dwRet = DeleteService(hDriverService);
    if (dwRet == FALSE)
    {
        printf("Delete Service Fail: %d\n", GetLastError());
    }

SAFE_EXIT:

    if (NULL != hDriverService)
    {
        CloseServiceHandle(hDriverService);
        hDriverService = NULL;
    }

    if (NULL != hServiceManager)
    {
        CloseServiceHandle(hServiceManager);
        hServiceManager = NULL;
    }

    return bRet;
}

/// @brief 测试驱动程序
void TestNTDriver()
{
    HANDLE hDevice = CreateFileA(
        "\\\\.\\HelloNTDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
        printf("Open Device Fail\n");
    else
        printf("Open Device Success\n");

    CloseHandle(hDevice);
}

int main(int argc, char** argv)
{
    if (argc == 3)
    {
        bool bRet = false;

        bRet = LoadNTDriver(argv[1], argv[2]);
        if (bRet)
            printf("Load NT Driver Success\n");
        else
            printf("Load NT Driver Fail\n");

        system("pause");

        TestNTDriver();

        system("pause");

        bRet = UnLoadNTDriver(argv[1]);
        if (bRet)
            printf("UnLoad NT Driver Success\n");
        else
            printf("UnLoad NT Driver Fail\n");
    }
   
    system("pause");
    return 0;
}