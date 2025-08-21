//#ifdef USE_PCH
#include ".\dllLIB\pch.h"
//#endif
#include <Windows.h> // Необходимо для OutputDebugString
#include <objidl.h>
#include <oleidl.h>
#include <ocidl.h>
#include <atlbase.h>
#include <atlcom.h>

#include "ZETADC.h"

#include "pugixml.hpp"
#include "pugixml.cpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <locale>
#include <cstdlib>
#include <array>
#include <string>
#include <format>
#include <atlstr.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
//using namespace rapidxml;

// GUID для USB устройств
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
    0xA5DCBF10, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
const int ENDPOINT_OUT = 0x1;
const int ENDPOINT_IN = 0x81;
const int ENDPOINT_ADC = 0x82;
const int BUFFER_SIZE = 512;// Размер буфера
const int ringSize = 2 * 3 * 2 * 5 * 2 * 7 * 3 * 11 * 13 * 2;
const int RECEIVE_SIZE = BUFFER_SIZE * 64;
const int XML_SIZE = 100000;

struct ZetADC {
	char AdcName[128];
	char AdcType[128];
	char AdcSerial[128];
	char description[128];
	char ethMethod[128];
	char ethAddr[128];
	char ethFTP[128];
	char idChannel[24][128];
	char nameChannel[24][128];
	char unitsChannel[24][128];
	char commentChannel[24][128];
	char unitsenseChannel[24][128];
	int numberOfChannels = -1;
	int numberOfActiveC = -1;
	int numberOfDiffChannels = -1;
	int numberOfDiffActive = -1;
	int numberOfDACChannels = -1;
	int numberOfDACActive = -1;
	std::array<double, 24> digResADC;
	std::array<double, 24> digResDiff;
	std::array<double, 2> digResDAC;
	std::array<int, 24> kodAmpl;
	std::array<double, 24> sense;
	std::array<double, 24> shift;
	std::array<double, 24> amplify;
	std::array<double, 24> reference;
	std::array<int, 24> HPF;
	std::array<int, 24> isChannel;
	std::array<int, 24> isChannelHCP;
	std::array<int, 24> isChannelDiff;
	std::array<int, 24> isChannelDAC;
	std::array<double, 24> koeffChannel;
	std::array<double, 24> shiftChannel;
	std::array<double, 24> koeffChannelDiff;
	std::array<double, 2> koeffChannelDAC;
	double freqADC = -1;
	double freqDAC = -1;
	long long DigitalOutput = 0;
	long long DigitalOutEnable = 0;
	int significant = -1;
};

struct ZetData {
//	libusb_device_handle* dev_handle = nullptr;
	int running = 0;	// поток сбора данных в циклический буфер
	unsigned char buffer[BUFFER_SIZE];
	pugi::xml_document doc;
	unsigned char receive[RECEIVE_SIZE];	// пакетный блок xml
	int pointer = 0;
	int sizeOst = 0;
	unsigned char ostatok[RECEIVE_SIZE];
	int16_t* ost16 = (int16_t*)ostatok;
	int32_t* ost32 = (int32_t*)ostatok;
	int64_t* ost64 = (int64_t*)ostatok;
	char fullText[XML_SIZE];
	char copyText[XML_SIZE];
	int pointerText = 0;
	int oborot = 0;
	int pointADC = 0;
	float dataADC[ringSize + BUFFER_SIZE];
};

struct ZetInfo {
	libusb_device_handle* handle;
	struct libusb_device_descriptor descriptor;
	struct libusb_device *device;
	int VID;
	int PID;
	int port;
	int bus;
};

static ZetADC zm[4];
static ZetData zd[4];
static ZetInfo zi[4];
static int numberOfModules = -1;

