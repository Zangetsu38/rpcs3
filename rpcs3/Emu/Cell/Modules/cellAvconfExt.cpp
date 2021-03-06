﻿#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"
#include "Utilities/StrUtil.h"

#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellAvconfExt);

struct avconf_manager
{
	std::vector<CellAudioInDeviceInfo> devices;

	void copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info);
	avconf_manager();
};

avconf_manager::avconf_manager()
{
	u32 curindex = 0;

	auto mic_list = fmt::split(g_cfg.audio.microphone_devices, {"@@@"});
	if (mic_list.size())
	{
		switch (g_cfg.audio.microphone_type)
		{
		case microphone_handler::standard:
			for (u32 index = 0; index < mic_list.size(); index++)
			{
				devices.emplace_back();

				devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
				devices[curindex].availableModeCount        = 1;
				devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
				devices[curindex].deviceId                  = 0xE11CC0DE + curindex;
				devices[curindex].type                      = 0xC0DEE11C;
				devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
				devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
				devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
				devices[curindex].deviceNumber              = curindex;
				strcpy(devices[curindex].name, mic_list[index].c_str());

				curindex++;
			}
			break;
		case microphone_handler::real_singstar:
		case microphone_handler::singstar:
			// Only one device for singstar device
			devices.emplace_back();

			devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
			devices[curindex].availableModeCount        = 1;
			devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			devices[curindex].deviceId                  = 0x57A3C0DE;
			devices[curindex].type                      = 0xC0DE57A3;
			devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
			devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			devices[curindex].deviceNumber              = curindex;
			strcpy(devices[curindex].name, mic_list[0].c_str());

			curindex++;
			break;
		case microphone_handler::rocksmith:
			devices.emplace_back();

			devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
			devices[curindex].availableModeCount        = 1;
			devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			devices[curindex].deviceId                  = 0x12BA00FF; // Specific to rocksmith usb input
			devices[curindex].type                      = 0xC0DE73C4;
			devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_1;
			devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			devices[curindex].deviceNumber              = curindex;
			strcpy(devices[curindex].name, mic_list[0].c_str());

			curindex++;
			break;
		default: break;
		}
	}

	if (g_cfg.io.camera != camera_handler::null)
	{
		devices.emplace_back();

		devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
		devices[curindex].availableModeCount        = 1;
		devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
		devices[curindex].deviceId                  = 0xDEADBEEF;
		devices[curindex].type                      = 0xBEEFDEAD;
		devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
		devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_NONE;
		devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
		devices[curindex].deviceNumber              = curindex;
		strcpy(devices[curindex].name, "USB Camera");

		curindex++;
	}
}

void avconf_manager::copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info)
{
	memset(info.get_ptr(), 0, sizeof(CellAudioInDeviceInfo));

	info->portType                  = devices[num].portType;
	info->availableModeCount        = devices[num].availableModeCount;
	info->state                     = devices[num].state;
	info->deviceId                  = devices[num].deviceId;
	info->type                      = devices[num].type;
	info->availableModes[0].type    = devices[num].availableModes[0].type;
	info->availableModes[0].channel = devices[num].availableModes[0].channel;
	info->availableModes[0].fs      = devices[num].availableModes[0].fs;
	info->deviceNumber              = devices[num].deviceNumber;
	strcpy(info->name, devices[num].name);
}

s32 cellAudioOutUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioOutUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

s32 cellAudioOutGetDeviceInfo2(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetDeviceInfo2(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);
	return CELL_OK;
}

