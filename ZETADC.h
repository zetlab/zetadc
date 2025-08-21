#pragma once

#include <Windows.h>
#include <SetupAPI.h>
#include <winusb.h>
#include <tchar.h>
#include <initguid.h>
#include <cfgmgr32.h> // ��� CM_Get_Parent
#include <Windows.h>
#include <SetupAPI.h>
#include <winusb.h>
#include <usb.h>
#include <string>
#include <iostream>
#include <thread>
#include "libusb.h"
#include <initguid.h>
#include <array>
#include <cfgmgr32.h> // ��� CM_Get_Parent

//	Find all modules of ADC and make list of VID, PID, bus, port
//return number of modules or -X
// if return < 0, this is code error
//		return code error
//	- 1 ������ ������������� libusb
//	- 2 �� ������� �������� ������ ���������
//	- 3 �� ������� ����������� �� �������� ����
//	- 4 �� ����� ��������� ����������
int modules();
//	For initialisation of module ADC
//return code error
//		-1	init error error in libusb_init()
//		-2	Device not found in libusb_open_device_with_vid_pid()
//		-3	Detach error in libusb_detach_kernel_driver()
//		-4	Claim error in libusb_claim_interface()
//		-5	Size of config.xml larger then driver buffer
//		-6	Size of config.xml larger then driver buffer
//		-7	Pid not found or == 0
//		-8	numModule < 0
//		-9	numModule >= ����������� ����������� ���������� �������
int init(int numModule);
//return code error
//		-1	thread is running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-5	Unknown exeption
//		-8	numModule < 0
//		-9	numModule >= ����������� ����������� ���������� �������
int startADC(int numModule);
//return code error
//		-1	thread is not running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-8	numModule < 0
//		-9	numModule >= ����������� ����������� ���������� �������
int stopADC(int numModule);
// ��������� ��������� ������ � ��������� ������
long long getPointerADC(int numModule);
// ����� ������ ��� �� ���������� ������ �� ���������� ������ � ���������� ������
// ������ ����� � ��������� �������. ������������� ������ �� ��� ���������� �� ��� �������� �������, ���������������� �� ������ � ��. � ����������� ��������
// ���������� ���������� ����������� ����� ��� ��� ������
//  0 ���������� ���������� �������
//- 1 ���������� ������ ��� �������� sizeBuffer
//- 2 ���������� ������ ��� �������� ������ 3 / 4 ������� ���������� ������
//- 3 ����� ������ channel ������ 0
//- 4 ����� ������ channel ������ ���������� ������� NUMBEROFCHANNELS � ������
//- 5 ����� �� �������.�������� ISCHANNEL ����� 0
//- 6 ��������� pointer ������ ����������� ��������� ���������� ������
//- 7 ��������� pointer ������ ����������� ��������� ���������� ������ - ������ ���������� ������
int getDataADC(int numModule, float* buffer, int sizeBuffer, long long pointer, int channel);
enum zet {
	VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML,								// Integer
	FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,				// Float
	SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC,
	NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE,				// char
};
// ����� ������������� ���������
// ��������� VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML
//���������� ������������ ����� ��� 0 ��� ���������� ���������� ��� ���������� ��� ������, ������� ������ 0.
//- 1 ����� ������ numModule ������ 0
//- 2 ��� ������������ �������
//- 3 ����� ������ numModule ������ ���������� ����������� �������
//- 4 ����� ������ channel ������ 0
//- 5 ����� ������ channel ������ ���������� ������� � ������
int getInt(int numModule, int channel, int param);
// ����� ��������� � ��������� �������
// ��������� - FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,
			//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
float getFloat(int numModule, int channel, int param);
// ����� ��������� ��������� 
// ��������� NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE
char* getString(int numModule, int channel, int param);
// ��������� � buffer xml ����, size - ������ ������
// ���������� �������� ������ xml ����� ��� ��� ������
int getXML(int numModule, char* buffer, int size);
// ������ ������������� ��������
int putInt(int numModule, int channel, int param, int val);	//VID = 100, PID, BUS, PORT, ISCHANNEL,
// ������ �������� � ��������� �������
int putFloat(int numModule, int channel, int param, float val); 		//FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,			// Float
// ������ ��������� �������� � ������� UTF-8
int putString(int numModule, int channel, int param, char* val); //NAMEMOD = 300, TYPEMOD, SERIALMOD, NAMECHANNEL, UNITS, COMMENT, UNITSENSE														//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
// ������������ �� �������� ���������� XML ���� � �������� ��� � ����������
int putXML(int numModule);
