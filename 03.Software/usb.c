#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <initguid.h>
#include <windows.h>
#include <basetyps.h>
#include <winioctl.h>
#include <setupapi.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <usbiodef.h>
#include<hidsdi.h>
#include <hidclass.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib,"hid.lib ")
TCHAR * cExampleId = "{b5078240-a3f3-5a4c-aae5-d196daa1777c}";

typedef struct _HIDMINI_OUTPUT_REPORT {

	UCHAR ReportId;

	UCHAR Data[64];

} HIDMINI_OUTPUT_REPORT, *PHIDMINI_OUTPUT_REPORT;


typedef struct COMPortData
{
	TCHAR id[128];
	TCHAR port[128];
}COMPortData;
DWORD dwComCount = 0;
COMPortData comPortArray[10] = { 0 };

DWORD dwPrintContainerId(TCHAR * id, DWORD dwLen)
{
	for (int i = 0; i<dwLen; ++i) {
		if (id[i] != 0)
			printf("%c", id[i]);
	}
	printf("\r\n");
}
DWORD dwCheckContainerId(TCHAR * pcId1, TCHAR * pcId2, DWORD dwLen)
{
	if (pcId1 == NULL || pcId2 == NULL)
		return 0; // 返回0表示输入无效

	for (DWORD i = 0; i < dwLen; i++)
	{
		if (pcId1[i] != pcId2[i])
			return 0; // 返回0表示数组不相等
	}

	return 1; // 返回1表示数组相等
}
HANDLE OpenHidDevice(LPTSTR hidDevicePath);
int usbmain(void) {
	HDEVINFO deviceInfoSet;
	SP_DEVINFO_DATA deviceInfoData;
	SP_DEVICE_INTERFACE_DATA interfaceData;
	SP_DEVICE_INTERFACE_DETAIL_DATA *interfaceDetailData = NULL;
	DWORD requiredSize = 0;
	DWORD index = 0;
	//GUID hidGuid = GUID_CLASS_USB_DEVICE;
	//HidD_GetHidGuid(&hidGuid);
	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);
	// 初始化设备信息集合
	deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		printf("Failed to get device information set. Error code: %d\r\n", GetLastError());
		return 1;
	}

	// 枚举设备
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); index++) {
		TCHAR containerID[MAX_PATH] = { 0 };
		if (!SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_BASE_CONTAINERID, NULL, (PBYTE)containerID, sizeof(containerID), NULL)) {
			printf("Failed to get base container ID for hid. Error %d\r\n", GetLastError());
			SetupDiDestroyDeviceInfoList(deviceInfoSet);
			return 1;
		}
		printf("Base Container ID for HID \r\n");
		for (int i = 0; i<sizeof(containerID); ++i) {
			if (containerID[i] == 0)
				continue;
			printf("%c", containerID[i]);
		}
		printf("\r\n");
		int iFound = -1;
		for (DWORD i = 0; i < dwComCount; i++)
		{
			if (dwCheckContainerId(containerID, comPortArray[i].id, strlen(cExampleId)*2))
			{
				dwPrintContainerId(containerID, strlen(cExampleId) * 2);
				printf("!Found COM port %s", comPortArray[i].port);
				iFound = i;
			}
		}
		if (iFound == -1)
			continue;
		// 枚举设备接口
		interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		for (DWORD j = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, &deviceInfoData, &hidGuid, j, &interfaceData); j++) {
			// 获取设备接口详情数据大小
			if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL)) {
				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
					printf("Failed to get device interface detail data size. Error code: %d\r\n", GetLastError());
					continue;
				}
			}

			// 分配内存并获取设备接口详情数据
			interfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(requiredSize);
			interfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, interfaceDetailData, requiredSize, NULL, NULL)) {
				printf("Failed to get device interface detail data. Error code: %d\r\n", GetLastError());
				free(interfaceDetailData);
				continue;
			}

			// 输出设备路径
			printf("Device Path: %s\r\n", interfaceDetailData->DevicePath);
			HANDLE pDeviceHandle = OpenHidDevice(interfaceDetailData->DevicePath);
			if (pDeviceHandle == INVALID_HANDLE_VALUE)
			{
				continue;
			}

			// 这里可以进一步处理获取的设备信息，例如获取设备属性等
			HIDD_ATTRIBUTES Attributes;
			ZeroMemory(&Attributes, sizeof(Attributes));
			Attributes.Size = sizeof(HIDD_ATTRIBUTES);
			if (!HidD_GetAttributes(pDeviceHandle, &Attributes))
			{
				CloseHandle(pDeviceHandle);
				continue;
			}

			printf("VenID 0x%x, Product ID 0x%x\r\n", Attributes.VendorID, Attributes.ProductID);

			PHIDMINI_OUTPUT_REPORT buffer;
			ULONG bufferSize;
			BOOLEAN bSuccess;

			bufferSize = sizeof(HIDMINI_OUTPUT_REPORT);
			buffer = (PHIDMINI_OUTPUT_REPORT)malloc(bufferSize);
			ZeroMemory(buffer, bufferSize);
			buffer->ReportId = 0x02;
			buffer->Data[0] = 0x00;
			buffer->Data[1] = 0x01;
			buffer->Data[2] = 0x02;
			//bSuccess = HidD_SetOutputReport(pDeviceHandle,  // HidDeviceObject,
			//	buffer,    // ReportBuffer,
			//	bufferSize // ReportBufferLength
			//);
			ULONG rtn = 0;

			bSuccess = WriteFile(pDeviceHandle, buffer, bufferSize, &rtn, NULL);

			if (!bSuccess)
			{
				printf("failed HidD_SetOutputReport\n");
			}
			else
			{
				printf("Set following data in output report: %d\n",
					((PHIDMINI_OUTPUT_REPORT)buffer)->Data);
			}

			free(buffer);

			free(interfaceDetailData);
		}
	}

	// 释放设备信息集合
	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	return 0;
}

