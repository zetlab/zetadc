#define _AFXDLL
#pragma once
#ifndef ZETUSB_H
#define ZETUSB_H

#ifdef ZETUSB_EXPORTS
#define ZETUSB_API __declspec(dllexport) // Экспорт функции
#else
#define ZETUSB_API __declspec(dllimport) // Импорт функции
#endif
#include <objidl.h>
#include <oleidl.h>
#include <ocidl.h>

//	Find all modules of ADC and make list of VID, PID, bus, port
//return number of modules or -X
// if return < 0, this is code error
//		return code error
extern "C" ZETUSB_API int zet_modules();
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
extern "C" ZETUSB_API int zet_init(int numMod);
//return code error
//		-1	thread is running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-5	Unknown exeption
//		-8	numModule < 0
//		-9	numModule >= максимально допустимого количества модулей
extern "C" ZETUSB_API int zet_startADC(int numMod);
//return code error
//		-1	thread is not running
//		-2	error in libusb_bulk_transfer
//		-3	error in libusb_bulk_transfer
//		-8	numModule < 0
//		-9	numModule >= максимально допустимого количества модулей
extern "C" ZETUSB_API int zet_stopADC(int numMod);
// прочитать указатель данных в кольцевом буфере
extern "C" ZETUSB_API long long zet_getPointerADC(int numMod);
// взять данные АЦП из кольцевого буфера по указанному модулю и указанному каналу
// массив чисел в плавающей запятой. целочисленные данные от АЦП умножаются на вес младшего разряда, чувствительность по каналу и пр. и добавляется смещение
// возвращает количество прочитанных чисел или код ошибки
extern "C" ZETUSB_API int zet_getDataADC(int numMod, float* buffer, int sizeBuffer, long long pointer, int channel);
enum zet {
	VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML,								// Integer
	FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,				// Float
	SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC,
	NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE,				// char
};
// взять целочисленные параметры
// параметры VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML
extern "C" ZETUSB_API int zet_getInt(int numModule, int channel, int param);
// взять параметры в плавающей запятой
// параметры - FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,
			//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
extern "C" ZETUSB_API float zet_getFloat(int numModule, int channel, int param);
// взять строковые параметры 
// параметры NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE
extern "C" ZETUSB_API char* zet_getString(int numModule, int channel, int param);
// поместить в buffer xml файл, size - размер буфера
// возвращает реальный размер xml файла или код ошибки
extern "C" ZETUSB_API int zet_getXML(int numMod, char* buffer, int sizeBuffer);
// задать целочисленный параметр
extern "C" ZETUSB_API int zet_putInt(int numModule, int channel, int param, int val);
// задать параметр в плавающей запятой
extern "C" ZETUSB_API int zet_putFloat(int numModule, int channel, int param, float val);
// задать строковый параметр в формате UTF-8
extern "C" ZETUSB_API int zet_putString(int numModule, int channel, int param, char* val);
// сформировать из заданных параметров XML файл и записать его в устройство
extern "C" ZETUSB_API int zet_putXML(int numMod);

#endif
