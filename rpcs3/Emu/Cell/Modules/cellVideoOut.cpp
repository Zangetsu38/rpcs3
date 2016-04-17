#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"

#include "cellVideoOut.h"

extern logs::channel cellSysutil;

const extern std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map
{
	{ video_resolution::_1080,      { 1920, 1080 } },
	{ video_resolution::_720,       { 1280, 720 } },
	{ video_resolution::_480,       { 720, 480 } },
	{ video_resolution::_576,       { 720, 576 } },
	{ video_resolution::_1600x1080, { 1600, 1080 } },
	{ video_resolution::_1440x1080, { 1440, 1080 } },
	{ video_resolution::_1280x1080, { 1280, 1080 } },
	{ video_resolution::_960x1080,  { 960, 1080 } },
	// 3D Stereo
	{ video_resolution::_720_3d,    { 1280, 1470 } },
	{ video_resolution::_1024_3d,   { 1024, 1470 } },
	{ video_resolution::_960_3d,    { 960, 1470 } },
	{ video_resolution::_800_3d,    { 800, 1470 } },
	{ video_resolution::_640_3d,    { 640, 1470 } },
};

const extern std::unordered_map<video_resolution, CellVideoOutResolutionId, value_hash<video_resolution>> g_video_out_resolution_id
{
	{ video_resolution::_1080,      CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ video_resolution::_720,       CELL_VIDEO_OUT_RESOLUTION_720 },
	{ video_resolution::_480,       CELL_VIDEO_OUT_RESOLUTION_480 },
	{ video_resolution::_576,       CELL_VIDEO_OUT_RESOLUTION_576 },
	{ video_resolution::_1600x1080, CELL_VIDEO_OUT_RESOLUTION_1600x1080 },
	{ video_resolution::_1440x1080, CELL_VIDEO_OUT_RESOLUTION_1440x1080 },
	{ video_resolution::_1280x1080, CELL_VIDEO_OUT_RESOLUTION_1280x1080 },
	{ video_resolution::_960x1080,  CELL_VIDEO_OUT_RESOLUTION_960x1080 },
	// 3D Stereo
	{ video_resolution::_720_3d,    CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING },
	{ video_resolution::_1024_3d,   CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING },
	{ video_resolution::_960_3d,    CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING },
	{ video_resolution::_800_3d,    CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING },
	{ video_resolution::_640_3d,    CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING },
};

const extern std::unordered_map<video_aspect, CellVideoOutDisplayAspect, value_hash<video_aspect>> g_video_out_aspect_id
{
	{ video_aspect::_auto, CELL_VIDEO_OUT_ASPECT_AUTO },
	{ video_aspect::_16_9, CELL_VIDEO_OUT_ASPECT_16_9 },
	{ video_aspect::_4_3,  CELL_VIDEO_OUT_ASPECT_4_3 },
};

const extern std::unordered_map<video_refresh, CellVideoOutRefreshRate, value_hash<video_refresh>> g_video_out_refresh_id
{
	{ video_refresh::_auto,  CELL_VIDEO_OUT_REFRESH_RATE_AUTO },
	{ video_refresh::_59_94, CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ },
	{ video_refresh::_50,    CELL_VIDEO_OUT_REFRESH_RATE_50HZ },
	{ video_refresh::_60,    CELL_VIDEO_OUT_REFRESH_RATE_60HZ },
	{ video_refresh::_30,    CELL_VIDEO_OUT_REFRESH_RATE_30HZ },
};

template<>
void fmt_class_string<CellVideoOutError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_VIDEO_OUT_ERROR_NOT_IMPLEMENTED);
		STR_CASE(CELL_VIDEO_OUT_ERROR_ILLEGAL_CONFIGURATION);
		STR_CASE(CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER);
		STR_CASE(CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE);
		STR_CASE(CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND);
		STR_CASE(CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT);
		STR_CASE(CELL_VIDEO_OUT_ERROR_UNSUPPORTED_DISPLAY_MODE);
		STR_CASE(CELL_VIDEO_OUT_ERROR_CONDITION_BUSY);
		STR_CASE(CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET);
		}

		return unknown;
	});
}

