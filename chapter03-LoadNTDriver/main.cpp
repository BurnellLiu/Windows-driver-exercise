

#include <cstdio>
#include <cstdlib>


#include <Windows.h>

/// @brief ����NT��������
/// ��Ҫ����ԱȨ��, ��������ʧ��
/// 32λϵͳ��Ҫ����32λ����, 64λϵͳ��Ҫ����64λ����
/// @param[in] pDriverName ��������
/// @param[in] pDriverPath �����ļ�·��
/// @return �ɹ�����true, ʧ�ܷ���false
bool LoadNTDriver(IN const char* pDriverName, IN const char* pDriverPath)
{
    bool bRet = false;
    DWORD dwRet = FALSE;
    char* pFullPathBuffer = NULL;
    DWORD bufferSize = 256;
    DWORD fullPathSize = 0;

    SC_HANDLE hServiceManager = NULL; // ������ƹ��������
    SC_HANDLE hDriverService = NULL; // ����������

    // ��ȡ���������ļ�ȫ·��
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

    // ��SCM������
    // ��Ҫ����ԱȨ��
    hServiceManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hServiceManager)
    {
        bRet = false;
        printf("OpenSCManager Fail: %d\n", GetLastError());
        goto SAFE_EXIT;
    }

    // ��������
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

        // ��������Ѿ�������򿪷���
        hDriverService = OpenServiceA(hServiceManager, pDriverName, SERVICE_ALL_ACCESS);
        if (NULL == hDriverService)
        {
            printf("Open Service Fail: %d\n", GetLastError());
            bRet = false;
            goto SAFE_EXIT;
        }
    }

    // ��������
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

/// @brief ж��NT��������
/// ��Ҫ����ԱȨ��, �����ж��ʧ��
/// @param[in] pDriverName ������������
/// @return �ɹ�����true, ʧ�ܷ���false
bool UnLoadNTDriver(const char* pDriverName)
{
    bool bRet = true;
    DWORD dwRet = FALSE;

    SC_HANDLE hServiceManager = NULL; // ������ƹ��������
    SC_HANDLE hDriverService = NULL; // ����������
    SERVICE_STATUS serviceStatus;

    // ��SCM������
    // ��Ҫ����ԱȨ��
    hServiceManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hServiceManager)
    {
        bRet = false;
        printf("OpenSCManager Fail: %d\n", GetLastError());
        goto SAFE_EXIT;
    }

    // �򿪷���
    hDriverService = OpenServiceA(hServiceManager, pDriverName, SERVICE_ALL_ACCESS);
    if (NULL == hDriverService)
    {
        printf("Open Service Fail: %d\n", GetLastError());
        bRet = false;
        goto SAFE_EXIT;
    }

    // ֹͣ����
    dwRet = ControlService(hDriverService, SERVICE_CONTROL_STOP, &serviceStatus);
    if (dwRet == FALSE)
    {
        printf("Control Service Stop Fail: %d\n", GetLastError());
    }

    // ɾ������
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

/// @brief ������������
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