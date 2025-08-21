# Zet USB ADC Library

Библиотека для работы с анализаторами ZET 030-I, ZET 030-P, сейсморегистратором ZET 048-E24 и АЦП-модулями ZET 211, ZET 221 по USB. Использует libusb и pugixml.

---

## Поддерживаемое оборудование

- Анализаторы ZET 030-I и ZET 030-P
- Сейсморегистратор ZET 048-E24
- АЦП-модули ZET 211 и ZET 221

---

## Зависимости

- [libusb](https://libusb.info) (для USB-обмена)
- [pugixml](http://https://pugixml.org/) (для работы с XML-конфигурациями)

---

## Структура решения

В составе Solution три проекта:

1. Приложение на C++ с диалоговым интерфейсом на MFC (MS Visual Studio 2022) для прямого подключения библиотеки.
2. Сборка динамической библиотеки (DLL).
3. Тестовое приложение для загрузки DLL, чтения/записи параметров и получения данных АЦП.

---

## Основные этапы работы

1. Определить число подключенных модулей:
   ```cpp
   int count = zet_modules();
   ```
2. Проинициализировать нужные модули (можно не все):
   ```cpp
   int res = zet_init(moduleIndex);
   ```
3. Читать или записывать настройки через XML или отдельные параметры.
4. Запустить сбор данных АЦП:
   ```cpp
   zet_startADC(moduleIndex);
   ```
5. По таймеру/потоку получать указатель кольцевого буфера и вычитывать данные:
   ```cpp
   long long ptr = zet_getPointerADC(moduleIndex);
   zet_getDataADC(moduleIndex, buffer, size, ptr, channel);
   ```
6. По завершении остановить сбор АЦП:
   ```cpp
   zet_stopADC(moduleIndex);
   ```

---

## API-справочник

### Управление модулями

| Функция               | Прототип                             | Описание                                         |
|-----------------------|--------------------------------------|--------------------------------------------------|
| zet_modules           | `int zet_modules();`                 | Возвращает число подключенных модулей или код ошибки. |
| zet_init              | `int zet_init(int moduleIndex);`     | Инициализация модуля и загрузка его XML-конфигурации. |

#### zet_modules

```cpp
int zet_modules();
```

- Возвращает количество модулей.
- Ошибки (отрицательное значение):

  - `-1` — ошибка инициализации libusb
  - `-2` — не удалось получить список устройств
  - `-3` — не удалось отсоединить драйвер ядра
  - `-4` — не удалось захватить интерфейс

#### zet_init

```cpp
int zet_init(int moduleIndex);
```

- `moduleIndex` — от 0 до (count – 1).
- Загружает XML-конфиг из модуля.
- Ошибки:

  - `0`  — успех
  - `-1` — ошибка libusb_init()
  - `-2` — модуль с VID/PID не найден
  - `-3` — не удалось отсоединить драйвер ядра
  - `-4` — не удалось захватить интерфейс
  - `-5` — размер XML больше буфера библиотеки
  - `-6` — то же, дублируется
  - `-7` — PID модуля не найден
  - `-8` — `moduleIndex < 0`
  - `-9` — `moduleIndex >=` числа модулей

---

### Конфигурация и параметры

| Функция              | Прототип                                                             | Описание                                       |
|----------------------|----------------------------------------------------------------------|------------------------------------------------|
| zet_getXML           | `int zet_getXML(int, char* buffer, int size);`                       | Копирует XML-конфиг в `buffer`.                |
| zet_putXML           | `int zet_putXML(int, const char* buffer, int size);`                 | Записывает `buffer` как XML-конфиг в модуль.   |
| zet_getInt           | `int zet_getInt(int, int channel, int param);`                       | Читает целочисленный параметр из XML-конфига.  |
| zet_putInt           | `int zet_putInt(int, int channel, int param, int value);`            | Записывает целочисленный `value` в XML-конфиг. |
| zet_getFloat         | `float zet_getFloat(int, int channel, int param);`                   | Читает параметр с плавающей точкой.            |
| zet_putFloat         | `int zet_putFloat(int, int channel, int param, float value);`        | Записывает `value` с плавающей точкой.         |
| zet_getString        | `const char* zet_getString(int, int channel, int param);`            | Читает текстовый параметр.                     |
| zet_putString        | `int zet_putString(int, int channel, int param, const char* value);` | Записывает текстовый `value`.                  |

#### Общая схема работы с XML

1. Вызвать `zet_getXML()` и распарсить буфер.
2. Изменить нужные узлы через `zet_putInt/Float/String()`.
3. Отправить в модуль `zet_putXML()`.
4. Повторно вызвать `zet_init()` и `zet_getXML()`, чтобы проверить применённые настройки.

---

### Приём данных АЦП

| Функция                 | Прототип                                                                    | Описание                                              |
|-------------------------|-----------------------------------------------------------------------------|-------------------------------------------------------|
| zet_startADC            | `int zet_startADC(int moduleIndex);`                                        | Запускает сбор в кольцевой буфер библиотеки.          |
| zet_stopADC             | `int zet_stopADC(int moduleIndex);`                                         | Останавливает сбор АЦП.                               |
| zet_getPointerADC       | `long long zet_getPointerADC(int moduleIndex);`                             | Текущий указатель в буфере.                           |
| zet_getDataADC          | `int zet_getDataADC(int mod, float* buf, int size, long long ptr, int ch);` | Копирует `size` отсчетов из буфера по каналу `ch`.    |

#### zet_startADC / zet_stopADC

```cpp
int zet_startADC(int moduleIndex);
int zet_stopADC(int moduleIndex);
```

- Ошибки `zet_startADC`:

  - `0`  — успех
  - `-1` — передача уже запущена
  - `-2`, `-3` — ошибки libusb_bulk_transfer
  - `-8` — `moduleIndex < 0`
  - `-9` — `moduleIndex >=` числа модулей

#### zet_getPointerADC

```cpp
long long zet_getPointerADC(int moduleIndex);
```

- Возвращает позицию последнего отсчёта.

#### zet_getDataADC

```cpp
int zet_getDataADC(
  int moduleIndex,
  float* buffer,
  int sizeBuffer,
  long long pointer,
  int channel
);
```

- Копирует данные от `(pointer – sizeBuffer)` до `(pointer – 1)` включительно.
- Ошибки:

  - `0`  — успех
  - `-1` — `sizeBuffer < 0`
  - `-2` — `sizeBuffer` > ¾ размера кольцевого буфера
  - `-3` — `channel < 0`
  - `-4` — `channel >= NUMBEROFCHANNELS`
  - `-5` — канал выключен (`ISCHANNEL == 0`)
  - `-6` — `pointer >` внутреннего указателя
  - `-7` — `pointer <` внутреннего указателя – буфер

---

## Пример использования

```cpp
#include "ZetLib.h"

int main() {
    int count = zet_modules();
    if (count <= 0) return -1;

    // Инициализация первого модуля
    if (zet_init(0) != 0) return -2;

    // Запуск АЦП
    if (zet_startADC(0) != 0) return -3;

    // Ожидание накопления данных…
    Sleep(100);

    long long ptr = zet_getPointerADC(0);
    const int N = 512;
    float data[N];
    if (zet_getDataADC(0, data, N, ptr, 0) != 0) return -4;

    zet_stopADC(0);
    return 0;
}
```