error_code cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.trace("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	auto &displayMode = state->displayMode;
	auto &resolutionId = displayMode.resolutionId;
	auto &aspectId = displayMode.aspect;
	auto &conversionId = displayMode.conversion;
	auto &refreshRatesId = displayMode.refreshRates;

	cellSysutil.fatal("cellVideoOutGetState(resolutionId=0x%x, aspect=%d, refreshRates=%d, colorSpace=0x%x, conversion=0x%x)",
		resolutionId, state->displayMode.aspect, state->displayMode.refreshRates, state->colorSpace, state->displayMode.conversion);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
		state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;

		state->displayMode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;

		switch (resolutionId)
		{
		//case 0x0: cellSysutil.fatal("Get State: Default resolution 0"); break;
			//resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution);
		case 0x01: cellSysutil.fatal("Get State: Resolution 1080p"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_1080; break;
		case 0x02: cellSysutil.fatal("Get State: Resolution 720p"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_720; break;
		case 0x04: cellSysutil.fatal("Get State: Resolution 480p"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_480; break;
		case 0x05: cellSysutil.fatal("Get State: Resolution 576p"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_576; break;
		case 0x0a: cellSysutil.fatal("Get State: Resolution 1600"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_1600x1080; break;
		case 0x0b: cellSysutil.fatal("Get State: Resolution 1440"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_1440x1080; break;
		case 0x0c: cellSysutil.fatal("Get State: Resolution 1280"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_1280x1080; break;
		case 0x0d: cellSysutil.fatal("Get State: Resolution 960"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_960x1080; break;
		case 0x81: cellSysutil.fatal("Get State: Resolution 720p 3D"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING; break;
		case 0x88: cellSysutil.fatal("Get State: Resolution 1024 3D"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING; break;
		case 0x89: cellSysutil.fatal("Get State: Resolution 960 3D"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING; break;
		case 0x8a: cellSysutil.fatal("Get State: Resolution 800 3D"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING; break;
		case 0x8b: cellSysutil.fatal("Get State: Resolution 640 3D"), resolutionId = CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING; break;
		default: cellSysutil.fatal("Get State: Resolution unsupported(resolutionId=0x%x)", resolutionId),
			resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution); break;
		}

		switch (aspectId)
		{
		case 0x00:  cellSysutil.fatal("Get State: Aspect ratio Auto"), aspectId = CELL_VIDEO_OUT_ASPECT_AUTO; break;
		case 0x01:  cellSysutil.fatal("Get State: Aspect ratio 4:3"), aspectId = CELL_VIDEO_OUT_ASPECT_4_3; break;
		case 0x02:  cellSysutil.fatal("Get State: Aspect ratio 16:9"), aspectId = CELL_VIDEO_OUT_ASPECT_16_9; break;
		default: cellSysutil.fatal("Get State: Aspect ratio unsupported(aspectId=0x%x)", aspectId),
			aspectId = CELL_VIDEO_OUT_ASPECT_16_9; break;
		}

		switch (conversionId)
		{
		case 0x00:  cellSysutil.fatal("Get State: Conversion None"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE; break;
		case 0x01:  cellSysutil.fatal("Get State: Conversion to WXGA"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WXGA; break;
		case 0x02:  cellSysutil.fatal("Get State: Conversion to SWGA"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_SXGA; break;
		case 0x03:  cellSysutil.fatal("Get State: Conversion to WUXGA"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WUXGA; break;
		case 0x05:  cellSysutil.fatal("Get State: Conversion to 1080"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080; break;
		case 0x10:  cellSysutil.fatal("Get State: Conversion to Remote Play"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_REMOTEPLAY; break;
		case 0x80:  cellSysutil.fatal("Get State: Conversion to 720 3D Stereo"), conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING; break;
		default: cellSysutil.fatal("Get State: Conversion unsupported(conversionId=0x%x)", conversionId),
			conversionId = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE; break;
		}

		switch (refreshRatesId)
		{
		case 0x00:  cellSysutil.fatal("Get State: Refresh rates Auto"), refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_AUTO; break;
		case 0x01:  cellSysutil.fatal("Get State: Refresh rates 59.59hz"), refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ; break;
		case 0x02:  cellSysutil.fatal("Get State: Refresh rates 50hz"), refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_50HZ; break;
		case 0x04:  cellSysutil.fatal("Get State: Refresh rates 60hz"), refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_60HZ; break;
		case 0x08:  cellSysutil.fatal("Get State: Refresh rates 30hz"), refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_30HZ; break;
		default: cellSysutil.fatal("Get State: Refresh rates unsupported(refreshRatesId=0x%x)", refreshRatesId),
			refreshRatesId = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ; break;
		}

		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
{
	cellSysutil.trace("cellVideoOutGetResolution(resolutionId=0x%x, resolution=*0x%x)", resolutionId, resolution);

	if (!resolution)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	switch (resolutionId)
	{
	case 0x01: *resolution = { 0x780, 0x438 }; break;
	case 0x02: *resolution = { 0x500, 0x2d0 }; break;
	case 0x04: *resolution = { 0x2d0, 0x1e0 }; break;
	case 0x05: *resolution = { 0x2d0, 0x240 }; break;
	case 0x0a: *resolution = { 0x640, 0x438 }; break;
	case 0x0b: *resolution = { 0x5a0, 0x438 }; break;
	case 0x0c: *resolution = { 0x500, 0x438 }; break;
	case 0x0d: *resolution = { 0x3c0, 0x438 }; break;
	case 0x64: *resolution = { 0x550, 0x300 }; break;
	case 0x81: *resolution = { 0x500, 0x5be }; break;
	case 0x82: *resolution = { 0x780, 0x438 }; break;
	case 0x83: *resolution = { 0x780, 0x89d }; break;
	case 0x8b: *resolution = { 0x280, 0x5be }; break;
	case 0x8a: *resolution = { 0x320, 0x5be }; break;
	case 0x89: *resolution = { 0x3c0, 0x5be }; break;
	case 0x88: *resolution = { 0x400, 0x5be }; break;
	case 0x91: *resolution = { 0x500, 0x5be }; break;
	case 0x92: *resolution = { 0x780, 0x438 }; break;
	case 0x9b: *resolution = { 0x280, 0x5be }; break;
	case 0x9a: *resolution = { 0x320, 0x5be }; break;
	case 0x99: *resolution = { 0x3c0, 0x5be }; break;
	case 0x98: *resolution = { 0x400, 0x5be }; break;
	case 0xa1: *resolution = { 0x780, 0x438 }; break;

	default: return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.todo("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", videoOut, config, option, waitForEvent);

	cellSysutil.fatal("cellVideoOutConfigure(resolutionId=0x%x)", config->resolutionId);

	if (!config)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (!config->pitch)
	{
		return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	if (config->resolutionId == 0x92 || config->resolutionId == 0xa1)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_CONFIGURATION;
	}

	bool found = false;
	video_resolution res;

	for (const auto& ares : g_video_out_resolution_id)
	{
		if (ares.second == config->resolutionId)
		{
			found = true;
			res = ares.first;
			break;
		}
	}

	if (!found)
	{
		cellSysutil.error("Unusual resolution requested: 0x%x", config->resolutionId);
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_DISPLAY_MODE;
	}

	auto& res_info = g_video_out_resolution_map.at(res);

	auto conf = g_fxo->get<rsx::avconf>();
	conf->aspect = config->aspect;
	conf->format = config->format;
	conf->scanline_pitch = config->pitch;
	conf->resolution_x = u32(res_info.first);
	conf->resolution_y = u32(res_info.second);
	conf->state = 1;

	cellSysutil.fatal("Video Out Configure(aspect=0x%x, format=0x%x, scanline_pitch=0x%x, resolution_x=0x%x, resolution_y=0x%x)",
		conf->aspect, conf->format, conf->scanline_pitch, conf->resolution_x, conf->resolution_y);

	return CELL_OK;
}

error_code cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	auto &configId = config->resolutionId;
	auto &aspectId = config->aspect;
	cellSysutil.fatal("cellVideoOutGetConfiguration(resolutionId=0x%x, pitch=0x%x, format=%d, aspect=%d)", configId, config->pitch, config->format, config->aspect);

	if (option) *option = {};
	*config = {};

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		switch (configId)
		{
		case 0x01: cellSysutil.fatal("Get Configuration: Resolution 1080"), configId = CELL_VIDEO_OUT_RESOLUTION_1080; break;
		case 0x02: cellSysutil.fatal("Get Configuration: Resolution 720"), configId = CELL_VIDEO_OUT_RESOLUTION_720; break;
		case 0x04: cellSysutil.fatal("Get Configuration: Resolution 480"), configId = CELL_VIDEO_OUT_RESOLUTION_480; break;
		case 0x05: cellSysutil.fatal("Get Configuration: Resolution 576"), configId = CELL_VIDEO_OUT_RESOLUTION_576; break;
		case 0x0a: cellSysutil.fatal("Get Configuration: Resolution 1600"), configId = CELL_VIDEO_OUT_RESOLUTION_1600x1080; break;
		case 0x0b: cellSysutil.fatal("Get Configuration: Resolution 1440"), configId = CELL_VIDEO_OUT_RESOLUTION_1440x1080; break;
		case 0x0c: cellSysutil.fatal("Get Configuration: Resolution 1280"), configId = CELL_VIDEO_OUT_RESOLUTION_1280x1080; break;
		case 0x0d: cellSysutil.fatal("Get Configuration: Resolution 960"), configId = CELL_VIDEO_OUT_RESOLUTION_960x1080; break;
		case 0x81: cellSysutil.fatal("Get Configuration: Resolution 720 3D"), configId = CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING; break;
		case 0x88: cellSysutil.fatal("Get Configuration: Resolution 1024 3D"), configId = CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING; break;
		case 0x89: cellSysutil.fatal("Get Configuration: Resolution 960 3D"), configId = CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING; break;
		case 0x8a: cellSysutil.fatal("Get Configuration: Resolution 800 3D"), configId = CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING; break;
		case 0x8b: cellSysutil.fatal("Get Configuration: Resolution 640 3D"), configId = CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING; break;
		default: cellSysutil.fatal("Get Configuration: Aspect ratio not found(aspectId=0x%x)", configId),
			configId = g_video_out_resolution_id.at(g_cfg.video.resolution); break;
		}

		switch (aspectId)
		{
		case 0x00:  cellSysutil.fatal("Get Configuration: Aspect ratio Auto"), aspectId = CELL_VIDEO_OUT_ASPECT_AUTO; break;
		case 0x01:  cellSysutil.fatal("Get Configuration: Aspect ratio 4:3p"), aspectId = CELL_VIDEO_OUT_ASPECT_4_3; break;
		case 0x02:  cellSysutil.fatal("Get Configuration: Aspect ratio 16:9"), aspectId = CELL_VIDEO_OUT_ASPECT_16_9; break;
		default: cellSysutil.fatal("Get Configuration: Aspect ratio not found(aspectId=0x%x)", aspectId),
			aspectId = CELL_VIDEO_OUT_ASPECT_16_9; break;
		}

		config->format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		config->pitch = 4 * g_video_out_resolution_map.at(g_cfg.video.resolution).first;

		cellSysutil.fatal("cellVideoOutGetConfiguration(resolutionId=0x%x, pitch0x%x)", configId, config->pitch);

		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	//cellSysutil.fatal("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
	info->latency = 100;
	info->availableModeCount = 1;
	info->state = CELL_VIDEO_OUT_DEVICE_STATE_AVAILABLE;
	info->rgbOutputRange = 1;
	info->colorInfo.blueX = 0xFFFF;
	info->colorInfo.blueY = 0xFFFF;
	info->colorInfo.greenX = 0xFFFF;
	info->colorInfo.greenY = 0xFFFF;
	info->colorInfo.redX = 0xFFFF;
	info->colorInfo.redY = 0xFFFF;
	info->colorInfo.whiteX = 0xFFFF;
	info->colorInfo.whiteY = 0xFFFF;
	info->colorInfo.gamma = 100;
	info->availableModes[0].aspect = g_video_out_aspect_id.at(g_cfg.video.aspect_ratio);
	info->availableModes[0].conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
	info->availableModes[0].refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ | CELL_VIDEO_OUT_REFRESH_RATE_60HZ;
	info->availableModes[0].resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution);
	info->availableModes[0].scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;

	return CELL_OK;
}

error_code cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return not_an_error(1);
	case CELL_VIDEO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);
	cellSysutil.fatal("cellVideoOutGetResolutionAvailability(resolutionId=0x%x, aspect=%d)", resolutionId, aspect);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		if (g_cfg.video.monitor_3d)
		{
			return not_an_error((
				// 3D Stereo
				resolutionId == CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING
				// Resolution 2D
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_1080
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_720
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_480
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_576
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_1600x1080
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_1440x1080
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_1280x1080
				|| resolutionId == CELL_VIDEO_OUT_RESOLUTION_960x1080)
				// Aspect
				&& (aspect == CELL_VIDEO_OUT_ASPECT_AUTO || aspect == CELL_VIDEO_OUT_ASPECT_16_9)
			);
		}
		else
		{
			return not_an_error(
				resolutionId == g_video_out_resolution_id.at(g_cfg.video.resolution)
				&& (aspect == CELL_VIDEO_OUT_ASPECT_AUTO || aspect == g_video_out_aspect_id.at(g_cfg.video.aspect_ratio)));
		}

	case CELL_VIDEO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutConfigure2()
{
	cellSysutil.todo("cellVideoOutConfigure2()");
	return CELL_OK;
}

s32 cellVideoOutGetResolutionAvailability2()
{
	cellSysutil.todo("cellVideoOutGetResolutionAvailability2()");
	return CELL_OK;
}

s32 cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange)
{
	cellSysutil.todo("cellVideoOutGetConvertCursorColorInfo()");
	return CELL_OK;
}

s32 cellVideoOutDebugSetMonitorType(u32 videoOut, u32 monitorType)
{
	cellSysutil.todo("cellVideoOutDebugSetMonitorType()");
	return CELL_OK;
}

s32 cellVideoOutRegisterCallback(u32 slot, vm::ptr<CellVideoOutCallback> function, vm::ptr<void> userData)
{
	cellSysutil.todo("cellVideoOutRegisterCallback()");
	return CELL_OK;
}

s32 cellVideoOutUnregisterCallback(u32 slot)
{
	cellSysutil.todo("cellVideoOutUnregisterCallback()");
	return CELL_OK;
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution).flag(MFF_PERFECT);
	REG_FUNC(cellSysutil, cellVideoOutConfigure);
	REG_FUNC(cellSysutil, cellVideoOutConfigure2);
	REG_FUNC(cellSysutil, cellVideoOutGetConfiguration);
	REG_FUNC(cellSysutil, cellVideoOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellVideoOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability2);
	REG_FUNC(cellSysutil, cellVideoOutGetConvertCursorColorInfo);
	REG_FUNC(cellSysutil, cellVideoOutDebugSetMonitorType);
	REG_FUNC(cellSysutil, cellVideoOutRegisterCallback);
	REG_FUNC(cellSysutil, cellVideoOutUnregisterCallback);
}
