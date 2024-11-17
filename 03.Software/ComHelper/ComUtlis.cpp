#include "stdafx.h"
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
#include <vector>
#include <map>
#include "ComHelperDlg.h"
#include "afxwin.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib,"hid.lib ")
int32_t iDebug = 0;
using namespace std;
char * cExampleId = "{b5078240-a3f3-5a4c-aae5-d196daa1777c}";

typedef struct _HIDMINI_OUTPUT_REPORT {

	UCHAR ReportId;

	UCHAR Data[64];

} HIDMINI_OUTPUT_REPORT, *PHIDMINI_OUTPUT_REPORT;



typedef struct COMPortData
{
	TCHAR id[256];
	TCHAR port[256];
	TCHAR name[128];
	TCHAR acHardwareId[128];
	time_t timestamp;
	uint32_t hid;
}COMPortData;
DWORD dwComCount = 0;
DWORD dwComShadowCount = 0;

COMPortData comPortArray[100] = { 0 };
COMPortData comShadowPortArray[100] = { 0 };

vector<COMPortData> hComPortList;

DWORD iGetComPortNumber(void)
{
	return dwComCount;
}

time_t iGetComPortTimestamp(DWORD dwIndex)
{
	return comPortArray[dwIndex].timestamp;
}

TCHAR * iGetComPortName(DWORD dwIndex)
{
	return comPortArray[dwIndex].port;
}

DWORD dwPrintContainerId(TCHAR * id, DWORD dwLen)
{
	for (DWORD i = 0; i<dwLen; ++i) {
		if (id[i] != 0)
			printf("%c", id[i]);
	}
	printf("\r\n");
	return 0;
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

HANDLE SyncNameToHidDevice(HWND hWin, LPTSTR hidDevicePath, TCHAR * pcComPortName);
HANDLE SyncNamesToHidDevice(HWND hWin, LPTSTR hidDevicePath, int16_t * piComIndex, int16_t * piInterface, DWORD iCount);

int iRefreshComPortHid(HWND hWin) {
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
	_tprintf(_T("Refresh HID\r\n"));

	deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Failed to get device information set. Error code: %d\r\n"), GetLastError());
		return 1;
	}

	// 枚举设备
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); index++) {
		TCHAR containerID[MAX_PATH] = { 0 };
		if (!SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_BASE_CONTAINERID, NULL, (PBYTE)containerID, sizeof(containerID), NULL)) {
			_tprintf(_T("Failed to get base container ID for hid. Error %d\r\n"), GetLastError());
			SetupDiDestroyDeviceInfoList(deviceInfoSet);
			return 1;
		}
		if (iDebug)
		{
			_tprintf(_T("Base Container ID for HID \r\n"));
			for (DWORD i = 0; i < _tcslen(containerID); ++i) {
				//if (containerID[i] == 0)
				//	continue;
				printf("%c", containerID[i]);
			}
			printf("\r\n");
		}
		if (containerID[2] == 0x30 && containerID[4] == 0x30)
		{
			_tprintf(_T("Skip invalid container id %s\r\n"), containerID);
			continue;
		}
		int iFound = -1;
		uint32_t iFoundCount = 0;
		int16_t iComIndex[10] = { -1 };
		int16_t iInterfaceIndex[10] = { -1 };
		for (DWORD i = 0; i < dwComCount; i++)
		{
			if (dwCheckContainerId(containerID, comPortArray[i].id, strlen(cExampleId) * 2))
			{
				//dwPrintContainerId(containerID, strlen(cExampleId) * 2);
				TCHAR * iStart = _tcsstr(comPortArray[i].acHardwareId, _T("MI_"));
				
				iFound = i;
				iComIndex[iFoundCount] = i;
				iInterfaceIndex[iFoundCount] = _wtoi(iStart + 3);
				_tprintf(_T("Found COM port %s with hardware id %s, container id %s interface id %d\r\n"), 
					comPortArray[i].port, comPortArray[i].acHardwareId, containerID, iInterfaceIndex[iFoundCount]);

				iFoundCount++;
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
					_tprintf(_T("Failed to get device interface detail data size. Error code: %d\r\n"), GetLastError());
					continue;
				}
			}

			// 分配内存并获取设备接口详情数据
			interfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(requiredSize);
			interfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, interfaceDetailData, requiredSize, NULL, NULL)) {
				_tprintf(_T("Failed to get device interface detail data. Error code: %d\r\n"), GetLastError());
				free(interfaceDetailData);
				continue;
			}

			// 输出设备路径
			_tprintf(_T("hid path: %s\r\n"), interfaceDetailData->DevicePath);
			//HANDLE pDeviceHandle = SyncNameToHidDevice(hWin, interfaceDetailData->DevicePath, comPortArray[iFound].name);
			SyncNamesToHidDevice(hWin, interfaceDetailData->DevicePath, iComIndex, iInterfaceIndex, iFoundCount);

			free(interfaceDetailData);
		
		}
	}

	// 释放设备信息集合
	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	return 0;
}