void threadFunction(int numModule);
int packageStream(int numModule, int actLength, int* pointer);
int calcChan16(unsigned char* packet, int sizePacket, int numModule);
float parsePack16(unsigned char* pack, int index, ZetADC* zetmod);
int calcChan24(unsigned char* packet, int sizePacket, int numModule);
float parsePack24(unsigned char* pack, int index, ZetADC* zetmod);
int putParam(ZetADC* zetmod, ZetData* zdata);
//------------------------------------------------------------------------------------------
int modules() {
	libusb_context* ctx = nullptr;
	libusb_device** device_list = nullptr;
	ssize_t count;
	int err = 0;
	wchar_t debugMessage[256];

	numberOfModules = -2;
	// Инициализация libusb
	if (libusb_init(&ctx) < 0) {
		std::cerr << "Ошибка инициализации libusb" << std::endl;
		OutputDebugString(L"Ошибка инициализации libusb\n");
		return -1;
	}

	// Получение списка устройств
	count = libusb_get_device_list(ctx, &device_list);
	if (count < 0) {
		std::cerr << "Не удалось получить список устройств" << std::endl;
		OutputDebugString(L"Не удалось получить список устройств\n");
		libusb_exit(ctx);
		return -2;
	}

	swprintf_s(debugMessage, L"Найдено устройство %zd\n", count);
	OutputDebugString(debugMessage);
	std::cout << debugMessage << std::endl;

	// Перебор устройств
	int number = 0;
	for (ssize_t i = 0; i < count; ++i) {
		libusb_device* dev = device_list[i];
		libusb_device_descriptor desc;

		if (libusb_get_device_descriptor(dev, &desc) == 0) {
			if (desc.idVendor == 0x2ffd) {
				uint8_t bus = libusb_get_bus_number(dev);
				uint8_t port = libusb_get_port_number(dev);
//				wchar_t debugMessage[256];
				swprintf_s(debugMessage, L"Устройство = %zd, VID = %x, PID = %x, Serial = %d, bus = %d, port = %d\n", i, desc.idVendor, desc.idProduct, desc.iSerialNumber, bus, port);
				OutputDebugString(debugMessage);
				std::cout << debugMessage << std::endl;
				zi[number].VID = desc.idVendor;
				zi[number].PID = desc.idProduct;
				zi[number].bus = bus;
				zi[number].port = port;
				zi[number].descriptor = desc;
				zi[number].device = dev;
				zi[number].handle = NULL;
				err = libusb_open(dev, &zi[number].handle);
				if(err != LIBUSB_SUCCESS)
				{
					return err;
				}
				// Отсоединяем драйвер ядра (если нужно)
				if ((err = libusb_kernel_driver_active(zi[number].handle, 0)) == 1) {
					err = libusb_detach_kernel_driver(zi[number].handle, 0);
					if (err < 0) {
						swprintf_s(debugMessage, L"Detach error: %S\n", libusb_error_name(err));
						OutputDebugString(debugMessage);
						std::cout << debugMessage << std::endl;
						libusb_close(zi[number].handle);
						libusb_exit(nullptr);
						return-3;
					}
					swprintf_s(debugMessage, L"libusb_detach_kernel_driver = %d\n", err);
					OutputDebugString(debugMessage);
					std::cout << debugMessage << std::endl;
				}
				else {
					swprintf_s(debugMessage, L"libusb_kernel_driver_active = %d\n", err);
					OutputDebugString(debugMessage);
					std::cout << debugMessage << std::endl;
				}
				// Захватываем интерфейс
				err = libusb_claim_interface(zi[number].handle, 0);
				if (err < 0) {
					swprintf_s(debugMessage, L"Claim error: %S\n", libusb_error_name(err));
					OutputDebugString(debugMessage);
					std::cout << debugMessage << std::endl;
					libusb_close(zi[number].handle);
					libusb_exit(nullptr);
					return-4;
				}
				swprintf_s(debugMessage, L"libusb_claim_interface = %S\n", libusb_error_name(err));
				OutputDebugString(debugMessage);
				std::cout << debugMessage << std::endl;
				number++;
				if(number >= sizeof(zi) / sizeof(zi[0])) break;
			}
		}
	}
	numberOfModules = number;
	// Очистка
	libusb_free_device_list(device_list, 1);
	return number;
}
//--------------------------------------------------------------------------------------------
int getParam(ZetADC* zetmod, ZetData* zdata)
{
	wchar_t debugMessage[256];

	zetmod->digResADC.fill(1.0);
	zetmod->digResDiff.fill(1.0);
	zetmod->digResDAC.fill(1.0);
	zetmod->kodAmpl.fill(0);
	zetmod->sense.fill(1.0);
	zetmod->shift.fill(0.0);
	zetmod->amplify.fill(1.0);
	zetmod->reference.fill(1.0);
	zetmod->isChannel.fill(1);
	zetmod->isChannelDiff.fill(0);
	zetmod->isChannelDAC.fill(1);
	zetmod->koeffChannel.fill(1.0);
	zetmod->shiftChannel.fill(0.0);
	zetmod->koeffChannelDiff.fill(1.0);
	zetmod->koeffChannelDAC.fill(1.0);

	int err = 0;

   // Загружаем XML в PugiXML
//	static xml_document<> doc;
//	doc.parse<0>(&content[0]);
//	static unsigned char copyText[100000];
	memcpy(zdata->copyText, zdata->fullText, sizeof(zdata->fullText)+1);
//	zdata->doc.parse<0>((char*)copyText);
	pugi::xml_parse_result result = zdata->doc.load_buffer(zdata->copyText, sizeof(zdata->copyText));// , pugi::parse_default | pugi::parse_trim_pcdata));
	if (!result) {
		std::cerr << "Ошибка парсинга XML: " << result.description() << std::endl;
		swprintf_s(debugMessage, L"Ошибка парсинга XML = %S\n", result.description());
		OutputDebugString(debugMessage);
		return -1;
	}
	pugi::xml_node config = zdata->doc.child("Config");
	if (config == 0) return -2;
	std::string version = config.attribute("version").as_string();
	std::cout << "Config version = " << version << std::endl;
	swprintf_s(debugMessage, L"Config version =  %S\n", version.c_str());
	OutputDebugString(debugMessage);
	pugi::xml_node deviceNode = config.child("Device");
	std::string name = deviceNode.attribute("name").as_string();
	std::string type = deviceNode.attribute("type").as_string();
	std::string serial = deviceNode.attribute("serial").as_string();
	memcpy(zetmod->AdcName, name.c_str(), sizeof(name));
	memcpy(zetmod->AdcType, type.c_str(), sizeof(type));
	memcpy(zetmod->AdcSerial, serial.c_str(), sizeof(serial));
	// Пустой узел <Description>
	std::string descLabel = deviceNode.child("Description").attribute("label").as_string();
	std::cout << "  Description label = " << descLabel << std::endl;
	swprintf_s(debugMessage, L"Description label = %S\n", descLabel.c_str());
	OutputDebugString(debugMessage);
	memcpy(zetmod->description, descLabel.c_str(),sizeof(descLabel));
	// Узел <Ethernet>
	pugi::xml_node eth = deviceNode.child("Ethernet");
	if (eth != nullptr) {
		std::string method = eth.attribute("method").as_string();
		std::string addr = eth.attribute("addr").as_string();
		std::string ftp = eth.attribute("ftp").as_string();
		std::cout << "  Ethernet: method=" << method
			<< ", addr=" << addr
			<< ", ftp=" << ftp << std::endl;
		memcpy(zetmod->ethMethod, method.c_str(), sizeof(method));
		memcpy(zetmod->ethAddr, addr.c_str(), sizeof(addr));
		memcpy(zetmod->ethFTP, ftp.c_str(), sizeof(ftp));
	}
	// Узел <DigitalResolChanADC> — список чисел
	std::string listText = deviceNode.child("DigitalResolChanADC").child_value();
	if(listText != "")
	{
		std::stringstream ss(listText);
		std::string item;
		int index = 0;
		while (std::getline(ss, item, ',')) {
			zetmod->digResADC[index] = std::stod(item);
			index++;
		}
	}
	// Узел <DigitalResolDiffADC> — список чисел
	listText = deviceNode.child("DigitalResolDiffADC").child_value();
	if (listText != "")
	{
		std::stringstream ss(listText);
		std::string item;
		int index = 0;
		while (std::getline(ss, item, ',')) {
			zetmod->digResDiff[index] = std::stod(item);
			index++;
		}
	}
	// Узел <DigitalResolChanDAC> — список чисел
	listText = deviceNode.child("DigitalResolChanDAC").child_value();
	if (listText != "")
	{
		std::stringstream ss(listText);
		std::string item;
		int index = 0;
		while (std::getline(ss, item, ',')) {
			zetmod->digResDAC[index] = std::stod(item);
			index++;
		}
	}

	// извлекаем KodAmplify
	listText = deviceNode.child("KodAmplify").child_value();
	if (listText != "")
	{
		std::stringstream ss(listText);
		std::string item;
		int index = 0;
		while (std::getline(ss, item, ',')) {
			zetmod->kodAmpl[index] = std::atoi(item.c_str());
			index++;
		}
	}
	// Извлекаем Sense, Shift, Reference, Amplify для каждого канала
	pugi::xml_node nodeChannels = deviceNode.child("Channels");
	int index = 0;
	for (pugi::xml_node ch : nodeChannels.children("Channel")) {
		int         id = ch.attribute("id").as_int();

		memcpy(zetmod->idChannel[id], ch.attribute("id").as_string(), sizeof(ch.attribute("id").as_string()));
		memcpy(zetmod->nameChannel[id], ch.attribute("name").as_string(), sizeof(ch.attribute("name").as_string()));
		memcpy(zetmod->unitsChannel[id], ch.attribute("units").as_string(), sizeof(ch.attribute("units").as_string()));
		memcpy(zetmod->unitsenseChannel[id], ch.attribute("unitsense").as_string(), sizeof(ch.attribute("unitsense").as_string()));

		zetmod->sense[id] = ch.child("Sense").text().as_double();
		zetmod->shift[id] = ch.child("Shift").text().as_double();
		zetmod->reference[id] = ch.child("Reference").text().as_double();
		zetmod->amplify[id] = ch.child("Amplify").text().as_double();
		zetmod->HPF[id] = ch.child("HPF").text().as_int();
		if (zetmod->kodAmpl[index] == 0) zetmod->kodAmpl[index] = 1;
		else zetmod->kodAmpl[index] = 30;
		zetmod->shiftChannel[index] = zetmod->shift[index];
		zetmod->koeffChannel[index] = (double)(zetmod->digResADC[index] / zetmod->amplify[index] / zetmod->kodAmpl[index] / (double)zetmod->sense[index]);
		index++;
	}
	zetmod->numberOfChannels = index;

	// Прочие одиночные элементы
	zetmod->freqADC = deviceNode.child("Freq").text().as_double();
	zetmod->freqDAC = deviceNode.child("FreqDAC").text().as_double();

	const char* channel = deviceNode.child("Channel").child_value();
	long long chSin = 1;
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = std::stoll(channel, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << channel << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	zetmod->numberOfActiveC = 0;
	for (int i = 0; i < zetmod->numberOfChannels; i++)
	{
		if ((chSin & 0x1) != 0)
		{
			zetmod->isChannel[i] = 1;
			zetmod->numberOfActiveC++;
		}
		else
		{
			zetmod->isChannel[i] = 0;
		}
		chSin = chSin >> 1;
	}

	const char* hcpChannel = deviceNode.child("HCPChannel").child_value();
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = 0;
		chSin = std::stoll(hcpChannel, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << hcpChannel << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	for (int i = 0; i < zetmod->numberOfChannels; i++)
	{
		if ((chSin & 0x1) != 0)
		{
			zetmod->isChannelHCP[i] = 1;
		}
		else
		{
			zetmod->isChannelHCP[i] = 0;
		}
		chSin = chSin >> 1;
	}

	const char* ChannelDiff = deviceNode.child("ChannelDiff").child_value();
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = 0;
		chSin = std::stoll(ChannelDiff, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << ChannelDiff << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	for (int i = 0; i < zetmod->numberOfChannels; i++)
	{
		if ((chSin & 0x1) != 0)
		{
			zetmod->isChannelDiff[i] = 1;
		}
		else
		{
			zetmod->isChannelDiff[i] = 0;
		}
		chSin = chSin >> 1;
	}

	const char* ChannelDAC = deviceNode.child("ChannelDAC").child_value();
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = 0;
		chSin = std::stoll(ChannelDAC, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << ChannelDAC << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	for (int i = 0; i < zetmod->numberOfChannels; i++)
	{
		if ((chSin & 0x1) != 0)
		{
			zetmod->isChannelDAC[i] = 1;
		}
		else
		{
			zetmod->isChannelDAC[i] = 0;
		}
		chSin = chSin >> 1;
	}

	const char* DigitalOutput = deviceNode.child("DigitalOutput").child_value();
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = 0;
		chSin = std::stoll(DigitalOutput, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << DigitalOutput << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	zetmod->DigitalOutput = chSin;

	const char* DigitalOutEnable = deviceNode.child("DigitalOutEnable").child_value();
	try {
		// base = 0 → autodetect: при префиксе "0x" считает как hex
		chSin = 0;
		chSin = std::stoll(DigitalOutEnable, nullptr, 0);
		std::cout << "Parsed: " << chSin << "\n";  // 65535
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Не число: " << DigitalOutEnable << "\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Выход за диапазон\n";
	}
	zetmod->DigitalOutEnable = chSin;

	return err;
}
//------------------------------------------------------------------------------------------------------------------------
int init(int numModule)
{
	int err;
	int pointer = 0;
	int realSize = 0;
	int textSize = 0;
	int result = 0;
	int actual_length;
	unsigned int TIMEOUT_MS = 20;// Таймаут в миллисекундах
	wchar_t debugMessage[256];

	if (numModule < 0) return -8;
	if (numModule >= sizeof(zd) / sizeof(zd[0])) return -9;

	unsigned char data[28] = { 0x1c, 0x00, 0x50, 0x00, 0x46, 0x4f, 0x08, 0x00, 0x08, 0x00, 0x09, 0x00, 0x4c, 0x4f, 0x41, 0x44, 0x63, 0x6f, 0x6e, 0x66, 0x2e, 0x78, 0x6d, 0x6c, 0x00, 0x00, 0x00, 0x00 };
	unsigned char xmlpacket[16] =		{ 0x10, 0x01, 0x50, 0x00, 0x46, 0x44, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x01 };
	//										size+16	even											number							size
	unsigned char dummyPacketI[16] =	{ 0x10, 0x00, 0x50, 0x00, 0x46, 0x44, 0x08, 0x00, 0x17, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
																						//the whole size little then great
	unsigned char dummyPacketII[16] =	{ 0x10, 0x00, 0x50, 0x00, 0x46, 0x52, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	actual_length = 1;
	while (actual_length != 0) {		// вычитываем если есть остаток
		result = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_IN,        // Адрес endpoint для чтения
			zd[numModule].buffer,             // Буфер для данных
			BUFFER_SIZE,        // Размер буфера
			&actual_length,     // Указатель для количества прочитанных байт
			TIMEOUT_MS          // Таймаут
		);
	}


	// Отправка данных (endpoint 0x01)
	err = libusb_bulk_transfer(
		zi[numModule].handle,
		ENDPOINT_OUT,          // Endpoint OUT
		data,
		sizeof(data),
		&actual_length,
		100            // Таймаут в ms
	);

	if (err < 0) {
		swprintf_s(debugMessage, L"Write error: %S\n", libusb_error_name(err));
		OutputDebugString(debugMessage);
	}
	else {
		swprintf_s(debugMessage, L"Sent %d bytes\n", actual_length);
		OutputDebugString(debugMessage);
	}
	Sleep(20);

	actual_length = 0;
	while (actual_length >= 0)
	{
		result = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_IN,        // Адрес endpoint для чтения
			zd[numModule].buffer,             // Буфер для данных
			BUFFER_SIZE,        // Размер буфера
			&actual_length,     // Указатель для количества прочитанных байт
			TIMEOUT_MS          // Таймаут
		);
		//		Sleep(100);
				// 4. Обработайте результат
		if (result == LIBUSB_SUCCESS) {
			if (actual_length > 0) {
				if (pointer + actual_length >= (int)sizeof(zd[numModule].receive)) return -5;// Size of config.xml larger then driver buffer
				memcpy(&zd[numModule].receive[pointer], zd[numModule].buffer, actual_length);
				pointer += actual_length;
			}
		}
		else
		{
			swprintf_s(debugMessage, L"Ошибка чтения: %S\n", libusb_error_name(result));
			OutputDebugString(debugMessage);
			// Обработка специфических ошибок
			if (result == LIBUSB_ERROR_TIMEOUT)
			{
				swprintf_s(debugMessage, L"Таймаут! Устройство не ответило вовремя\n");
				OutputDebugString(debugMessage);
			}
			else if (result == LIBUSB_ERROR_PIPE)
			{
				swprintf_s(debugMessage, L"Неправильный endpoint\n");
				OutputDebugString(debugMessage);
			}
			actual_length = -1;
		}
	}
	zd[numModule].pointer = pointer;
	int ukaz = 0;
	int pointText = 0;	// выделяем текст xml из пакетов
	if (pointer > 0) {
		while (1)
		{
			realSize = (unsigned int)zd[numModule].receive[ukaz] + (unsigned int)zd[numModule].receive[ukaz + 1] * 256;
			textSize = (unsigned int)zd[numModule].receive[ukaz + 14] + (unsigned int)zd[numModule].receive[ukaz + 15] * 256;
			if (textSize > realSize) textSize = 0;
			swprintf_s(debugMessage, L"Прочитано realSize = %d, textSize = %d\n", realSize, textSize);
			OutputDebugString(debugMessage);
			if (pointText + textSize >= (int)sizeof(zd[numModule].fullText)) return -6;	// Size of config.xml larger then driver buffer
			memcpy(&zd[numModule].fullText[pointText], &zd[numModule].receive[ukaz + 16], textSize);
			swprintf_s(debugMessage, L"byte head packet = %d, textsize = %d\n", realSize, textSize);
			OutputDebugString(debugMessage);
/*			TRACE(L"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
				zd[numModule].receive[ukaz + 0],
				zd[numModule].receive[ukaz + 1],
				zd[numModule].receive[ukaz + 2],
				zd[numModule].receive[ukaz + 3],
				zd[numModule].receive[ukaz + 4],
				zd[numModule].receive[ukaz + 5],
				zd[numModule].receive[ukaz + 6],
				zd[numModule].receive[ukaz + 7],
				zd[numModule].receive[ukaz + 8],
				zd[numModule].receive[ukaz + 9],
				zd[numModule].receive[ukaz + 10],
				zd[numModule].receive[ukaz + 11],
				zd[numModule].receive[ukaz + 12],
				zd[numModule].receive[ukaz + 13],
				zd[numModule].receive[ukaz + 14],
				zd[numModule].receive[ukaz + 15]
				);*/
			ukaz += realSize;
			pointText += textSize;
//			zd[numModule].fulltext[pointText] = 0;
			if (ukaz >= pointer) break;
		}
		zd[numModule].pointerText = pointText;
	}


// Предположим, что fulltext — это UTF-8
	CStringA utf8str(reinterpret_cast<const char*>(zd[numModule].fullText)); // ANSI строка
	CStringW widestr = CStringW(CA2W(utf8str, CP_UTF8));       // UTF-8 → UTF-16

	OutputDebugStringW(widestr);
//	TRACE(L"textSize = %d\n", pointText);

	getParam(&zm[numModule],&zd[numModule]);
    return 0;
}
//----------------------------------------------------------------------------
void threadFunction(int numModule) {
	Sleep(20);
	int result = 0;
	unsigned int TIMEOUT_MS = 100;// Таймаут в миллисекундах
	int actual_length = 0;
	int pointer = 0;
	wchar_t debugMessage[256];

	while (zd[numModule].running == 1) {
		result = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_ADC,        // Адрес endpoint для чтения
			zd[numModule].receive,             // Буфер для данных
			BUFFER_SIZE*16,        // Размер буфера
			&actual_length,     // Указатель для количества прочитанных байт
			TIMEOUT_MS          // Таймаут
		);
		if (result == LIBUSB_SUCCESS) {
			if (actual_length > 0) {
				swprintf_s(debugMessage, L"LIBUSB_SUCCESS = %d\n", actual_length);
				OutputDebugString(debugMessage);
				packageStream(numModule, actual_length, &pointer);
			}
		}
		else
		{
			swprintf_s(debugMessage, L"Ошибка чтения: %S\n", libusb_error_name(result));
			OutputDebugString(debugMessage);
			// Обработка специфических ошибок
			if (result == LIBUSB_ERROR_TIMEOUT)
			{
				swprintf_s(debugMessage, L"Таймаут! Устройство не ответило вовремя\n");
				OutputDebugString(debugMessage);
			}
			else if (result == LIBUSB_ERROR_PIPE)
			{
				swprintf_s(debugMessage, L"Неправильный endpoint\n");
				OutputDebugString(debugMessage);
			}
			actual_length = -1;
		}
	}
	return;
}
//--------------------------------------------------------------------------------------------
int startADC(int numModule) {
	unsigned char data[12] = { 0x0c, 0x00, 0xaa, 0xaa, 0x53, 0x43, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00 };
	//stop	unsigned char data[12] = { 0x0c, 0x00, 047a, 0x00, 0x53, 0x43, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int actual_length;
	unsigned int TIMEOUT_MS = 20;// Таймаут в миллисекундах
	wchar_t debugMessage[256];

	if (numModule < 0) return -8;
	if (numModule >= sizeof(zd) / sizeof(zd[0])) return -9;
	if (zd[numModule].running == 1) return -1;
	zd[numModule].running = 1;
	zd[numModule].pointADC = 0;
	zd[numModule].oborot = 0;
	// Отправка данных (endpoint 0x01)
	int err = libusb_bulk_transfer(
		zi[numModule].handle,
		ENDPOINT_OUT,          // Endpoint OUT
		data,
		sizeof(data),
		&actual_length,
		TIMEOUT_MS            // Таймаут в ms
	);
	if (err < 0) {
		swprintf_s(debugMessage, L"libusb_bulk_transfer = %S\n", libusb_error_name(err));
		OutputDebugString(debugMessage);
		return -2;
	}
	err = libusb_bulk_transfer(
		zi[numModule].handle,
		ENDPOINT_IN,          // Endpoint IN
		data,
		sizeof(data),
		&actual_length,
		TIMEOUT_MS            // Таймаут в ms
	);
	if (err < 0) {
		swprintf_s(debugMessage, L"libusb_bulk_transfer = %S\n", libusb_error_name(err));
		OutputDebugString(debugMessage);
		return -3;
	}
	try
	{
		std::thread myThread(threadFunction, numModule); // Создание и запуск потока
		if (myThread.joinable()) {
			swprintf_s(debugMessage, L"joinable");
			OutputDebugString(debugMessage);
			myThread.detach(); // Ожидание отсоединения от потока
		}
	}
	catch(const std::exception& e)
	{
		swprintf_s(debugMessage, L"Unknown exeption %S\n", e.what());
		OutputDebugString(debugMessage);
		return -5;
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------
int stopADC(int numModule)
{
	unsigned char data[12] = { 0x0c, 0x00, 047, 0x00, 0x53, 0x43, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int actual_length;
	unsigned int TIMEOUT_MS = 100;// Таймаут в миллисекундах
	wchar_t debugMessage[256];

	if (numModule < 0) return -8;
	if (numModule >= sizeof(zd) / sizeof(zd[0])) return -9;
	if (zd[numModule].running == 0) return -1;
	// Отправка данных (endpoint 0x01)
	int err = libusb_bulk_transfer(
		zi[numModule].handle,
		ENDPOINT_OUT,          // Endpoint OUT
		data,
		sizeof(data),
		&actual_length,
		TIMEOUT_MS            // Таймаут в ms
	);
	if (err < 0) {
		swprintf_s(debugMessage, L"libusb_bulk_transfer = %S\n", libusb_error_name(err));
		OutputDebugString(debugMessage);
		return -2;
	}
	err = libusb_bulk_transfer(
		zi[numModule].handle,
		ENDPOINT_IN,          // Endpoint IN
		data,
		sizeof(data),
		&actual_length,
		TIMEOUT_MS            // Таймаут в ms
	);
	if (err < 0) {
		swprintf_s(debugMessage, L"libusb_bulk_transfer = %S\n", libusb_error_name(err));
		OutputDebugString(debugMessage);
		return -3;
	}
	Sleep(100);
	zd[numModule].running = 0;
	libusb_close(zi[numModule].handle);
	libusb_exit(nullptr);
	return 0;
}
//-----------------------------------------------------------------------------------------------------
//int packageStream(unsigned char* data, int actLength, float* dataBuff, int* pointer, ZetADC* zetmod)
int packageStream(int numModule, int actLength, int* pointer)
{
	int err = 0;
	uint8_t* data8 = (uint8_t*)zd[numModule].receive;
	uint16_t* data16 = (uint16_t*)zd[numModule].receive;
	uint32_t* data32 = (uint32_t*)zd[numModule].receive;
	uint64_t* data64 = (uint64_t*)zd[numModule].receive;
	uint64_t timeSec = 0;
	int indexTime = 0;
	int timeMoment = 0;
	int priznak = 0;
	wchar_t debugMessage[256];

	for (int i = 0; i < actLength / 4; i++)        // поиск времени секунды
	{
		if (data32[i] == 0xaaaa0010) {       //token + full size
			if (i <= (actLength / 4 - 4)) {
				if (data32[i + 1] == 0x00085453) {// root size +  ZSP_CODE_STREAM_TIME
					timeSec = data64[i / 2 + 1];
					indexTime = i;
				}
			}
		}
	}
	if (indexTime != 0)
	{
		swprintf_s(debugMessage, L"sizeOst = %d, index = %d\n", zd[numModule].sizeOst, indexTime);
		OutputDebugString(debugMessage);
	}

	int sdvig = 0;
	if (zd[numModule].sizeOst != 0)
	{        // Вначале - дописать хвост и обработать пакет
		unsigned int sizePack = zd[numModule].ost16[0];       // размер пакета
		if (sizePack <= RECEIVE_SIZE) 
		{
			int ukaz = sizePack - zd[numModule].sizeOst;      //остаток, который надо дописать
			int sizeDok = 0;                    // размер докачки
			if (zd[numModule].sizeOst <= actLength)
			{    // остаток, который надо докачать меньше пакета
				sizeDok = zd[numModule].sizeOst;           // докачка равна остатку
			}
			else 
			{                                  // остаток больше пакета
				sizeDok = actLength;          // докачка равна пакету
			}
			sdvig = zd[numModule].sizeOst;
			for (int i = 0; i < sizeDok; i++) 
			{
				zd[numModule].ostatok[ukaz] = data8[i];
				ukaz++;
			}
			zd[numModule].sizeOst = zd[numModule].sizeOst - sizeDok;
			if (zd[numModule].sizeOst == 0)
			{       // пакет набрался
				if (zm[numModule].significant == 2) calcChan16(zd[numModule].ostatok, sizePack, numModule);         //
				if (zm[numModule].significant == 3) calcChan24(zd[numModule].ostatok, sizePack, numModule);         //расчет пакета
			}
		}
	}
	if (zd[numModule].sizeOst == 0) {      // пакеты не надо добирать
		for (int i = sdvig / 4; i < actLength / 4; i++) {
			if ((data32[i] & 0xffff0000) == 0xaaaa0000) {
				swprintf_s(debugMessage, L"index before = %d\n", i * 4);
				OutputDebugString(debugMessage);
				if (i <= (actLength / 4 - 4))
				{
					if ((data32[i + 1] & 0xfffff0ff) == 0x00083049) {
						priznak = 0;
						if (data32[i + 1] == 0x00083249) priznak = 2; //это для 16 битного
						if (data32[i + 1] == 0x00083349) priznak = 3; //это для 24 битного
						if (data32[i + 1] == 0x00083449) priznak = 4; //это для 32 битного
						if (priznak != 0)
						{
							zm[numModule].significant = priznak;
							unsigned int sizePacket = data32[i] & 0xffff;
							if (sizePacket >= RECEIVE_SIZE) break;
							if ((int)(i + sizePacket / 4) > actLength / 4)
							{   // запоминаем остаток на следующий раз
								zd[numModule].sizeOst = i * 4 + sizePacket - actLength;
								int head = sizePacket - zd[numModule].sizeOst;        //голова, которую надо захватить
								for (int k = 0; k < head; k++)
								{
									zd[numModule].ostatok[k] = data8[i * 4 + k];
								}
							}
							else
							{
								swprintf_s(debugMessage, L"index = %d\n", i * 4);
								OutputDebugString(debugMessage);
								if (priznak == 2) calcChan16(&data8[i * 4], sizePacket, numModule);         //
								if (priznak == 3) calcChan24(&data8[i * 4], sizePacket, numModule);         //расчет пакета по обычному
								
							}
							i = i + sizePacket / 4 - 1;
						}
					}
				}
			}
			if ((i >= indexTime) && (timeMoment == 0)) timeMoment = *pointer;
		}
	}
	*pointer = zd[numModule].pointADC;
	return err;
}
//-----------------------------------------------------------------------------------------------------------------------
int calcChan16(unsigned char* packet, int sizePacket, int numModule) {
	int error = 0;
	int index = zd[numModule].pointADC;
	int cycle = 0;
	wchar_t debugMessage[256];

	swprintf_s(debugMessage, L"calcChan sizePacket = %d, numberOfActive = %d\n", sizePacket, zm[numModule].numberOfActiveC);
	OutputDebugString(debugMessage);
	for (int i = 0x10; i < sizePacket; i = i + zm[numModule].numberOfActiveC * 2) {
		cycle = 0;
		for (int k = 0; k < zm[numModule].numberOfChannels; k++)
		{
			if (zm[numModule].isChannel[k] != 0)
			{
				zd[numModule].dataADC[index] = parsePack16(&packet[i + cycle * 2], k, &zm[numModule]);
				index++;
				cycle++;
			}
		}
		if (index >= ringSize)
		{
			index = 0;
			zd[numModule].oborot++;
		}
	}
	swprintf_s(debugMessage, L"calcChan size = %d, val =  %7.2f, %7.2f, %7.2f, %7.2f\n", index - zd[numModule].pointADC, zd[numModule].dataADC[zd[numModule].pointADC],
		zd[numModule].dataADC[zd[numModule].pointADC + 1], zd[numModule].dataADC[zd[numModule].pointADC + 8], zd[numModule].dataADC[zd[numModule].pointADC + 9]);
	OutputDebugString(debugMessage);
	zd[numModule].pointADC = index;

	return error;
}
//--------------------------------------------------------------------------------------------------
int calcChan24(unsigned char* packet, int sizePacket, int numModule) {
	int index = zd[numModule].pointADC;
	int cycle = 0;
	wchar_t debugMessage[256];

	swprintf_s(debugMessage, L"calcChan sizePacket = %d, numberOfActive = %d\n", sizePacket, zm[numModule].numberOfActiveC);
	OutputDebugString(debugMessage);
	for (int i = 0x10; i < sizePacket; i = i + zm[numModule].numberOfActiveC * 3) {
		cycle = 0;
		for (int k = 0; k < zm[numModule].numberOfChannels; k++)
		{
			if (zm[numModule].isChannel[k] != 0)
			{
				zd[numModule].dataADC[index] = parsePack24(&packet[i + cycle * 3], k, &zm[numModule]);
				index++;
				cycle++;
			}
		}
		if (index >= ringSize)
		{
			index = 0;
			zd[numModule].oborot++;
		}
	}
	swprintf_s(debugMessage, L"calcChan size = %d, val = %7.2f, %7.2f, %7.2f, %7.2f\n", index - zd[numModule].pointADC, zd[numModule].dataADC[zd[numModule].pointADC],
		zd[numModule].dataADC[zd[numModule].pointADC + 1], zd[numModule].dataADC[zd[numModule].pointADC + 2], zd[numModule].dataADC[zd[numModule].pointADC + 3]);
	OutputDebugString(debugMessage);
	zd[numModule].pointADC = index;
	return 0;
}
//--------------------------------------------------------------------------------------------------
float parsePack16(unsigned char* pack, int index, ZetADC* zetmod) {
	float res = static_cast<float>((uint8_t)pack[0] + (((char)pack[1]) << 8));
	res = (float)(res * zetmod->koeffChannel[index] + zetmod->shiftChannel[index]);
	return res;
}
//--------------------------------------------------------------------------------------------------
float parsePack24(unsigned char* pack, int index, ZetADC* zetmod) {
	float res = static_cast<float>(((uint8_t)pack[0] << 8) + ((uint8_t)pack[1] << 16) + (((char)pack[2]) << 24));
	res = (float)(res * zetmod->koeffChannel[index] + zetmod->shiftChannel[index]);
	return res;
}
//------------------------------------------------------------------------------------------------------
int checkModule(int numModule) {
	if (numModule < 0) return -1;
	if (numberOfModules == -1) return -2;
	if (numModule >= numberOfModules)return -3;
	return numModule;
}
//----------------------------------------------------------------------------------------
int checkChannel(int numModule, int channel) {
	int err = 0;
	err = checkModule(numModule);
	if (checkModule(numModule) < 0) return err;
	if (channel < 0) return -4;
	if (channel > zm[numModule].numberOfChannels) return -5;
	return err;
}
//------------------------------------------------------------------------------------------------
int getInt(int numModule, int channel, int param)	//VID = 100, PID, BUS, PORT, NUMBEROFCHANNELS, ISCHANNEL, SIZERINGBUFFER, SIZEXML
{
	int err = 0;
	err = checkChannel(numModule, channel);
	if (err < 0) return err;
	switch (param)
	{
	case zet::VID:
		return zi[numModule].VID;

	case zet::PID:
		return zi[numModule].PID;

	case zet::BUS:
		return zi[numModule].bus;

	case zet::PORT:
		return zi[numModule].port;

	case zet::NUMBEROFCHANNELS:
		return zm[numModule].numberOfChannels;

	case zet::ISCHANNEL:
		return zm[numModule].isChannel[channel];

	case zet::SIZERINGBUFFER:
		return ringSize/zm[numModule].numberOfActiveC;

	case zet::SIZEXML:
		return zd[numModule].pointerText;
	}
	return -10;
}
//----------------------------------------------------------------------------------------------
float getFloat(int numModule, int channel, int param) {		//FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,			// Float
														//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
	int err = 0;
	err = checkChannel(numModule, channel);
	if (err < 0) return (float)err;
	switch (param)
	{
		case zet::FREQUENCY:
			return (float)zm[numModule].freqADC;

		case zet::DigitalResol_ADC:
			return (float)zm[numModule].digResADC[channel];

		case zet::DigitalResolDiff_ADC:
			return (float)zm[numModule].digResDiff[channel];

		case zet::DigitalResol_DAC:
			return (float)zm[numModule].digResDAC[channel];

		case zet::SENSE:
			return (float)zm[numModule].sense[channel];

		case zet::SHIFT:
			return (float)zm[numModule].shift[channel];

		case zet::AMPLIFY:
			return (float)zm[numModule].amplify[channel];

		case zet::REFERENCE:
			return (float)zm[numModule].reference[channel];

		case zet::FREQUENCY_DAC:
			return (float)zm[numModule].freqDAC;
	}
	return -1.;
}
//--------------------------------------------------
char* getString(int numModule, int channel, int param) //NAMEMOD = 300, TYPEMOD, SERIALMOD, IDCHANNEL, NAMECHANNEL, UNITS, COMMENT, UNITSENSE
{	
	int err = 0;
	static char stroka[100] = "error in param";
	err = checkChannel(numModule, channel);
	if (err < 0)
	{
		sprintf_s(stroka, sizeof(stroka), "error = %d", err);
		return stroka;
	}
	switch (param)
	{
		case zet::NAMEMOD:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].AdcName);
			break;

		case zet::TYPEMOD:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].AdcType);
			break;

		case zet::SERIALMOD:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].AdcSerial);
			break;

		case zet::IDCHANNEL:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].idChannel[channel]);
			break;

		case zet::NAMECHANNEL:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].nameChannel[channel]);
			break;

		case zet::UNITS:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].unitsChannel[channel]);
			break;

		case zet::COMMENT:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].commentChannel[channel]);
			break;

		case zet::UNITSENSE:
			sprintf_s(stroka, sizeof(stroka), zm[numModule].unitsenseChannel[channel]);
			break;
	}
	return stroka;
}
//------------------------------------------------------------------------------------
int getXML(int numModule, char* buffer, int size)
{
	int err = 0;
	if (numModule < 0) return -1;
	if (numberOfModules == -1) return -2;
	if (numModule >= numberOfModules) return -3;
	if (zd[numModule].pointerText == 0)
	{
		return -4;
	}
	if (size < zd[numModule].pointerText)
	{
		return-5;
	}
	for (int i = 0; i < zd[numModule].pointerText; i++)
	{
		if (i >= size)
		{
			return i;
		}
		buffer[i] = zd[numModule].fullText[i];
	}
	return zd[numModule].pointerText;
}
//------------------------------------------------------------------------------------------------
int putInt(int numModule, int channel, int param, int val)	//VID = 100, PID, BUS, PORT, ISCHANNEL,
{
	int err = 0;
	err = checkChannel(numModule, channel);
	if (err < 0) return err;
	switch (param)
	{
	case zet::VID:
		return zi[numModule].VID;

	case zet::PID:
		return zi[numModule].PID;

	case zet::BUS:
		return zi[numModule].bus;

	case zet::PORT:
		return zi[numModule].port;

	case zet::ISCHANNEL:
		zm[numModule].isChannel[channel] = val;
		return zm[numModule].isChannel[channel];
	}
	return -10;
}
//-----------------------------------------------------------------------------------------------------------
int putFloat(int numModule, int channel, int param, float val) {		//FREQUENCY = 200, DigitalResol_ADC, DigitalResolDiff_ADC, DigitalResol_DAC,			// Float
														//SENSE, SHIFT, AMPLIFY, REFERENCE, FREQUENCY_DAC
	int err = 0;
	err = checkChannel(numModule, channel);
	if (err < 0) return err;
	switch (param)
	{
	case zet::FREQUENCY:
		zm[numModule].freqADC = val;
		return 0;

	case zet::SENSE:
		zm[numModule].sense[channel] = val;
		return 0;

	case zet::SHIFT:
		zm[numModule].shift[channel] = val;
		return 0;

	case zet::AMPLIFY:
		zm[numModule].amplify[channel] = val;
		return 0;

	case zet::REFERENCE:
		zm[numModule].reference[channel] = val;
		return 0;

	case zet::FREQUENCY_DAC:
		zm[numModule].freqDAC = val;
		return 0;
	}
	return -10;
}
//--------------------------------------------------
int putString(int numModule, int channel, int param, char*val) //NAMEMOD = 300, TYPEMOD, SERIALMOD, NAMECHANNEL, UNITS, COMMENT, UNITSENSE
{
	int err = 0;
	static char stroka[100] = "error in param";

	int len = (int)strlen(val);
	if (len <= 0) return -7;
	err = checkChannel(numModule, channel);
	if (err < 0)
	{
		sprintf_s(stroka, sizeof(stroka), "error = %d", err);
		return err;
	}
	switch (param)
	{
	case zet::NAMEMOD:
		if (strlen(zm[numModule].AdcName) >= len) {
			memcpy(zm[numModule].AdcName, val, len);
		}
		break;

	case zet::TYPEMOD:
		if (strlen(zm[numModule].AdcType) >= len) {
			memcpy(zm[numModule].AdcType, val, len);
		}
		break;

	case zet::SERIALMOD:
		if (strlen(zm[numModule].AdcSerial) >= len) {
			memcpy(zm[numModule].AdcSerial, val, len);
		}
		break;

	case zet::NAMECHANNEL:
		if (strlen(zm[numModule].nameChannel[channel]) >= len) {
			memcpy(zm[numModule].nameChannel[channel], val, len);
		}
		break;

	case zet::UNITS:
		if (strlen(zm[numModule].unitsChannel[channel]) >= len) {
			memcpy(zm[numModule].unitsChannel[channel], val, len);
		}
		break;

	case zet::COMMENT:
		if (strlen(zm[numModule].commentChannel[channel]) >= len) {
			memcpy(zm[numModule].commentChannel[channel], val, len);
		}
		break;

	case zet::UNITSENSE:
		if (strlen(zm[numModule].unitsenseChannel[channel]) >= len) {
			memcpy(zm[numModule].unitsenseChannel[channel], val, len);
		}
		break;
	}
	return 0;
}
//-------------------------------------------------------------------------------------------
int putXML(int numModule)
{
	int err = 0;
	int pointer = 0;
	int len = 0;
	unsigned int TIMEOUT_MS = 2000;// Таймаут в миллисекундах
//	wchar_t debugMessage[256];

	if (numModule < 0) return -8;
	if (numModule >= sizeof(zd) / sizeof(zd[0])) return -9;
	if (zi[numModule].handle == nullptr) return -10;
	err = putParam(&zm[numModule], &zd[numModule]);
	if (err < 0)
	{
		OutputDebugStringW(L"PutParam error\n");
		return err;
	}
	static unsigned char copyReceive[100000] = { 0 };
	memcpy(copyReceive, zd[numModule].receive, zd[numModule].pointer);
			// Сохраняем в буфер
	std::ostringstream buffer;
	zd[numModule].doc.save(buffer,
		"  ",                          // отступ в 2 пробела
		pugi::format_indent,          // флаги форматирования
		pugi::encoding_utf8           // кодировка
	);
	// Получаем строку с готовым XML
	std::string xml_content = buffer.str();
	if (xml_content.size() <= 0) return -10;
	len = (int)xml_content.size();
	OutputDebugStringA(xml_content.c_str());
	memcpy(zd[numModule].copyText, xml_content.c_str(), xml_content.size()+1);
/*	for (int i = 0; i < xml_content.size(); i++) {
		if (zd[numModule].fullText[i] != zd[numModule].copyText[i]) {
			swprintf_s(debugMessage,sizeof(debugMessage)/2,L"i = %d, fullText[i] = %d    copyText[i] = %d\n",i, zd[numModule].fullText[i],zd[numModule].copyText[i]);
			OutputDebugString(debugMessage);
		}
	}*/
	// Можно вывести или отправить по сети
	std::cout << xml_content << std::endl;

	unsigned char data[28] = { 0x1c, 0x00, 0x50, 0x00, 0x46, 0x4f, 0x08, 0x00, 0x08, 0x00, 0x09, 0x00, 0x53, 0x41, 0x56, 0x45, 0x63, 0x6f, 0x6e, 0x66, 0x2e, 0x78, 0x6d, 0x6c, 0x00, 0x00, 0x00, 0x00 };

	int actual_length;
	// Отправка данных (endpoint 0x01)
	try {
		err = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_OUT,          // Endpoint OUT
			data,
			sizeof(data),
			&actual_length,
			100            // Таймаут в ms
		);
	}
	catch (const std::exception& e)
	{
		wchar_t mystr[100];

		swprintf_s(mystr, _countof(mystr), L"Unknown exeption %S\n", e.what());
		OutputDebugString(mystr);
		return -5;
	}
	if (err < 0) {
		//		TRACE(L"Write error: %s\n", libusb_error_name(err));
	}
	else {
		//		TRACE(L"Sent %d bytes\n", actual_length);
	}
	Sleep(20);

	unsigned char xmlpacket[16] = { 0x10, 0x01, 0x50, 0x00, 0x46, 0x44, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x01 };
	//									size+16	even										number							size
	UINT16* xml16;						//0-size+16, 4-full size of packets, 7- size
	unsigned char dummyPacketI[16] = { 0x10, 0x00, 0x50, 0x00, 0x46, 0x44, 0x08, 0x00, 0x17, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	//the whole size little then great
	unsigned char dummyPacketII[16] = { 0x10, 0x00, 0x50, 0x00, 0x46, 0x52, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int sizePack = 256;
	xml16 = (UINT16*)xmlpacket;
	int index = 0;
	int sz = 0;
	for (int i = 0; i < len; i = i + sizePack)
	{
		xml16[4] = i;
		sz = len - i;
		if (sz < sizePack)
		{
			xml16[7] = sz;
//			xml16[4] = len;
			if ((sz & 0x1) != 0) sz++;
			sizePack = sz;
			xml16[0] = sz + 16;
		}
		memcpy(&zd[numModule].receive[index], xmlpacket, 16);
		int ukaz = index;
/*		TRACE(L"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
			zd[numModule].receive[ukaz + 0],
			zd[numModule].receive[ukaz + 1],
			zd[numModule].receive[ukaz + 2],
			zd[numModule].receive[ukaz + 3],
			zd[numModule].receive[ukaz + 4],
			zd[numModule].receive[ukaz + 5],
			zd[numModule].receive[ukaz + 6],
			zd[numModule].receive[ukaz + 7],
			zd[numModule].receive[ukaz + 8],
			zd[numModule].receive[ukaz + 9],
			zd[numModule].receive[ukaz + 10],
			zd[numModule].receive[ukaz + 11],
			zd[numModule].receive[ukaz + 12],
			zd[numModule].receive[ukaz + 13],
			zd[numModule].receive[ukaz + 14],
			zd[numModule].receive[ukaz + 15]
		);*/
		index = index + 16;
		memcpy(&zd[numModule].receive[index], &zd[numModule].copyText[i], sizePack);
		index = index + sizePack;
	}
	xml16 = (UINT16*)dummyPacketI;
	xml16[4] = len;
	memcpy(&zd[numModule].receive[index], dummyPacketI, 16);
	int ukaz = index;
/*	TRACE(L"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
		zd[numModule].receive[ukaz + 0],
		zd[numModule].receive[ukaz + 1],
		zd[numModule].receive[ukaz + 2],
		zd[numModule].receive[ukaz + 3],
		zd[numModule].receive[ukaz + 4],
		zd[numModule].receive[ukaz + 5],
		zd[numModule].receive[ukaz + 6],
		zd[numModule].receive[ukaz + 7],
		zd[numModule].receive[ukaz + 8],
		zd[numModule].receive[ukaz + 9],
		zd[numModule].receive[ukaz + 10],
		zd[numModule].receive[ukaz + 11],
		zd[numModule].receive[ukaz + 12],
		zd[numModule].receive[ukaz + 13],
		zd[numModule].receive[ukaz + 14],
		zd[numModule].receive[ukaz + 15]
	);*/
	index = index + 16;

	memcpy(&zd[numModule].receive[index], dummyPacketII, 16);
	ukaz = index;
/*	TRACE(L"0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
		zd[numModule].receive[ukaz + 0],
		zd[numModule].receive[ukaz + 1],
		zd[numModule].receive[ukaz + 2],
		zd[numModule].receive[ukaz + 3],
		zd[numModule].receive[ukaz + 4],
		zd[numModule].receive[ukaz + 5],
		zd[numModule].receive[ukaz + 6],
		zd[numModule].receive[ukaz + 7],
		zd[numModule].receive[ukaz + 8],
		zd[numModule].receive[ukaz + 9],
		zd[numModule].receive[ukaz + 10],
		zd[numModule].receive[ukaz + 11],
		zd[numModule].receive[ukaz + 12],
		zd[numModule].receive[ukaz + 13],
		zd[numModule].receive[ukaz + 14],
		zd[numModule].receive[ukaz + 15]
	);*/
	index = index + 16;
	zd[numModule].receive[index] = 0;
	zd[numModule].pointer = index;
	actual_length = 1024;
	int realSize = 0;
	int textSize = 0;
	int result = 0;

/*	for (int i = 0; i < zd[numModule].pointer; i++)
	{
		if (zd[numModule].receive[i] != copyReceive[i])
		{
			swprintf_s(debugMessage, L"i = %d, %d != %d-copy\n", i, zd[numModule].receive[i], copyReceive[i]);
			OutputDebugString(debugMessage);
		}
	}*/


	result = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_OUT,        // Адрес endpoint для чтения
			zd[numModule].receive,             // Буфер для передачи
			zd[numModule].pointer,        // Размер буфера
			&actual_length,     // Указатель для количества записанных байт
			TIMEOUT_MS          // Таймаут
	);
				// 4. Обработайте результат
	if (result == LIBUSB_SUCCESS) {
		OutputDebugString(L"Write ok\n");
	}
	else
	{
		//			TRACE("Ошибка чтения: %s\n", libusb_error_name(result));
					// Обработка специфических ошибок
		if (result == LIBUSB_ERROR_TIMEOUT)
		{
			//				TRACE("Таймаут! Устройство не ответило вовремя\n");
		}
		else if (result == LIBUSB_ERROR_PIPE)
		{
			//				TRACE("Неправильный endpoint\n");
		}
		actual_length = -1;
	}
	try {
		actual_length = 0;
		err = libusb_bulk_transfer(
			zi[numModule].handle,
			ENDPOINT_IN,          // Endpoint OUT
			data,
			16,
			&actual_length,
			200            // Таймаут в ms
		);
//		int sjd = LIBUSB_ERROR_TIMEOUT;
		int flag = 0;
		if ((err == 0) && (actual_length != 0))
		{
			if(data[5] == 0x52)
				if (data[4] == 0x46)
				{
					wchar_t mystr[100];
					swprintf_s(mystr, _countof(mystr), L"WRITE XML error = %d\n", data[12]);
					OutputDebugString(mystr);
					flag = 1;
				}
		}
		if (flag == 0)
		{
			wchar_t mystr[100];
			swprintf_s(mystr, _countof(mystr), L"WRITE XML error = %d\n", err);
			OutputDebugString(mystr);
		}
	}
	catch (const std::exception& e)
	{
		wchar_t mystr[100];

		swprintf_s(mystr, _countof(mystr), L"Unknown exeption %S\n", e.what());
		OutputDebugString(mystr);
		return -5;
	}

	return 0;
}
//-------------------------------------------------------------------------------
long long getPointerADC(int numModule)
{
	return (zd[numModule].pointADC + (long long)zd[numModule].oborot * ringSize) / (long long)zm[numModule].numberOfActiveC;//отсчет АЦП с момента запуска
}
//------------------------------------------------------------------------------------
int getDataADC(int numModule, float* buffer, int sizeBuffer, long long pointer, int channel)
{
	int index = 0;
	int getChannel = 0;
	if (sizeBuffer <= 0) return -1;
	if (sizeBuffer >= (ringSize * 3) / 4) return -2;
	if (channel < 0) return -3;
	if (channel >= zm[numModule].numberOfChannels) return -4;
	if (zm[numModule].isChannel[channel] == 0) return -5;
	for (int i = 0; i < zm[numModule].numberOfChannels; i++)
	{
		if (i == channel)
		{
			break;
		}
		if (zm[numModule].isChannel[i] != 0)getChannel++;
	}
	long long realPoint = (zd[numModule].pointADC + (long long)zd[numModule].oborot * ringSize) / (long long)zm[numModule].numberOfActiveC;	//отсчет АЦП с момента запуска
	if (pointer > realPoint) return -6;
	long chanSize = ringSize / zm[numModule].numberOfActiveC;
	if ((pointer - sizeBuffer) < (realPoint - chanSize)) return -7;
	long getOborot = (long)((pointer - sizeBuffer) / chanSize);
	long ostatok = (long)(pointer - sizeBuffer - getOborot * chanSize);
	long first = ostatok * zm[numModule].numberOfActiveC;
	if (first < 0) first = first + ringSize;
	if (first > ringSize) first = first - ringSize;
	int endPoint = first + sizeBuffer * zm[numModule].numberOfActiveC;
	if (endPoint > ringSize)endPoint = endPoint - ringSize;
	int rasn = zd[numModule].pointADC - endPoint;
	if (rasn < 0)rasn = rasn + ringSize;

	for (int i = 0; i < sizeBuffer; i++)
	{
		index = first + i * zm[numModule].numberOfActiveC + getChannel;
		if (index >= ringSize) index = index - ringSize;
		buffer[i] = zd[numModule].dataADC[index];
	}
	return 0;
}
//-------------------------------------------------------------------------------------
int putParam(ZetADC* zetmod, ZetData* zdata)
{
	int err = 0;
	wchar_t debugMessage[256];

	// Загружаем XML в RapidXML
//	static xml_document<> doc;
	memcpy(zdata->copyText, zdata->fullText, sizeof(zdata->fullText));
	pugi::xml_parse_result result = zdata->doc.load_buffer(zdata->copyText, sizeof(zdata->copyText));// , pugi::parse_default | pugi::parse_trim_pcdata));
	if (!result) {
		std::cerr << "Ошибка парсинга XML: " << result.description() << std::endl;
		swprintf_s(debugMessage, L"Ошибка парсинга XML = %S\n", result.description());
		OutputDebugString(debugMessage);
		return -1;
	}
	pugi::xml_node config = zdata->doc.child("Config");
	if (config == 0) return -2;
	std::string version = config.attribute("version").as_string();
	std::cout << "Config version = " << version << std::endl;
	swprintf_s(debugMessage, L"Config version =  %S\n", version.c_str());
	OutputDebugString(debugMessage);
	pugi::xml_node deviceNode = config.child("Device");

	pugi::xml_attribute name_attr = deviceNode.attribute("name");
	if (name_attr) {
		name_attr.set_value(zetmod->AdcName);
	}
	pugi::xml_attribute descr = deviceNode.child("Description").attribute("label");
	if (descr) {
		descr.set_value(zetmod->description);
	}
	pugi::xml_attribute method_attr = deviceNode.child("Ethernet").attribute("method");
	if (method_attr) {
		method_attr.set_value(zetmod->ethMethod);
	}
	pugi::xml_attribute addr_attr = deviceNode.child("Ethernet").attribute("addr");
	if (addr_attr) {
		addr_attr.set_value(zetmod->ethAddr);
	}
	pugi::xml_attribute ftp_attr = deviceNode.child("Ethernet").attribute("ftp");
	if (ftp_attr) {
		ftp_attr.set_value(zetmod->ethFTP);
	}
	pugi::xml_node kod_node = deviceNode.child("KodAmplify");
	if (kod_node) {
		// Собираем строку "0,0,0,0"
		std::ostringstream oss;
		int kod;
		for (size_t i = 0; i < zetmod->numberOfChannels; ++i) {
			if (i > 0) oss << ",";
			if (zetmod->kodAmpl[i] == 1) kod = 0;
			else kod = 1;
			oss << kod;
		}
		std::string kodText = oss.str();
		kod_node.text().set(kodText.c_str());
	}
	// Задаем Sense, Shift, Reference, Amplify для каждого канала
	pugi::xml_node nodeChannels = deviceNode.child("Channels");
	for (pugi::xml_node ch : nodeChannels.children("Channel")) {
		int         id = ch.attribute("id").as_int();
		ch.attribute("name").set_value(zetmod->nameChannel[id]);
		ch.attribute("units").set_value(zetmod->unitsChannel[id]);
		ch.attribute("unitsense").set_value(zetmod->unitsenseChannel[id]);
		ch.child("Sense").text().set(zetmod->sense[id]);
		ch.child("Shift").text().set(zetmod->shift[id]);
		ch.child("Reference").text().set(zetmod->reference[id]);
		ch.child("Amplify").text().set(zetmod->amplify[id]);
		ch.child("HPF").text().set(zetmod->HPF[id]);
	}
	pugi::xml_node freq_node = deviceNode.child("Freq");
	if (freq_node) {
		freq_node.text().set(zetmod->freqADC);
	}
	pugi::xml_node freq_nodeDAC = deviceNode.child("FreqDAC");
	if (freq_nodeDAC) {
		freq_nodeDAC.text().set(zetmod->freqDAC);
	}

	// Сбор битовой маски Channel
	long long chSin = 0;
	for (int i = 0; i < zetmod->numberOfChannels; ++i) {
		if (zetmod->isChannel[i]) {
			chSin |= (1LL << i);
		}
	}
	// Преобразование в строку и запись в XML
	std::ostringstream oss;
	oss << "0x" << std::hex << chSin;
	deviceNode.child("Channel").text().set(oss.str().c_str());

	// Сбор битовой маски HCPChannel
	chSin = 0;
	for (int i = 0; i < zetmod->numberOfChannels; ++i) {
		if (zetmod->isChannelHCP[i]) {
			chSin |= (1LL << i);
		}
	}
	// Преобразование в строку и запись в XML
	oss.str("");  // Clear the content
	oss.clear();
	oss << "0x" << std::hex << chSin;
	deviceNode.child("HCPChannel").text().set(oss.str().c_str());

	// Сбор битовой маски ChannelDiff
	chSin = 0;
	for (int i = 0; i < zetmod->numberOfChannels; ++i) {
		if (zetmod->isChannelDiff[i]) {
			chSin |= (1LL << i);
		}
	}
	// Преобразование в строку и запись в XML
	oss.str("");  // Clear the content
	oss.clear();
	oss << "0x" << std::hex << chSin;
	deviceNode.child("ChannelDiff").text().set(oss.str().c_str());

	// Сбор битовой маски ChannelDAC
	chSin = 0;
	for (int i = 0; i < zetmod->numberOfChannels; ++i) {
		if (zetmod->isChannelDAC[i]) {
			chSin |= (1LL << i);
		}
	}
	// Преобразование в строку и запись в XML
	oss.str("");  // Clear the content
	oss.clear();
	oss << "0x" << std::hex << chSin;
	deviceNode.child("ChannelDAC").text().set(oss.str().c_str());

	// Преобразование в строку и запись в XML DigitalOutput
	oss.str("");  // Clear the content
	oss.clear();
	oss << "0x" << std::hex << zetmod->DigitalOutput;
	deviceNode.child("DigitalOutput").text().set(oss.str().c_str());

	// Преобразование в строку и запись в XML DigitalOutEnable
	oss.str("");  // Clear the content
	oss.clear();
	oss << "0x" << std::hex << zetmod->DigitalOutEnable;
	deviceNode.child("DigitalOutEnable").text().set(oss.str().c_str());
	
	return err;
}
//------------------------------------------------------------------------------------------------------------------------
