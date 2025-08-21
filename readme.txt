Для работы с анализаторами ZET 030-I, ZET 030-P, сейсморегистратором ZET 048-E24, модулями АЦП ZET 211, ZET 221, подключаемых к компьютеру по USB. Исользуются программы libusb и pugixml.
В этом Solution включены три проекта:
 - проект для прямого подключения библиотек к программе написанной для Windows на C++ на MS VisualStudio 2022 в MFC в интерфейсе Dialog.
 - проект для создания библиотеки для динамической загрузки - DLL.
 - проект для подключения к динамической библиотеки, чтению и записи параметров, чтение текущих данных АЦП.
 Принцип работы с библиотекой.
	Сначала надо определить сколько модулей подключено к компьютеру.
int zet_modules()
	возвращает количество модулей, подключенных к компьютеру. Это положительное число. Если число отрицательное, то это ошибка. Коды ошибок - 
		-1 ошибка инициализации libusb
		-2 не удалось получить список устройств
		-3 не удалось отключиться от драйвера ядра
		-4 не может захватить интерфейс
	Затем надо проинициализировать один или несколько модулей. Необязательно инициализировать все модули. При инициализации вычитывается из модуля файл конфигурации. Это файл в формате XML, в котором записаны все параметры модуля.
	
int zet_init(int numModule)
	возвращает код ошибки.
		0 нормальное выполнение инициализации
		-1 ошибка инициализации в libusb_init()
		-2 модуль не найден с этим VID и PID, libusb_open_device_with_vid_pid()
		-3 не удалось отключиться от драйвера ядра
		-4 не может захватить интерфейс
		-5 размер конфигурационного файла больше буфера в библитеке
		-6 размер конфигурационного файла больше буфера в библитеке
		-7 PID модуля не найден
		-8 номер модуля numModule меньше 0
		-9 номер модуля numModule больше количества подключенных модулей
	Затем можно вычитать сам XML файл конфигурации zet_getXML() или вычитывать параметры модуля, канала в целочисленном формате zet_getInt(), в формате плавающей запятой zet_getFloat(), в текстовом виде zet_getString(). Аналогично можно записать параметры в конфигурационный файл, находящийся в буфере библиотеки zet_putInt(), zet_putFloat(), zet_putString(). Для записи файла конфигурации из библиотеки в модуль используется команда zet_putXML(). После записи в модуль файла конфигурации модуль пытается применить новые настройки и формирует получаемую конфигурацию. Поэтому необходимо заново проинициализировать модуль и прочитать полученный файл конфигурации и проверить, что все настройки применились и измененные параметры сохранились.
	Для работы с данными АЦП необходимо выполнить команду zet_startADC(). При этом в кольцевой буфер библитеки записываются данные АЦП модуля. Размер кольцевого буфера для одного канала АЦП можно вычитать функцией zet_getInt(SIZERINGBUFFER). Таким образом для одной операции чтения данных zet_getDataADC() доступны данные не более размера кольцевого буфера. Затем по таймеру или в отдельном потоке необходимо вычитывать текущий указатель кольцевого буфера zet_getPointerADC() и при накоплении в кольцевом буфере необходимого обьема данных вычитывать данные их кольцевого буфера zet_getDataADC(). Для удобства указатель буфера идет по нарастающей и не возвращается в 0. После вычитывания данные из буфера не исчезают и не пропадают. Таким образом можно вычитывать массивы данных разных размеров для различных алгоритмов. Указатель кольцевого буфера указывает на последний полученный отсчет АЦП pointer. Функция zet_getDataADC() берет sizeBuffer данных начиная с pointer-sizeBuffer отсчета АЦП до pointer отсчета АЦП.
	Данные в буфере формируются следующим образом: целочисленные данные от АЦП умножаются на вес младшего разряда DigitalResol_ADC, чувствительность по каналу SENSE, коэффициент усиления AMPLIFY и добавляется смещение SHIFT.
	При выходе из программы необходимо выполнить команду zet_stopADC().
	
int zet_startADC(int numModule)
	Запускает передачу данных АЦП во внутренний буфер библиотеки.
	возвращает код ошибки:
		0 нормальное выполнение команды
		-1 поток и передача данных уже запущена
		-2 ошибка в передаче данных libusb_bulk_transfer
		-3 ошибка в передаче данных libusb_bulk_transfer
		-8 numModule меньше 0
		-9 numModule больше количества подключенных модулей
		
