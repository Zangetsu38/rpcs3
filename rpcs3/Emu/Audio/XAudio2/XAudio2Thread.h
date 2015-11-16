#pragma once

#ifdef _WIN32

#include "3rdparty/minidx12/Include/sdkddkver.h" // Use last SDKDDKVer in SDK 10.586 for Win10
#include "Emu/Audio/AudioThread.h"

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) // XAudio2 2.8 and 2.9 available only on Win8+ and Win10.
#include "3rdparty/minidx12/Include/xaudio2.h"
#else 
#include "3rdparty/XAudio2_7/XAudio2.h" // XAudio 2.7 for Windows 7 Only
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
#pragma comment(lib,"../3rdparty/minidx12/Lib/xaudio2.lib") // Lib 2.9 for Only Windows 10
#elif (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#pragma comment(lib,"../3rdparty/minidx12/Lib/xaudio2_8.lib") // Lib 2.8 for Only Windows 8+
#endif

class XAudio2Thread : public AudioThread
{
	IXAudio2* m_xaudio2_instance;
	IXAudio2MasteringVoice* m_master_voice;
	IXAudio2SourceVoice* m_source_voice;

public:
	XAudio2Thread();
	virtual ~XAudio2Thread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};

#endif
