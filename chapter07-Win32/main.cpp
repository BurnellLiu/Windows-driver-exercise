

#include <cstdio>

#include <Windows.h>

#define IOCTL_TEST1 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TEST2 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

void TestIOControl1(HANDLE hDevice)
{
    UCHAR inputBuffer[10] = { 0 };
    UCHAR outputBuffer[10] = { 0 };
    memset(inputBuffer, 0xBB, 10);
    DWORD dwOutput;
    //输入缓冲区作为输入，输出缓冲区作为输出
    BOOL iRet = DeviceIoControl(hDevice, IOCTL_TEST1, inputBuffer, 10, outputBuffer, 10, &dwOutput, NULL);
    if (FALSE == iRet)
        return;

    printf("Output buffer:%d bytes\n", dwOutput);
    for (int i = 0; i < (int)dwOutput; i++)
    {
        printf("%02X ", outputBuffer[i]);
    }
    printf("\n");

}

void TestIOControl2(HANDLE hDevice)
{
    UCHAR inputBuffer[10] = { 0 };
    UCHAR outputBuffer[10] = { 0 };
    memset(inputBuffer, 0xBB, 10);
    DWORD dwOutput;
    //输入缓冲区作为输入，输出缓冲区作为输出
    BOOL iRet = DeviceIoControl(hDevice, IOCTL_TEST2, inputBuffer, 10, outputBuffer, 10, &dwOutput, NULL);
    if (FALSE == iRet)
        return;

    printf("Output buffer:%d bytes\n", dwOutput);
    for (int i = 0; i < (int)dwOutput; i++)
    {
        printf("%02X ", outputBuffer[i]);
    }
    printf("\n");

}

void TestReadAndWrite(HANDLE hDevice)
{
    // 写设备, 触发IRP_MJ_WRITE
    UCHAR writeBuffer[10] = { 0 };
    ULONG writeLen = 0;
    memset(writeBuffer, 0xAB, 10);
    BOOL iRet = WriteFile(hDevice, writeBuffer, 10, &writeLen, NULL);
    if (TRUE == iRet)
    {
        printf("Writed Length: %u\n", writeLen);
    }

    // 读设备, 触发IRP_MJ_READ
    UCHAR readBuffer[10] = { 0 };
    ULONG readedLen = 0;
    iRet = ReadFile(hDevice, readBuffer, 10, &readedLen, NULL);
    if (TRUE == iRet)
    {
        printf("Readed Length : %u\n", readedLen);
        for (ULONG i = 0; i < readedLen; i++)
        {
            printf("%02X ", readBuffer[i]);
        }
        printf("\n");
    }
}


int main(int argc, char *argv[])
{
    // 触发IRP_MJ_CREATE
    HANDLE hDevice = CreateFileA(
        "\\\\.\\HelloNTDriver", 
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
        goto SAFE_EXIT;
    }

    if (argc >= 2)
    {
        if (strcmp(argv[1], "RW") == 0)
            TestReadAndWrite(hDevice);

        if (strcmp(argv[1], "IOCTL1") == 0)
            TestIOControl1(hDevice);

        if (strcmp(argv[1], "IOCTL2") == 0)
            TestIOControl2(hDevice);
    }

    

SAFE_EXIT:

    if (INVALID_HANDLE_VALUE != hDevice &&
        NULL != hDevice)
    {
        // 触发IRP_MJ_CLEANUP, IRP_MJ_CLOSE
        CloseHandle(hDevice);
        hDevice = NULL;
    }


    system("pause");
    return 0;
}