long long zet_getPointerADC(int numModule)
	читает текущий указатель во внутреннем буфере библиотеки
	возвращает указатель, который больше 0
	
int zet_getDataADC(int numModule, float* buffer, int sizeBuffer, long long pointer, int channel)
	копирует данные по одному каналу АЦП из внутреннего буфера библиотеки в пользовательский
	numModule - номер модуля от 0 до N-1
	buffer - указатель на пользовательский буфер плавающей запятой
	sizeBuffer - количество данных для передачи
	pointer - указатель до которого надо прочитать данные в буфер
	channel - номер канала от 0 до NUMBEROFCHANNELS-1
	Данные передаются из внутреннего буфера в пользовательский от pointer - sizeBuffer до pointer-1 включительно.
	Возвращает код ошибки
		0 нормальное выполнение команды
		-1 количество данных для передачи sizeBuffer
		-2 количество данных для передачи больше 3/4 размера кольцевого буфера
		-3 номер канала channel меньше 0
		-4 номер канала channel больше количества каналов NUMBEROFCHANNELS в модуле
		-5 канал не включен. Параметр ISCHANNEL равен 0
		-6 указатель pointer больше внутреннего указателя кольцевого буфера
		-7 указатель pointer меньше внутреннего указателя кольцевого буфера-размер кольцевого буфера
		
int zet_getInt(int numModule, int channel, int param)
	вычитывает параметр param в формате целого числа из конфигурационного файла модуля, расположенного в памяти библиотеки
	param может принимать значения:
	VID	- вычитывает VID модуля
	PID - вычитывает PID модуля
	BUS - вычитывает шину USB в компьютере
	PORT - вычитывает PORT USB в компьютере
	NUMBEROFCHANNELS - количество всех каналов АЦП в модуле
	ISCHANNEL - 0 - канал выключен, 1 - включен
	SIZERINGBUFFER - размер кольцевого буфера
	SIZEXML - размер конфигурационного файла модуля в байтах
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр
		
float zet_getFloat(int numModule, int channel, int param)
	вычитывает параметр param в формате плавающей запятой из конфигурационного файла модуля, расположенного в памяти библиотеки
	param может принимать значения:
	FREQUENCY - частота дискретизации АЦП - задается в Гц
	DigitalResol_ADC - вес младшего разряда АЦП в мВ
	DigitalResolDiff_ADC - вес младшего разряда АЦП дифференциального подключения
	DigitalResol_DAC - вес младшего разряда ЦАП
	SENSE - чувствительность канала измерений
	SHIFT - сдвиг в единицах измерений канала
	AMPLIFY - коэффициент усиления внешнего усилителя
	REFERENCE - опорное значение для расчета уровней сигнала в дециБелах
	FREQUENCY_DAC - частота дискретизации ЦАП - задается в Гц
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр
		
float zet_getString(int numModule, int channel, int param)
	вычитывает параметр param в текстовом формате из конфигурационного файла модуля, расположенного в памяти библиотеки
	param может принимать значения:
	NAMEMOD - название модуля
	TYPEMOD - тип модуля
	SERIALMOD - заводской номер модуля
	IDCHANNEL - идентификатор канала
	NAMECHANNEL - название канала
	UNITS - единица измерения канала (Н, кг, g, м/с, ...)
	COMMENT - коментарий
	UNITSENSE - величина к которой приведена чувствительность датчика - мВ или В
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр
		
int zet_getXML(int numModule, char* buffer, int size)
	копирует содержимое XML файла конфигурации указанного модуля в пользовательский массив buffer размера size.
	Возвращает реальный размер xml файла или код ошибки
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 файл конфигурации в памяти библиотеки отсутствует
		-5 размер буфера size меньше размера файла
		
int zet_putInt(int numModule, int channel, int param, int val)
	Задает целочисленный параметр param в указанном модуле и канале в конфигурационный файл, расположенный в памяти библиотеки
	param может принимать значения:
	ISCHANNEL 0 или 1 - включить или выключить канал
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр

