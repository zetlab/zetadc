# Zet USB ADC Library

Library for working with ZETLAB analyzers ZET 030-I, ZET 030-P, seismograph recorder ZET 048-E24, and ADC modules ZET 211 and ZET 221 over USB. Uses libusb for USB communication and pugixml for XML configuration parsing.

---

## Supported Hardware

- Analyzers ZET 030-I and ZET 030-P
- Seismograph recorder ZET 048-E24
- ADC modules ZET 211 and ZET 221

---

## Dependencies

- [libusb](https://libusb.info) (for USB data exchange)
- [pugixml](http://https://pugixml.org/) (for reading and writing XML configuration)

---

## Solution Structure

The Visual Studio solution contains three projects:

1. A C++ MFC dialog–based application (MS Visual Studio 2022) that links the library directly.
2. A project that builds the library as a DLL.
3. A test application demonstrating dynamic DLL loading, parameter I/O, and ADC data retrieval.

---

## Basic Workflow

1. Determine how many modules are connected:
   ```cpp
   int count = zet_modules();
   ```
2. Initialize one or more modules:
   ```cpp
   int status = zet_init(moduleIndex);
   ```
3. Read or write configuration via XML or individual parameters.
4. Start ADC data acquisition:
   ```cpp
   zet_startADC(moduleIndex);
   ```
5. Periodically or in a separate thread, get the ring buffer pointer and read data:
   ```cpp
   long long ptr = zet_getPointerADC(moduleIndex);
   zet_getDataADC(moduleIndex, buffer, size, ptr, channel);
   ```
6. When finished, stop ADC acquisition:
   ```cpp
   zet_stopADC(moduleIndex);
   ```

---

## API Reference

### Module Management

| Function         | Prototype                          | Description                                             |
|------------------|------------------------------------|---------------------------------------------------------|
| zet_modules      | `int zet_modules();`               | Returns number of connected modules or negative error.  |
| zet_init         | `int zet_init(int moduleIndex);`   | Initializes a module and loads its XML configuration.   |

#### zet_modules

```cpp
int zet_modules();
```

Returns the count of connected modules. Negative values indicate errors:

- `-1` — libusb initialization failed  
- `-2` — failed to list devices  
- `-3` — failed to detach kernel driver  
- `-4` — failed to claim interface  

#### zet_init

```cpp
int zet_init(int moduleIndex);
```

`moduleIndex` from 0 to (count – 1). Loads the XML config from the module. Error codes:

- `0`  — success  
- `-1` — libusb_init() failed  
- `-2` — device with VID/PID not found  
- `-3` — failed to detach kernel driver  
- `-4` — failed to claim interface  
- `-5`, `-6` — XML size exceeds library buffer  
- `-7` — module PID not recognized  
- `-8` — `moduleIndex < 0`  
- `-9` — `moduleIndex >=` number of modules  

---

### Configuration and Parameters

| Function         | Prototype                                                           | Description                                                  |
|------------------|---------------------------------------------------------------------|--------------------------------------------------------------|
| zet_getXML       | `int zet_getXML(int, char* buffer, int size);`                      | Copies module XML config into `buffer`.                      |
| zet_putXML       | `int zet_putXML(int, const char* buffer, int size);`                | Writes `buffer` as XML config to the module.                 |
| zet_getInt       | `int zet_getInt(int, int channel, int param);`                      | Reads an integer parameter from the in-memory XML config.    |
| zet_putInt       | `int zet_putInt(int, int channel, int param, int value);`           | Writes an integer `value` to the in-memory XML config.       |
| zet_getFloat     | `float zet_getFloat(int, int channel, int param);`                  | Reads a floating-point parameter.                            |
| zet_putFloat     | `int zet_putFloat(int, int channel, int param, float value);`       | Writes a floating-point `value`.                             |
| zet_getString    | `const char* zet_getString(int, int channel, int param);`           | Reads a text parameter.                                      |
| zet_putString    | `int zet_putString(int, int channel, int param, const char* value);`| Writes a text `value`.                                       |

Workflow:

1. Call `zet_getXML()` and parse the buffer.
2. Modify parameters using `zet_putInt/Float/String()`.
3. Send updates with `zet_putXML()`.
4. Reinitialize module and verify changes via `zet_getXML()`.

---

### ADC Data Acquisition

| Function              | Prototype                                                                     | Description                                                   |
|-----------------------|-------------------------------------------------------------------------------|---------------------------------------------------------------|
| zet_startADC          | `int zet_startADC(int moduleIndex);`                                          | Begins ADC data transfer into the library’s ring buffer.      |
| zet_stopADC           | `int zet_stopADC(int moduleIndex);`                                           | Stops ADC data acquisition.                                   |
| zet_getPointerADC     | `long long zet_getPointerADC(int moduleIndex);`                               | Returns current write pointer in the ring buffer.             |
| zet_getDataADC        | `int zet_getDataADC(int mod, float* buf, int size, long long ptr, int ch);`   | Copies `size` samples from `(ptr–size)` … `(ptr–1)` for `ch`. |

#### Error codes for zet_startADC

- `0`  — success
- `-1` — already running
- `-2`, `-3` — libusb_bulk_transfer error
- `-8` — `moduleIndex < 0`
- `-9` — `moduleIndex >=` number of modules

#### zet_getPointerADC

```cpp
long long zet_getPointerADC(int moduleIndex);
```

Returns the index of the last sample written.

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

Copies data from `(pointer – sizeBuffer)` to `(pointer – 1)` inclusive. Errors:

- `0`  — success
- `-1` — `sizeBuffer < 0`
- `-2` — `sizeBuffer` > ¾ ring buffer size
- `-3` — `channel < 0`
- `-4` — `channel >= NUMBEROFCHANNELS`
- `-5` — channel disabled (`ISCHANNEL == 0`)
- `-6` — `pointer` beyond current write pointer
- `-7` — `pointer` too far behind

---

## Example Usage

```cpp
#include "ZetLib.h"

int main() {
    int count = zet_modules();
    if (count <= 0) return -1;

    if (zet_init(0) != 0) return -2;

    if (zet_startADC(0) != 0) return -3;

    Sleep(100);

    long long ptr = zet_getPointerADC(0);
    const int N = 512;
    float data[N];
    if (zet_getDataADC(0, data, N, ptr, 0) != 0) return -4;

    zet_stopADC(0);
    return 0;
}
```