HANDLE OpenHidDevice(LPTSTR hidDevicePath) {
	printf("openint hid path %s\r\n", hidDevicePath);
	//HANDLE hidDevice = CreateFile(hidDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	  HANDLE hidDevice = CreateFile(hidDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hidDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to open HID device. Error code: %d %s\n", GetLastError(), hidDevicePath);
		return NULL;
	}
	printf("open hid device %s success !!!!!!!!!!!!!!\r\n", hidDevicePath);
	return hidDevice;
}

void GetCompositeDevicePath(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, LPTSTR compositeDevicePath) {
	DWORD dataType;
	DWORD requiredSize = 0;

	// 获取设备路径
	if (!SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC, &dataType, (PBYTE)compositeDevicePath, MAX_PATH, &requiredSize)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			printf("Failed to get device registry property. Error code: %d\n", GetLastError());
			return;
		}
	}
}

// 获取USB Composite设备标识符
void GetCompositeDeviceInstanceId(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, LPTSTR compositeDeviceInstanceId) {
	DWORD requiredSize = 0;

	if (!SetupDiGetDeviceInstanceId(deviceInfoSet, deviceInfoData, compositeDeviceInstanceId, MAX_PATH, &requiredSize)) {
		printf("Failed to get device instance ID. Error code: %d\n", GetLastError());
		return;
	}
}


int iReadComContainerID(TCHAR * comPortName);
void GetSerialPort(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData) {
	DWORD dataType;
	DWORD requiredSize = 0;
	TCHAR portName[MAX_PATH];

	// 获取设备描述符
	if (!SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_FRIENDLYNAME, &dataType, (BYTE*)portName, sizeof(portName), &requiredSize)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			//printf("Failed to get device registry property. Error code: %d\n", GetLastError());
			return;
		}
	}


	// 从设备描述符中解析出串口号
	TCHAR* portToken = _tcsstr(portName, _T("(COM"));
	if (portToken != NULL) {
		iReadComContainerID(portName);
		_tcsncpy(portName, portToken, _tcslen(portToken) );
		_tprintf(_T("Serial Port: %s\n"), portName);
		TCHAR compositeDevicePath[MAX_PATH];
		GetCompositeDevicePath(deviceInfoSet, deviceInfoData, compositeDevicePath);
		_tprintf(_T("Associated USB Composite Device: %s\n"), compositeDevicePath);
		TCHAR compositeDeviceInstanceId[MAX_PATH];
		GetCompositeDeviceInstanceId(deviceInfoSet, deviceInfoData, compositeDeviceInstanceId);
		_tprintf(_T("Associated USB Composite Device ID: %s\n"), compositeDeviceInstanceId);
		printf("\r\n");
	}
}




int serialmain() {
	HDEVINFO deviceInfoSet;
	SP_DEVINFO_DATA deviceInfoData;
	DWORD index = 0;

	// 初始化设备信息集合
	deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES | DIGCF_PROFILE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		printf("Failed to get device information set. Error code: %d\n", GetLastError());
		return 1;
	}

	// 枚举设备
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); index++) {
		TCHAR devicePath[MAX_PATH];
		if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)devicePath, sizeof(devicePath), NULL)) {
			//_tprintf(_T("Device Description: %s\n"), devicePath);
		}
		// 获取串口号
		GetSerialPort(deviceInfoSet, &deviceInfoData);

	}

	// 释放设备信息集合
	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	return 0;
}


int iReadComContainerID(TCHAR * comPortName) {
	//const TCHAR comPortName[] = ("COM18");
	TCHAR containerID[MAX_PATH] = { 0 };
	//GUID hidGuid;
	//HidD_GetHidGuid((LPGUID)&hidGuid);
	// Get device information set for COM ports
	HDEVINFO hDeviceInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
	if (hDeviceInfo == INVALID_HANDLE_VALUE) {
		printf("Failed to get device information set for COM ports. Error %d\n", GetLastError());
		return 1;
	}

	// Enumerate COM ports and search for the specified one
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	DWORD dwIndex = 0;
	BOOL found = FALSE;
	while (SetupDiEnumDeviceInfo(hDeviceInfo, dwIndex, &deviceInfoData)) {
		// Get device friendly name
		TCHAR friendlyName[MAX_PATH];
		if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
			printf("Failed to get device friendly name. Error %d\n", GetLastError());
			continue;
		}

		// Check if it's the specified COM port
		if (_tcsstr(friendlyName, comPortName) != NULL) {
			// Get base container ID
			if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_BASE_CONTAINERID, NULL, (PBYTE)containerID, sizeof(containerID), NULL)) {
				printf("Failed to get base container ID for COM port %s. Error %d\n", comPortName, GetLastError());
				SetupDiDestroyDeviceInfoList(hDeviceInfo);
				return 1;
			}

			found = TRUE;
			break;
		}

		dwIndex++;
	}

	// Clean up
	SetupDiDestroyDeviceInfoList(hDeviceInfo);

	if (found) {
		printf("Base Container ID for COM port %s: %s\n", comPortName, containerID);
		for (int i = 0; i<sizeof(containerID); ++i) {
			if (containerID[i] != 0)
				printf("%c", containerID[i]);
			comPortArray[dwComCount].id[i] = containerID[i];
		}
		printf("\r\n");
		if (containerID[2] != 0x30 && containerID[4] != 0x30)
		{
			sprintf(comPortArray[dwComCount].port, comPortName);
			dwComCount++;
		}
		else {
			printf("no add \r\n");
		}
	}
	else {
		printf("COM port %s not found.\n", comPortName);
		return 1;
	}

	return 0;
}