int zet_putFloat(int numModule, int channel, int param, float val)
	Задает параметр param в формате плавающей запятой в указанном модуле и канале в конфигурационный файл, расположенный в памяти библиотеки
	param может принимать значения:
	FREQUENCY частота дискретизации АЦП - задается в Гц
	SENSE - чувствительность канала измерений
	SHIFT - сдвиг в единицах измерений канала
	AMPLIFY - коэффициент усиления внешнего усилителя
	REFERENCE - опорное значение для расчета уровней сигнала в дециБелах
	FREQUENCY_DAC - частота дискретизации ЦАП - задается в Гц	
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр

int zet_putString(int numModule, int channel, int param, char* val)
	Задает параметр param в текстовом формате в конфигурационный файл, расположенный в памяти библиотеки
	param может принимать значения:
	NAMEMOD - название модуля
	TYPEMOD - тип модуля
	SERIALMOD - заводской номер модуля
	IDCHANNEL - идентификатор канала
	NAMECHANNEL - название канала
	UNITS - единица измерения канала (Н, кг, g, м/с, ...)
	COMMENT - коментарий
	UNITSENSE - величина к которой приведена чувствительность датчика - мВ или В
	Возвращает полжительное число или 0 при нормальном выполнении или возвращает код ошибки, который меньше 0.
		-1 номер модуля numModule меньше 0 
		-2 нет подключенных модулей
		-3 номер модуля numModule больше количества поключенных модулей
		-4 номер канала channel меньше 0
		-5 номер канала channel больше количества калалов в модуле
		-10 неправильный параметр

int zet_putXML(int numModule)
	Записывает файла конфигурации из библиотеки в модуль
	Возвращает код ошибки или 0 если запись произошла успешно.
		-1 ошибка парсинга XML файла
		-2 неправильный формат XML файла
		-5 ошибка передачи файла
		-8 numModule меньше 0
		-9 numModule больше количества подключенных модулей
		-10 размер файла равен 0
	
int zet_stopADC(int numModule)
	Завершение работы с модулем, освобождение всех потоков, закрытие всех связей с модулем.
	Возвращает код ошибки
		0 нормальное выполнение команды
		-1 АЦП не запущено, команда zet_startADC() не выполнялась
		-2 ошибка в libusb_bulk_transfer
		-3 ошибка в libusb_bulk_transfer
		-4 номер модуля numModule меньше 0 
		-5 номер модуля numModule больше количества поключенных модулей
		
		Пример XML файла модуля ZET 030-I
		
<?xml version="1.0"?>
<Config version="1.2">
  <Device name="ZET 030-I" type="30" serial="23044">
    <Description label="" />
    <DigitalResolChanADC>4.66147e-09,4.66066e-09,4.65842e-09,4.66149e-09</DigitalResolChanADC>
    <Ethernet method="static" addr="192.168.0.100/24" ftp="no" />
    <Freq>25000</Freq>
    <Channel>0xf</Channel>
    <HCPChannel>0x0</HCPChannel>
    <KodAmplify>0,0,0,0</KodAmplify>
    <Recorder start="auto" />
    <RecordMinutes>0</RecordMinutes>
    <Channels>
      <Channel id="0" name="ZET 030-" units="мВ" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <CoordX>0</CoordX>
        <CoordY>0</CoordY>
        <CoordZ>0</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="1" name="ZET 030-" units="мВ" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <CoordX>0</CoordX>
        <CoordY>0</CoordY>
        <CoordZ>0</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="2" name="ZET 030-" units="мВ" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <CoordX>0</CoordX>
        <CoordY>0</CoordY>
        <CoordZ>0</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="3" name="ZET 030-" units="мВ" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <CoordX>0</CoordX>
        <CoordY>0</CoordY>
        <CoordZ>0</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
    </Channels>
  </Device>
</Config>

			Пример XML файла для модуля ZET 211
			