s32 cellVideoOutSetXVColor()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellVideoOutSetupDisplay()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellAudioInGetDeviceInfo(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioInDeviceInfo> info)
{
	cellAvconfExt.todo("cellAudioInGetDeviceInfo(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);

	auto av_manager = g_fxo->get<avconf_manager>();

	if (deviceNumber >= av_manager->devices.size())
		return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	av_manager->copy_device_info(deviceNumber, info);

	return CELL_OK;
}

s32 cellVideoOutConvertCursorColor(u32 videoOut, s32 displaybuffer_format, f32 gamma, s32 source_buffer_format, vm::ptr<void> src_addr, vm::ptr<u32> dest_addr, s32 num)
{
	cellAvconfExt.todo("cellVideoOutConvertCursorColor(videoOut=%d, displaybuffer_format=0x%x, gamma=0x%x, source_buffer_format=0x%x, src_addr=*0x%x, dest_addr=*0x%x, num=0x%x)", videoOut,
	    displaybuffer_format, gamma, source_buffer_format, src_addr, dest_addr, num);
	return CELL_OK;
}

s32 cellVideoOutGetGamma(u32 videoOut, vm::ptr<f32> gamma)
{
	cellAvconfExt.warning("cellVideoOutGetGamma(videoOut=%d, gamma=*0x%x)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	auto conf = g_fxo->get<rsx::avconf>();
	*gamma    = conf->gamma;

	return CELL_OK;
}

s32 cellAudioInGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioInDeviceInfo> device_info)
{
	cellAvconfExt.todo("cellAudioInGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, device_info);

	if (count > 16 || !device_info.addr())
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto av_manager = g_fxo->get<avconf_manager>();

	u32 num_devices_returned = std::min(count, (u32)av_manager->devices.size());

	for (u32 index = 0; index < num_devices_returned; index++)
	{
		av_manager->copy_device_info(index, device_info + index);
	}

	return (s32)num_devices_returned;
}

s32 cellAudioOutGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, info);
	return 0; // number of available devices
}

s32 cellVideoOutSetGamma(u32 videoOut, f32 gamma)
{
	cellAvconfExt.warning("cellVideoOutSetGamma(videoOut=%d, gamma=%f)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	if (gamma < 0.8f || gamma > 1.2f)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	auto conf   = g_fxo->get<rsx::avconf>();
	conf->gamma = gamma;

	return CELL_OK;
}

s32 cellAudioOutRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioOutRegistrationOption> option, vm::ptr<CellAudioOutDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioOutRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);
	return 0; // device number
}

s32 cellAudioOutSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioOutSetDeviceMode(deviceMode=0x%x)", deviceMode);
	return CELL_OK;
}

s32 cellAudioInSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioInSetDeviceMode(deviceMode=0x%x)", deviceMode);
	return CELL_OK;
}

s32 cellAudioInRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioInRegistrationOption> option, vm::ptr<CellAudioInDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioInRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);

	return 0; // device number
}

s32 cellAudioInUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioInUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

s32 cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<f32> screenSize)
{
	cellAvconfExt.warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	//	TODO: Use virtual screen size
#ifdef _WIN32
	//	HDC screen = GetDC(NULL);
	//	float diagonal = roundf(sqrtf((powf(float(GetDeviceCaps(screen, HORZSIZE)), 2) + powf(float(GetDeviceCaps(screen, VERTSIZE)), 2))) * 0.0393f);
#else
	// TODO: Linux implementation, without using wx
	// float diagonal = roundf(sqrtf((powf(wxGetDisplaySizeMM().GetWidth(), 2) + powf(wxGetDisplaySizeMM().GetHeight(), 2))) * 0.0393f);
#endif

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		if (g_cfg.video.monitor_3d)
		{
			*screenSize = 28.0;
			return CELL_OK;
		}
		else
		{
			return CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET;
		}
	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}
}

s32 cellVideoOutSetCopyControl(u32 videoOut, u32 control)
{
	cellAvconfExt.todo("cellVideoOutSetCopyControl(videoOut=%d, control=0x%x)", videoOut, control);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAvconfExt)
("cellSysutilAvconfExt", []() {
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetDeviceInfo2);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetXVColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetupDisplay);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutConvertCursorColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetScreenSize);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetCopyControl);
});