char* TCHARToChar(const TCHAR* tcharString) {
	// Get the length of the string
	int length = WideCharToMultiByte(CP_UTF8, 0, tcharString, -1, NULL, 0, NULL, NULL);

	// Allocate memory for the char string
	char* charString = new char[length];

	// Convert TCHAR to char
	WideCharToMultiByte(CP_UTF8, 0, tcharString, -1, charString, length, NULL, NULL);

	return charString;
}

// Swap function to swap elements in both arrays
void swap(int16_t *a, int16_t *b) {
	int16_t temp = *a;
	*a = *b;
	*b = temp;
}

// Bubble Sort function
void bubbleSort(int16_t *piComIndex, int16_t *piInterface, DWORD iCount) {
	for (DWORD i = 0; i < iCount - 1; i++) {
		for (DWORD j = 0; j < iCount - i - 1; j++) {
			if (piInterface[j] > piInterface[j + 1]) {
				// Swap elements in both arrays
				swap(&piInterface[j], &piInterface[j + 1]);
				swap(&piComIndex[j], &piComIndex[j + 1]);
			}
		}
	}
}


HANDLE SyncNamesToHidDevice(HWND hWin, LPTSTR hidDevicePath, int16_t * piComIndex, int16_t * piInterface, DWORD iCount) {
	HANDLE pDeviceHandle = CreateFile(hidDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pDeviceHandle == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Failed to open HID device. Error code: %d %s\n"), GetLastError(), hidDevicePath);
		return NULL;
	}
	_tprintf(_T("open hid device %s success\n"), hidDevicePath);

	HIDD_ATTRIBUTES Attributes;
	ZeroMemory(&Attributes, sizeof(Attributes));
	Attributes.Size = sizeof(HIDD_ATTRIBUTES);
	if (!HidD_GetAttributes(pDeviceHandle, &Attributes))
	{
		CloseHandle(pDeviceHandle);
		return pDeviceHandle;
	}

	printf("vendor id 0x%x, product id 0x%x\r\n", Attributes.VendorID, Attributes.ProductID);

	bubbleSort(piComIndex, piInterface, iCount);
	for (int i = 0; i < iCount; i++)
	{

		_tprintf(_T("i:%d,name:%s\n"),i, comPortArray[piComIndex[i]].name);

		PHIDMINI_OUTPUT_REPORT buffer;
		ULONG bufferSize;
		BOOLEAN bSuccess;

		bufferSize = sizeof(HIDMINI_OUTPUT_REPORT);
		buffer = (PHIDMINI_OUTPUT_REPORT)malloc(bufferSize);
		ZeroMemory(buffer, bufferSize);
		buffer->ReportId = 0x80;
		buffer->Data[0] = 0x80;
		buffer->Data[1] = i;
		buffer->Data[2] = 'C';
		buffer->Data[3] = 'O';
		buffer->Data[4] = 'M';
		buffer->Data[5] = '1';
		buffer->Data[6] = '9';
		buffer->Data[7] = 0;
		char * name = TCHARToChar(comPortArray[piComIndex[i]].name);
		snprintf((char *)(buffer->Data + 2), 64, "UART%d:%s->", i+1, name);
		free(name);
		//bSuccess = HidD_SetOutputReport(pDeviceHandle,  // HidDeviceObject,
		//	buffer,    // ReportBuffer,
		//	bufferSize // ReportBufferLength
		//);


		ULONG rtn = 0;

		bSuccess = WriteFile(pDeviceHandle, buffer, bufferSize, &rtn, NULL);

		if (!bSuccess)
		{
			_tprintf(_T("Failed to write hid, rtn %d\n"), rtn);
		}
		else
		{
			_tprintf(_T("Set com port name to hid successt\n"));

			ComMessage * comMessage = new ComMessage();
			comMessage->mComString.Format(TEXT("%s"), comPortArray[piComIndex[i]].name);
			comMessage->mString.Format(TEXT("同步串口信息"));
			comMessage->mType = 0;
			CComHelperDlg::PostUserMessage(hWin, comMessage);
		}

	}

	CloseHandle(pDeviceHandle);

	return pDeviceHandle;

	return NULL;
}
HANDLE SyncNameToHidDevice(HWND hWin, LPTSTR hidDevicePath, TCHAR * pcComPortName) {
	//_tprintf(_T("openint hid path %s\r\n"), hidDevicePath);
	//HANDLE hidDevice = CreateFile(hidDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	HANDLE pDeviceHandle = CreateFile(hidDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pDeviceHandle == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Failed to open HID device. Error code: %d %s\n"), GetLastError(), hidDevicePath);
		return NULL;
	}
	_tprintf(_T("open hid device %s success\n"), hidDevicePath);

	// 这里可以进一步处理获取的设备信息，例如获取设备属性等
	HIDD_ATTRIBUTES Attributes;
	ZeroMemory(&Attributes, sizeof(Attributes));
	Attributes.Size = sizeof(HIDD_ATTRIBUTES);
	if (!HidD_GetAttributes(pDeviceHandle, &Attributes))
	{
		CloseHandle(pDeviceHandle);
		return pDeviceHandle;
	}

	printf("vendor id 0x%x, product id 0x%x\r\n", Attributes.VendorID, Attributes.ProductID);

	PHIDMINI_OUTPUT_REPORT buffer;
	ULONG bufferSize;
	BOOLEAN bSuccess;

	static int i = 0;
	bufferSize = sizeof(HIDMINI_OUTPUT_REPORT);
	buffer = (PHIDMINI_OUTPUT_REPORT)malloc(bufferSize);
	ZeroMemory(buffer, bufferSize);
	buffer->ReportId = 0x80;
	buffer->Data[0] = 0x80;
	buffer->Data[1] = 'C';
	buffer->Data[2] = 'O';
	buffer->Data[3] = 'M';
	buffer->Data[4] = '1';
	buffer->Data[5] = '9';
	buffer->Data[6] = 0;
	char * name = TCHARToChar(pcComPortName);
	snprintf((char *)(buffer->Data + 1), 64, "N%d:%s",i++, name);
	free(name);
	//bSuccess = HidD_SetOutputReport(pDeviceHandle,  // HidDeviceObject,
	//	buffer,    // ReportBuffer,
	//	bufferSize // ReportBufferLength
	//);


	ULONG rtn = 0;

	bSuccess = WriteFile(pDeviceHandle, buffer, bufferSize, &rtn, NULL);

	if (!bSuccess)
	{
		_tprintf(_T("Failed to write hid, rtn %d\n"),rtn);
	}
	else
	{
		_tprintf(_T("Set com port name to hid successt\n"));

		ComMessage * comMessage = new ComMessage();
		comMessage->mComString.Format(TEXT("%s"), pcComPortName);
		comMessage->mString.Format(TEXT("同步串口信息"));
		comMessage->mType = 0;
		CComHelperDlg::PostUserMessage(hWin, comMessage);
	}

	free(buffer);
	CloseHandle(pDeviceHandle);

	return pDeviceHandle;
}