<?xml version="1.0"?>
<Config version="1.2">
  <Device name="ZET 211" type="211" serial="24109">
    <Description label="" />
    <DigitalResolChanADC>0.000355354,0.000355175,0.000354743,0.000355044,0.000354136,0.00035395,0.000356666,0.000356037,0.000355129,0.000353859,0.000354585,0.000354273,0.000357353,0.000354654,0.000355127,0.000356171</DigitalResolChanADC>
    <DigitalResolDiffADC>0.00035629,0.000356233,0.000355829,0.000355914,0.000355007,0.000355059,0.00035738,0.000357409,0.000355617,0.000355692,0.000355521,0.000355618,0.000357046,0.000357154,0.000356836,0.000356862</DigitalResolDiffADC>
    <DigitalResolChanDAC>0.000152084,0.000151978</DigitalResolChanDAC>
    <DigitalOutput>0x00</DigitalOutput>
    <DigitalOutEnable>0x00</DigitalOutEnable>
    <ConfigTime ts="1742887481">25.06.2025 15:21:16</ConfigTime>
    <Freq>5000</Freq>
    <Channels>
      <Channel id="0" name="ZET 211_" units="мВ" comment="⁣" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="1" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="2" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="3" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="4" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="5" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="6" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="7" name="ZET 211_" units="мВ" comment="" enabled="true" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="8" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="9" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="10" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="11" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="12" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="13" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="14" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
      <Channel id="15" name="ZET 211_" units="мВ" comment="" enabled="false" unitsense="В">
        <Sense>0.001</Sense>
        <Shift>0</Shift>
        <Reference>0.001</Reference>
        <Amplify>1</Amplify>
        <HPF>0</HPF>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
      </Channel>
    </Channels>
    <ChannelDiff>0x0000</ChannelDiff>
    <Channel>0x00ff</Channel>
    <FreqDAC>10000</FreqDAC>
    <ChannelDAC>0x0003</ChannelDAC>
    <DigitalPort>
      <out>255</out>
      <rec>255</rec>
    </DigitalPort>
  </Device>
</Config>

		Пример XML файла для модуля ZET 221
		
<?xml version="1.0"?>
<Config version="1.2">
  <Device name="ZET 221" type="221" serial="221002">
    <Description label="" />
    <DigitalResolChanADC>6.26829e-09,6.2569e-09,6.26343e-09,6.26314e-09,6.25666e-09,6.25431e-09,6.27637e-09,6.27678e-09,6.24161e-09,6.27185e-09,6.26587e-09,6.2755e-09,6.27012e-09,6.27526e-09,6.25715e-09,6.25294e-09</DigitalResolChanADC>
    <DigitalResolDiffADC>6.27835e-09,6.27835e-09,6.28327e-09,6.28327e-09,6.27402e-09,6.27402e-09,6.29395e-09,6.29395e-09,6.27294e-09,6.27294e-09,6.28667e-09,6.28667e-09,6.28832e-09,6.28832e-09,6.26202e-09,6.26202e-09</DigitalResolDiffADC>
    <DigitalResolChanDAC>0.000152969,0.00015244</DigitalResolChanDAC>
    <Freq>8000</Freq>
    <Channel>0x0001</Channel>
    <ChannelDiff>0x0000</ChannelDiff>
    <FreqDAC>20000</FreqDAC>
    <ChannelDAC>0x0003</ChannelDAC>
    <Channels>
      <Channel id="0" name="ZET 221_221002_1" units="мВ" unitsense="В" comment="" enabled="true">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="1" name="ZET 221_221002_2" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="2" name="ZET 221_221002_3" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="3" name="ZET 221_221002_4" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="4" name="ZET 221_221002_5" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="5" name="ZET 221_221002_6" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="6" name="ZET 221_221002_7" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="7" name="ZET 221_221002_8" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="8" name="ZET 221_221002_9" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="9" name="ZET 221_221002_10" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="10" name="ZET 221_221002_11" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="11" name="ZET 221_221002_12" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="12" name="ZET 221_221002_13" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="13" name="ZET 221_221002_14" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="14" name="ZET 221_221002_15" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
      <Channel id="15" name="ZET 221_221002_16" units="мВ" unitsense="В" comment="" enabled="false">
        <Sense>0.001000</Sense>
        <Shift>0.000000</Shift>
        <Reference>0.001000</Reference>
        <Amplify>1.000000</Amplify>
        <CoordX>0.000000</CoordX>
        <CoordY>0.000000</CoordY>
        <CoordZ>0.000000</CoordZ>
        <CoordP>0</CoordP>
        <HPF>0</HPF>
      </Channel>
    </Channels>
    <DigitalOutput>0x00</DigitalOutput>
    <DigitalOutEnable>0x00</DigitalOutEnable>
    <ConfigTime>28.05.2025 13:00:50</ConfigTime>
  </Device>
</Config>

