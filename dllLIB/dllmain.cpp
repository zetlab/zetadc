// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h> // Необходимо для OutputDebugString
#include <objidl.h>
#include <oleidl.h>
#include <ocidl.h>
#include <atlbase.h>
#include <atlcom.h>
#include "../ZETADC.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
//-------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_modules()
{
    int err = 0;
    err = modules();
    return err;
}
//-------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_init(int numModule)
{
    int err = 0;
    err = init(numModule);
    return err;
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_startADC(int numModule)
{
    return startADC(numModule);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_stopADC(int numModule)
{
    return stopADC(numModule);
}
//----------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_getInt(int numModule, int channel, int param)
{
    return getInt(numModule, channel, param);
}
//----------------------------------------------------------------
extern "C" __declspec(dllexport) float zet_getFloat(int numModule, int channel, int param)
{
    return getFloat(numModule, channel, param);
}
//----------------------------------------------------------------
extern "C" __declspec(dllexport) char* zet_getString(int numModule, int channel, int param)
{
    return getString(numModule, channel, param);
}
//----------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_getXML(int numModule, char* buffer, int sizeBuffer)
{
    return getXML(numModule, buffer, sizeBuffer);
}
//-----------------------------------------------------------------
extern "C" __declspec(dllexport) long long zet_getPointerADC(int numModule)
{
    return getPointerADC(numModule);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_getDataADC(int numModule, float* buffer, int sizeBuffer, long long pointer, int channel)
{
    return  getDataADC(numModule, buffer, sizeBuffer, pointer, channel);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_putInt(int numModule, int channel, int param, int value)
{
    return putInt(numModule, channel, param, value);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_putFloat(int numModule, int channel, int param, float value)
{
    return putFloat(numModule, channel, param, value);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_putString(int numModule, int channel, int param, char* value)
{
    return putString(numModule, channel, param, value);
}
//-------------------------------------------------------------------------
extern "C" __declspec(dllexport) int zet_putXML(int numModule)
{
    return putXML(numModule);
}
//-------------------------------------------------------------------------
