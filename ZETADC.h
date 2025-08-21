#pragma once

#include <Windows.h>
#include <SetupAPI.h>
#include <winusb.h>
#include <tchar.h>
#include <initguid.h>
#include <cfgmgr32.h> // Для CM_Get_Parent
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
#include <cfgmgr32.h> // Для CM_Get_Parent

//	Find all modules of ADC and make list of VID, PID, bus, port
//return number of modules or -X
// if return < 0, this is code error
//		return code error
//	- 1 Ошибка инициализации libusb
//	- 2 Не удалось получить список устройств
//	- 3 Не удалось отключиться от драйвера ядра
//	- 4 не может захватить интерфейсе
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
//		-9	numModule >= максимально допустимого количества модулей
int init(int numModule);
//return code error
//		-1	thread is running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-5	Unknown exeption
//		-8	numModule < 0
//		-9	numModule >= максимально допустимого количества модулей
int startADC(int numModule);
//return code error
//		-1	thread is not running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-8	numModule < 0
//		-9	numModule >= максимально допустимого количества модулей
int stopADC(int numModule);
// прочитать указатель данных в кольцевом буфере
long long getPointerADC(int numModule);
// взять данные АЦП из кольцевого буфера по указанному модулю и указанному каналу
// массив чисел в плавающей запятой. целочисленные данные от АЦП умножаются на вес младшего разряда, чувствительность по каналу и пр. и добавляется смещение
// возвращает количество прочитанных чисел или код ошибки
//  0 нормальное выполнение команды
//- 1 количество данных для передачи sizeBuffer
//- 2 количество данных для передачи больше 3 / 4 размера кольцевого буфера
//- 3 номер канала channel меньше 0
//- 4 номер канала channel больше количества каналов NUMBEROFCHANNELS в модуле
//- 5 канал не включен.Параметр ISCHANNEL равен 0
//- 6 указатель pointer больше внутреннего указателя кольцевого буфера
//- 7 указатель pointer меньше внутреннего указателя кольцевого буфера - размер кольцевого буфера
int getDataADC(int numModule, float* buffer, int sizeBuffer, long long pointer, int channel);
enum zet {
	VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML,								// Integer
	FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,				// Float
	SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC,
	NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE,				// char
};
// взять целочисленные параметры
// параметры VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML
//Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
//- 1 номер модуля numModule меньше 0
//- 2 нет подключенных модулей
//- 3 номер модуля numModule больше количества поключенных модулей
//- 4 номер канала channel меньше 0
//- 5 номер канала channel больше количества калалов в модуле
int getInt(int numModule, int channel, int param);
// взять параметры в плавающей запятой
// параметры - FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,
			//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
float getFloat(int numModule, int channel, int param);
// взять строковые параметры 
// параметры NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE
char* getString(int numModule, int channel, int param);
// поместить в buffer xml файл, size - размер буфера
// возвращает реальный размер xml файла или код ошибки
int getXML(int numModule, char* buffer, int size);
// задать целочисленный параметр
int putInt(int numModule, int channel, int param, int val);	//VID = 100, PID, BUS, PORT, ISCHANNEL,
// задать параметр в плавающей запятой
int putFloat(int numModule, int channel, int param, float val); 		//FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,			// Float
// задать строковый параметр в формате UTF-8
int putString(int numModule, int channel, int param, char* val); //NAMEMOD = 300, TYPEMOD, SERIALMOD, NAMECHANNEL, UNITS, COMMENT, UNITSENSE														//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
// сформировать из заданных параметров XML файл и записать его в устройство
int putXML(int numModule);