void GetCompositeDevicePath(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, LPTSTR compositeDevicePath) {
	DWORD dataType;
	DWORD requiredSize = 0;

	// 获取设备路径
	if (!SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC, &dataType, (PBYTE)compositeDevicePath, MAX_PATH, &requiredSize)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			_tprintf(_T("Failed to get device registry property. Error code: %d\n"), GetLastError());
			return;
		}
	}
}

// 获取USB Composite设备标识符
void GetCompositeDeviceInstanceId(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, LPTSTR compositeDeviceInstanceId) {
	DWORD requiredSize = 0;

	if (!SetupDiGetDeviceInstanceId(deviceInfoSet, deviceInfoData, compositeDeviceInstanceId, MAX_PATH, &requiredSize)) {
		_tprintf(_T("Failed to get device instance ID. Error code: %d\n"), GetLastError());
		return;
	}
}


int iRefreshComPort(HWND hWin) {
	HDEVINFO hDeviceInfo;
	SP_DEVINFO_DATA deviceInfoData;
	COMPortData hComData = { { 0 },{ 0 },1 };
	vector<COMPortData> hNewComPortList;
	DWORD index = 0;
	dwComShadowCount = 0;
	memset(comShadowPortArray, 0, sizeof(comShadowPortArray));
	// 初始化设备信息集合
	hDeviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES | DIGCF_PROFILE);
	if (hDeviceInfo == INVALID_HANDLE_VALUE) {
		printf("Failed to get device information set. Error code: %d\n", GetLastError());
		return 1;
	}

	// 枚举设备
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	CTime tm = CTime::GetCurrentTime();
	time_t milliSeconds = tm.GetTime();
	for (index = 0; SetupDiEnumDeviceInfo(hDeviceInfo, index, &deviceInfoData); index++) {
		TCHAR devicePath[MAX_PATH];
		TCHAR portName[MAX_PATH];
		DWORD dataType;
		DWORD requiredSize = 0;
		if (SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)devicePath, sizeof(devicePath), NULL)) {
			//_tprintf(_T("Device Description: %s\n"), devicePath);
		}
		
		// 获取串口号
		if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, &dataType, (BYTE*)portName, sizeof(portName), &requiredSize)) {
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				//printf("Failed to get device registry property. Error code: %d\n", GetLastError());
				continue;
			}
		}

		TCHAR* portToken = _tcsstr(portName, _T("(COM"));
		if (portToken != NULL) {
			TCHAR acDeviceHardwareId[MAX_PATH];

			if (SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)acDeviceHardwareId, sizeof(acDeviceHardwareId), NULL)) {
				_tprintf(_T("Device HARDWARDID: %s\n"), acDeviceHardwareId);
				_tcsncpy_s(comShadowPortArray[dwComShadowCount].acHardwareId, acDeviceHardwareId, _tcslen(acDeviceHardwareId));
			}

			TCHAR containerID[MAX_PATH] = { 0 };
		
			if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo, &deviceInfoData, SPDRP_BASE_CONTAINERID, NULL, (PBYTE)containerID, sizeof(containerID), NULL)) {
				_tprintf(_T("Failed to get base container ID for COM port %s. Error %d\n"), portName, GetLastError());
				SetupDiDestroyDeviceInfoList(hDeviceInfo);
				return 1;
			}
	
			/* copy com port name */
			_tcsncpy_s(comShadowPortArray[dwComShadowCount].name, portToken, _tcslen(portToken));

			/* copy com port container id */
			for (DWORD i = 0; i<_tcslen(containerID); ++i) {
				comShadowPortArray[dwComShadowCount].id[i] = containerID[i];
				//hComData.id[i] = containerID[i];
			}

			memset(comShadowPortArray[dwComShadowCount].port, 0, 128);
			_tcsncpy_s(comShadowPortArray[dwComShadowCount].port, portName, _tcslen(portName));
			comShadowPortArray[dwComShadowCount].timestamp = milliSeconds;
			//_tcsncpy_s(hComData.port, portName, _tcslen(portName));
			
			_tprintf(_T("[%d] COM name: %s detail: %d,%s container id: %d,%s, ts %I64d\n"), dwComShadowCount, portToken, _tcslen(portName), portName, _tcslen(containerID), containerID, comShadowPortArray[dwComShadowCount].timestamp);
			dwComShadowCount++;
			if (iDebug) {
				_tcsncpy_s(portName, portToken, _tcslen(portToken));
				_tprintf(_T("Serial Port: %s\n"), portName);
				TCHAR compositeDevicePath[MAX_PATH];
				GetCompositeDevicePath(hDeviceInfo, &deviceInfoData, compositeDevicePath);
				_tprintf(_T("Associated USB Composite Device: %s\n"), compositeDevicePath);
				TCHAR compositeDeviceInstanceId[MAX_PATH];
				GetCompositeDeviceInstanceId(hDeviceInfo, &deviceInfoData, compositeDeviceInstanceId);
				_tprintf(_T("Associated USB Composite Device ID: %s\n"), compositeDeviceInstanceId);
				printf("\r\n");
			}
		}

	}

	_tprintf(_T("COM port number %d->%d\r\n"), dwComCount, dwComShadowCount);

	if (dwComCount == 0) {
		memcpy(comPortArray, comShadowPortArray, sizeof(comShadowPortArray));
		dwComCount = dwComShadowCount;
		for (DWORD i = 0; i < dwComShadowCount; i++)
		{
			ComMessage * comMessage = new ComMessage();
			comMessage->mComString.Format(TEXT("%s"), comShadowPortArray[i].name);
			comMessage->mString.Format(TEXT("发现串口"));
			comMessage->mType = 0;
			CComHelperDlg::PostUserMessage(hWin, comMessage);
		}

	}
	else {
		/* Find the com port removed */
		for (DWORD i = 0; i < dwComCount; i++) {
			int dwFound = -1;
			for (DWORD j = 0; j < dwComShadowCount; j++) {
				if (_tcscmp(comPortArray[i].name, comShadowPortArray[j].name) == 0)
				{
					dwFound = j;
					break;
				}
			}
			if (dwFound == -1)
			{
				ComMessage * comMessage = new ComMessage();
				comMessage->mComString.Format(TEXT("%s"), comPortArray[i].name);
				comMessage->mString.Format(TEXT("串口拔出"));
				comMessage->mType = 0;
				CComHelperDlg::PostUserMessage(hWin, comMessage);
			}
		}

		for (DWORD i = 0; i < dwComShadowCount; i++)
		{
			int dwFound = -1;
			for (DWORD j = 0; j < dwComCount; j++)
			{
				if (_tcscmp(comPortArray[j].name, comShadowPortArray[i].name) == 0)
				{
					dwFound = j;
					break;
				}
			}
			if (dwFound >= 0)
			{
				comShadowPortArray[i].timestamp = comPortArray[dwFound].timestamp;
				_tprintf(_T("Found old COM port %s ts %I64d\r\n"), comShadowPortArray[i].name,comPortArray[dwFound].timestamp);
			}
			else {
				ComMessage * comMessage = new ComMessage();
				comMessage->mComString.Format(TEXT("%s"), comShadowPortArray[i].name);
				comMessage->mString.Format(TEXT("新串口插入"));
				comMessage->mType = 0;
				CComHelperDlg::PostUserMessage(hWin, comMessage);
			}
		}
		memcpy(comPortArray, comShadowPortArray, sizeof(comShadowPortArray));
		dwComCount = dwComShadowCount;
	}
	// 释放设备信息集合
	SetupDiDestroyDeviceInfoList(hDeviceInfo);

	return 0;
}
