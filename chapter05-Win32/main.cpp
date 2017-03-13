

#include <cstdio>

#include <Windows.h>

int main()
{
    // ´¥·¢IRP_MJ_CREATE
    HANDLE hDevice = CreateFileA(
        "\\\\.\\HelloNTDriver1", 
        GENERIC_READ | GENERIC_WRITE, 
        0, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL);
    if (INVALID_HANDLE_VALUE == hDevice ||
        NULL == hDevice)
    {
        printf("Open Fail\n");
    }

    if (INVALID_HANDLE_VALUE != hDevice &&
        NULL != hDevice)
    {
        // ´¥·¢IRP_MJ_CLEANUP, IRP_MJ_CLOSE
        CloseHandle(hDevice);
        hDevice = NULL;
    }


    system("pause");
    return 0;
}