#include "stdafx.h"
#include "cellGem.h"

#include "cellCamera.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "pad_thread.h"
#include "Emu/Io/MouseHandler.h"
#include "Utilities/Timer.h"

#include "psmove.h"
#include "psmove_tracker.h"
#include "psmove_fusion.h"

#include <climits>

#define EXT_DEVICE_ID_SHARP_SHOOTER 0x8081
#define EXT_DEVICE_ID_RACING_WHEEL  0x8101

LOG_CHANNEL(cellGem);

template <>
void fmt_class_string<CellGemError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_GEM_ERROR_RESOURCE_ALLOCATION_FAILED);
			STR_CASE(CELL_GEM_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_GEM_ERROR_UNINITIALIZED);
			STR_CASE(CELL_GEM_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_GEM_ERROR_INVALID_ALIGNMENT);
			STR_CASE(CELL_GEM_ERROR_UPDATE_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_UPDATE_NOT_STARTED);
			STR_CASE(CELL_GEM_ERROR_CONVERT_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_CONVERT_NOT_STARTED);
			STR_CASE(CELL_GEM_ERROR_WRITE_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_NOT_A_HUE);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellGemStatus>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_GEM_NOT_CONNECTED);
			STR_CASE(CELL_GEM_SPHERE_NOT_CALIBRATED);
			STR_CASE(CELL_GEM_SPHERE_CALIBRATING);
			STR_CASE(CELL_GEM_COMPUTING_AVAILABLE_COLORS);
			STR_CASE(CELL_GEM_HUE_NOT_SET);
			STR_CASE(CELL_GEM_NO_VIDEO);
			STR_CASE(CELL_GEM_TIME_OUT_OF_RANGE);
			STR_CASE(CELL_GEM_NOT_CALIBRATED);
			STR_CASE(CELL_GEM_NO_EXTERNAL_PORT_DEVICE);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<move_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case move_handler::null: return "Null";
		case move_handler::fake: return "Fake";
		case move_handler::move: return "PSMove";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<psmove_number>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case psmove_number::_7: return "PSMove #7";
		case psmove_number::_6: return "PSMove #6";
		case psmove_number::_5: return "PSMove #5";
		case psmove_number::_4: return "PSMove #4";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<psmove_ext>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case psmove_ext::shooter: return "PSMove Sharp Shooter";
		case psmove_ext::wheel: return "PSMove Racing Wheel";
		case psmove_ext::null: return "Null";
		}

		return unknown;
	});
}

// **********************
// * HLE helper structs *
// **********************

namespace move
{
	struct gem_config
	{
		atomic_t<u32> state = 0;

		struct gem_color
		{
			float r, g, b;

			gem_color() : r(0.0f), g(0.0f), b(0.0f) {}
			gem_color(float r_, float g_, float b_)
			{
				r = clamp(r_);
				g = clamp(g_);
				b = clamp(b_);
			}

			float clamp(float f) const
			{
				return std::max(0.0f, std::min(f, 1.0f));
			}

			bool is_black() const
			{
				return r == 0.0f && g == 0.0f && b == 0.0f;
			}
		};

		struct PSMoveDeleter
		{
			void operator()(PSMove* p)
			{
				psmove_disconnect(p);
			}
		};
		struct PSMoveTrackerDeleter
		{
			void operator()(PSMoveTracker* p)
			{
				psmove_tracker_free(p);
			}
		};
		struct PSMoveFusionDeleter
		{
			void operator()(PSMoveFusion* p)
			{
				psmove_fusion_free(p);
			}
		};

		struct gem_controller
		{
			u32 status = CELL_GEM_STATUS_DISCONNECTED;         // connection status (CELL_GEM_STATUS_DISCONNECTED or CELL_GEM_STATUS_READY)
			u32 ext_status = CELL_GEM_NO_EXTERNAL_PORT_DEVICE; // external port connection status
			u32 port = 7;                                      // assigned port
			bool enabled_magnetometer = false;    // whether the magnetometer is enabled (probably used for additional rotational precision)
			bool calibrated_magnetometer = false; // whether the magnetometer is calibrated
			bool enabled_filtering = false;       // whether filtering is enabled
			bool enabled_tracking = false;        // whether tracking is enabled
			bool enabled_LED = false;             // whether the LED is enabled
			u8 rumble = 0;                        // rumble intensity
			gem_color sphere_rgb = {};            // RGB color of the sphere LED
			u32 hue = CELL_GEM_HUE_NOT_SET;       // tracking hue of the motion controller

			// PSMoveAPI per-controller data
			struct
			{
				std::unique_ptr<PSMove, PSMoveDeleter> handle;
				std::string serial;      // unique identifier (Bluetooth MAC address)
				bool enable_orientation = false; // whether `psmove_enable_orientation` has been called and succeeded
			} psmove;

		};

		// PSMoveAPI data
		struct
		{
			std::unique_ptr<PSMoveTracker, PSMoveTrackerDeleter> tracker;
			std::unique_ptr<PSMoveFusion, PSMoveFusionDeleter> fusion;
			s32 connected_tracker_controllers;
		} psmove;

		CellGemAttribute attribute;
		CellGemVideoConvertAttribute vc_attribute;
		u64 status_flags = CELL_GEM_NOT_CALIBRATED;
		bool enable_pitch_correction = false;
		u32 inertial_counter;

		std::array<gem_controller, CELL_GEM_MAX_NUM> controllers;
		u32 connected_controllers;

		Timer timer;

		// helper functions
		bool is_controller_ready(u32 gem_num) const
		{
			return controllers[gem_num].status == CELL_GEM_STATUS_READY;
		}

		bool is_ext_connected(u32 gem_num) const
		{
			return controllers[gem_num].ext_status == CELL_GEM_EXT_CONNECTED;
		}

		void reset_controller(gem_config* gem, u32 gem_num);
	};

	// ************************
	// * HLE helper functions *
	// ************************

	void gem_config::reset_controller(gem_config* gem, u32 gem_num)
	{
		switch (g_cfg.io.move)
		{
			default:
			case move_handler::null:
			{
				connected_controllers = 0;
				break;
			}
			case move_handler::fake:
			{
				connected_controllers = 1;
				break;
			}
			case move_handler::move:
			{
				connected_controllers = gem->connected_controllers;
				break;
			}
		}

		// Assign status and port number
		if (gem_num < connected_controllers)
		{
			int psmove = 7u;
			switch (g_cfg.io.move_number)
			{
			default:
			case psmove_number::_7: break;
			case psmove_number::_6: psmove = 6u; break;
			case psmove_number::_5: psmove = 5u; break;
			case psmove_number::_4: psmove = 4u; break;
			}

			controllers[gem_num].status = CELL_GEM_STATUS_READY;
			controllers[gem_num].port = psmove - gem_num;

			if (!(g_cfg.io.move_ext == psmove_ext::null))
			{
				controllers[gem_num].ext_status = CELL_GEM_EXT_CONNECTED;
			}
			else
			{
				controllers[gem_num].ext_status = CELL_GEM_NO_EXTERNAL_PORT_DEVICE;
			}
		}
		else
		{
			controllers[gem_num].status = CELL_GEM_STATUS_DISCONNECTED;
			controllers[gem_num].port = 0;
		}
	}

	/**
	 * \brief Verifies that a Move controller id is valid
	 * \param gem_num Move controler ID to verify
	 * \return True if the ID is valid, false otherwise
	 */
	static bool check_gem_num(const u32 gem_num)
	{
		return gem_num >= 0 && gem_num < CELL_GEM_MAX_NUM;
	}

	std::pair<u32, u32> get_format_resolution(CellGemVideoConvertFormatEnum format)
	{
		std::pair<u32, u32> res;
		switch (format)
		{
		case CELL_GEM_RGBA_640x480:
		case CELL_GEM_YUV_640x480:
		case CELL_GEM_YUV422_640x480:
		case CELL_GEM_YUV411_640x480:
		case CELL_GEM_BAYER_RESTORED: 
			return { 640, 480 };
		case CELL_GEM_RGBA_320x240:
		case CELL_GEM_BAYER_RESTORED_RGGB:
		case CELL_GEM_BAYER_RESTORED_RASTERIZED:
			 return { 320, 240 };
		case CELL_GEM_NO_VIDEO_OUTPUT:
		default:
			return { 0, 0 };
		}
	}

	namespace psmoveapi
	{
		static void init(gem_config* gem)
		{
			if (!psmove_init(PSMOVE_CURRENT_VERSION))
			{
				fmt::throw_exception("Couldn't initialize PSMoveAPI", HERE);
				// TODO: Don't die
			}

			// const auto g_psmove = fxm::make<psmoveapi_thread>();

			gem->connected_controllers = psmove_count_connected();

			for (auto id = 0; id < gem->connected_controllers; ++id)
			{
				PSMove* controller_handle = psmove_connect_by_id(id);

				if (controller_handle)
				{
					const std::string serial = psmove_get_serial(controller_handle);

					auto& gem_controller = gem->controllers[id];

					cellGem.fatal("%d PSMove connected.", gem->connected_controllers);

					gem_controller.psmove.serial = serial;
					gem_controller.psmove.handle.reset(controller_handle);

					// g_psmove->register_controller(id, connected_controller);

					psmove_set_orientation_fusion_type(controller_handle, PSMoveOrientation_Fusion_Type::OrientationFusion_ComplementaryMARG);
					psmove_enable_orientation(controller_handle, PSMove_True);
					gem_controller.psmove.enable_orientation = psmove_has_orientation(controller_handle);
				}
			}
		}

		static void end(const gem_config::gem_controller* gem)
		{
			const auto handle = gem->psmove.handle.get();
			psmove_disconnect(handle);
		}

		// TODO: In forcegrgb: if tracker: maybe do psmove_tracker_enable_with_color

		namespace tracker
		{
			static void init(gem_config* gem)
			{
				PSMoveTrackerSettings settings;
				psmove_tracker_settings_set_default(&settings);
				settings.color_mapping_max_age = 0;
				settings.exposure_mode = Exposure_LOW;

				auto shared_data = g_fxo->get<gem_camera_shared>();

				settings.camera_mirror = static_cast<PSMove_Bool>(shared_data->attr.load()[CELL_CAMERA_MIRRORFLAG].v1);

				// If output_format is CELL_GEM_NO_VIDEO_OUTPUT, get_format_resolution will return 0, so
				// psmoveapi will auto pick camera resolution. This is fine, since the vc_attribute is
				// about video data meant to be _shown_ (converted) whereas psmoveapi uses the size for
				// tracking (a size of 0 would make no sense). For the rest of the cases though, it's fine
				// to do the conversion, since on a real PS3 the camera would use the picked (vc_attribute)
				// resolution for tracking too.
				std::tie(settings.camera_frame_width, settings.camera_frame_height) = get_format_resolution(gem->vc_attribute.output_format);

				settings.camera_frame_rate = shared_data->frame_rate.load();
				settings.camera_auto_white_balance = static_cast<PSMove_Bool>(gem->vc_attribute.conversion_flags & CELL_GEM_AUTO_WHITE_BALANCE);
				// settings.camera_gain = gem->vc_attribute.gain;

				auto tracker = gem->psmove.tracker.get();
				auto fusion = gem->psmove.fusion.get();
				tracker = psmove_tracker_new_with_settings(&settings);
				fusion = psmove_fusion_new(tracker, 1., 1000.);
				if (!tracker)
				{
					fmt::throw_exception("Gem: Couldn't initialize PSMoveAPI Tracker", HERE);
					// TODO: Don't die
				}

				const auto connected = psmove_tracker_count_connected();
				gem->psmove.connected_tracker_controllers = connected;

				for (auto id = 0; id < connected; ++id)
				{
					auto* handle = gem->controllers[id].psmove.handle.get();
					if (handle)
					{
						psmove_tracker_set_auto_update_leds(tracker, handle, PSMove_True);
						psmove_tracker_set_exposure(tracker, PSMoveTracker_Exposure::Exposure_LOW);
					}
				}
				gem->psmove.tracker.reset(tracker);
				gem->psmove.fusion.reset(fusion);
			}

			PSMoveTracker_Status enable(gem_config* gem, u32 gem_num)
			{
				const auto& controller = gem->controllers[gem_num];
				const auto handle = controller.psmove.handle.get();
				const auto tracker = gem->psmove.tracker.get();
				const auto color = controller.sphere_rgb;

				psmove_reset_orientation(handle);

				if (!tracker)
				{
					LOG_FATAL(HLE, "PSMoveAPI: Tracker not initialized, can't enable controller.");
					return Tracker_NOT_CALIBRATED;
				}

				PSMoveTracker_Status tracker_status;

				// enable tracker for controller
				if (color.is_black())
				{
					tracker_status = psmove_tracker_enable(tracker, handle);
				}
				else
				{
					tracker_status = psmove_tracker_enable_with_color(tracker, handle,
						color.r * 255, color.g * 255, color.b * 255);
				}

				switch (tracker_status)
				{
				case Tracker_CALIBRATED: LOG_FATAL(HLE, "PSMoveAPI: Calibrated"); break;
				case Tracker_NOT_CALIBRATED: LOG_ERROR(HLE, "PSMoveAPI: Controller not registered with tracker"); break;
				case Tracker_CALIBRATION_ERROR: LOG_ERROR(HLE, "PSMoveAPI: Calibration failed (check lighting, visibility)"); break;
				case Tracker_TRACKING: LOG_ERROR(HLE, "PSMoveAPI: Calibrated and successfully tracked in the camera"); break;
				default: break;
				}
				return tracker_status;
			}

			void force_enable(gem_config* gem, u32 gem_num)
			{
				const auto& controller = gem->controllers[gem_num];
				const auto handle = controller.psmove.handle.get();
				const auto tracker = gem->psmove.tracker.get();
				const auto color = controller.sphere_rgb;

				psmove_reset_orientation(handle);

				// force enable tracker for controller
				for (;;)
				{
					PSMoveTracker_Status tracker_status;

					if (color.is_black())
					{
						tracker_status = psmove_tracker_enable(tracker, handle);
					}
					else
					{
						tracker_status = psmove_tracker_enable_with_color(tracker, handle,
							color.r * 255, color.g * 255, color.b * 255);
					}

					if (tracker_status == Tracker_CALIBRATED)
					{
						cellGem.error("PSMoveAPI: Calibrated");
						psmove_reset_orientation(handle);
						gem->status_flags = CELL_GEM_FLAG_CALIBRATION_OCCURRED | CELL_GEM_FLAG_CALIBRATION_SUCCEEDED;
						break;
					}
					else if (tracker_status == Tracker_CALIBRATION_ERROR)
					{
						cellGem.fatal("PSMoveAPI: Calibration failed (check lighting, visibility)");
						gem->status_flags = CELL_GEM_FLAG_CALIBRATION_FAILED_CANT_FIND_SPHERE;
						break;
					}
				}
			}

			void disable(gem_config* gem, u32 gem_num)
			{
				const auto& handle = gem->controllers[gem_num].psmove.handle.get();

				psmove_tracker_disable(gem->psmove.tracker.get(), handle);
			}

			void update(gem_config* gem)
			{
				const auto tracker = gem->psmove.tracker.get();

				// TODO: Run in cellCamera?
				psmove_tracker_update_image(tracker);

				const auto connected = gem->psmove.connected_tracker_controllers;
				for (auto id = 0; id < connected; ++id)
				{
					const auto& handle = gem->controllers[id].psmove.handle.get();

					psmove_tracker_update(tracker, handle);
				}
			}

			namespace map
			{
				static bool gem_state(const gem_config::gem_controller& controller, vm::ptr<CellGemState>& gem_state)
				{
					if (!gem_state)
					{
						return false;
					}

					const auto gem = fxm::get<gem_config>();
					const auto handle = controller.psmove.handle.get();
					const auto tracker = gem->psmove.tracker.get();
					const auto fusion = gem->psmove.fusion.get();

					s32 width, height;
					psmove_tracker_get_size(tracker, &width, &height);

					float fx, fy, z, x, y, r;

					psmove_fusion_get_position(fusion, handle, &fx, &fy, &z);

					psmove_tracker_get_position(tracker, handle, &x, &y, &r);
					const auto distance = psmove_tracker_distance_from_radius(tracker, r) * 20; // cm->mm Multiplie distance x2
					const auto fx3 = fx * 30, fy3 = -fy * 30, z3 = -z * 30;

					gem_state->pos[0] = -x;
					gem_state->pos[1] = y;
					gem_state->pos[2] = distance;
					gem_state->pos[3] = 0;

					// RE5 cut, Epic Mickey
					gem_state->vel[0] = 0;
					gem_state->vel[1] = 0;
					gem_state->vel[2] = 0;
					gem_state->vel[3] = 0;

					gem_state->accel[0] = 0;
					gem_state->accel[1] = 0;
					gem_state->accel[2] = 0;
					gem_state->accel[3] = 0;

					// Tumble Action rotate, Menu select Kung fuu rider
					gem_state->angvel[0] = 0;
					gem_state->angvel[1] = 0;
					gem_state->angvel[2] = 0;
					gem_state->angvel[3] = 0;

					// Tumble Action rotate
					gem_state->angaccel[0] = 0;
					gem_state->angaccel[1] = 0;
					gem_state->angaccel[2] = 0;
					gem_state->angaccel[3] = 0;

					gem_state->handle_pos[0] = fx3;
					gem_state->handle_pos[1] = fy3;
					gem_state->handle_pos[2] = z3;
					gem_state->handle_pos[3] = 0;

					gem_state->handle_vel[0] = 0;
					gem_state->handle_vel[1] = 0;
					gem_state->handle_vel[2] = 0;
					gem_state->handle_vel[3] = 0;

					gem_state->handle_accel[0] = 0;
					gem_state->handle_accel[1] = 0;
					gem_state->handle_accel[2] = 0;
					gem_state->handle_accel[3] = 0;

					//cellGem.fatal("accel[0]: %+01.3f accel[1]: %+01.3f [2]: %+01.3f", gem_state->accel[0], gem_state->accel[1], gem_state->accel[2]);
					//cellGem.fatal("angvel[0]: %+01.3f, [1]: %+01.3f, [2]: %+01.3f, [3]: %+01.3f", gem_state->angvel[0], gem_state->angvel[1], gem_state->angvel[2], gem_state->angvel[3]);
					//cellGem.fatal("handle_pos[0]: %+01.3f, [1]: %+01.3f, [2]: %+01.3f, [3]: %+01.3f", gem_state->handle_pos[0], gem_state->handle_pos[1], gem_state->handle_pos[2], gem_state->handle_pos[3]);
					//cellGem.fatal("vel[0]: %+01.3f vel[1]: %+01.3f vel[2]: %+01.3f", gem_state->vel[0], gem_state->vel[1], gem_state->vel[2]);
					//cellGem.fatal("pos[0]: %+01.3f, [1]: %+01.3f, [2]: %01.3f}", gem_state->pos[0], gem_state->pos[1], gem_state->pos[2]);

					return true;
				}

				static bool gem_image_state(vm::ptr<CellGemImageState>& gem_image_state, PSMoveTracker* tracker, PSMove* handle)//const gem_t::gem_controller& controller, vm::ptr<CellGemImageState>& gem_image_state)
				{
					const auto shared_data = g_fxo->get<gem_camera_shared>();
					/*const auto gem = fxm::get<gem_t>();
					const auto handle = controller.psmove.handle.get();
					const auto tracker = gem->psmove.tracker.get();*/

					s32 width, height;
					psmove_tracker_get_size(tracker, &width, &height);

					float x, y, radius;
					
					const auto age = psmove_tracker_get_position(tracker, handle, &x, &y, &radius) * 1000; // ms -> us
					const auto distance = psmove_tracker_distance_from_radius(tracker, radius) * 20; // cm -> mm * 2

					if (age == -1)
					{
						cellGem.fatal("PSMoveAPI: Error getting tracker position.");
						return false;
					}

					gem_image_state->frame_timestamp = shared_data->frame_timestamp.load();
					gem_image_state->timestamp = gem_image_state->frame_timestamp + age;
					gem_image_state->u = x;
					gem_image_state->v = y;
					gem_image_state->r = radius;
					gem_image_state->projectionx = x - width / 2;
					gem_image_state->projectiony = -y + height / 2;
					gem_image_state->distance = distance;
					gem_image_state->visible = true;
					gem_image_state->r_valid = true;

					//cellGem.fatal("u: %+01.3f, v: %+01.3f, r: %+01.3f, projX: %+01.3f, projY: %+01.3f, dist: %+01.3f}",
						//gem_image_state->u, gem_image_state->v, gem_image_state->r, gem_image_state->projectionx, gem_image_state->projectiony, gem_image_state->distance);

					return true;
				}
			} // namespace map tracker

		} // namespace tracker

		bool poll(PSMove* controller_handle)
		{
			bool poll_success = false;

			// consume all buffered data
			while (psmove_poll(controller_handle))
			{
				poll_success = true;
			}

			return poll_success;
		}

		bool poll(const gem_config::gem_controller& controller)
		{
			return move::psmoveapi::poll(controller.psmove.handle.get());
		}

		// TODO(?): caching for rumble/color updates

		void update_rumble(gem_config::gem_controller& controller)
		{
			const auto handle = controller.psmove.handle.get();

			psmove_set_rumble(handle, controller.rumble);
			psmove_update_leds(handle);
		}

		void update_color(const gem_config::gem_controller& controller)
		{
			const auto color = controller.sphere_rgb;
			const auto handle = controller.psmove.handle.get();

			psmove_set_leds(handle, color.r * 255, color.g * 255, color.b * 255);
			psmove_update_leds(handle);
		}

		void reset_orientation(const gem_config::gem_controller& controller)
		{
			auto handle = controller.psmove.handle.get();
			psmove_reset_orientation(handle);
		}

		static bool get_ext_info(const gem_config::gem_controller& controller, vm::ptr<u32>& ext_id, vm::ptr<u8[CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE]> ext_info)
		{
			auto handle = controller.psmove.handle.get();
			int ext_connected = 0;

			if (!handle)
			{
				
			}

			if (psmove_is_ext_connected(handle))
			{
				/* if the extension device was not connected before, report connect */
				if (!ext_connected)
				{
					PSMove_Ext_Device_Info ext;
					enum PSMove_Bool success = psmove_get_ext_device_info(handle, &ext);
					if (success)
					{
						switch (ext.dev_id)
						{
						case EXT_DEVICE_ID_SHARP_SHOOTER:
							*ext_id = EXT_DEVICE_ID_SHARP_SHOOTER;
							cellGem.fatal("Sharp Shooter extension connected!");
							break;
						case EXT_DEVICE_ID_RACING_WHEEL:
							*ext_id = EXT_DEVICE_ID_RACING_WHEEL;
							cellGem.fatal("Racing Wheel extension connected!");
							break;
						default:
							cellGem.fatal("Unknown extension device (id 0x%04X) connected!", ext.dev_id);
							break;
						}
					}
					else
					{
						cellGem.fatal("Unknown extension device connected! Failed to get device info.");
					}
				}

				ext_connected = 1;
				return true;
			}
			else
			{
				return false;
			}			
		}

		static bool wheel_input_to_ext(const gem_config::gem_controller& controller, PSMove_Ext_Data* data, CellGemExtPortData& ext)
		{
			const auto handle = controller.psmove.handle.get();
			unsigned int move_buttons = psmove_get_buttons(handle);
			unsigned char send_buf[3] = { 0x20, 0x00, 0x00 };

			static unsigned char last_l2 = 0;
			static unsigned char last_r2 = 0;

			unsigned char throttle = (*data)[0];
			unsigned char l2 = (*data)[1];
			unsigned char r2 = (*data)[2];
			unsigned char c = (*data)[3];

			static bool flip = false;
			ext.status = (flip ? CELL_GEM_EXT_CONNECTED : 0) | CELL_GEM_EXT_EXT0 | CELL_GEM_EXT_EXT1;
			ext.analog_left_x = 1;
			ext.analog_left_y = 1;
			ext.analog_right_x = 1;
			ext.analog_right_y = 1;
			ext.digital1 = l2;
			ext.digital2 = r2;
			ext.custom[0] = throttle;

			//cellGem.fatal("Statut: %+01.3f, Throttle: %+01.3f, L2: %+01.3f,  R2: %+01.3f", ext.status, ext.custom[0], ext.digital1, ext.digital2);
			//cellGem.fatal("Throttle: %3d,  L2: %3d,  R2: %3d",
				//throttle, l2, r2);

			return true;
		}

		static bool input_to_pad(const gem_config::gem_controller& controller, be_t<u16>& digital_buttons, be_t<u16>& analog_t)
		{
			const auto handle = controller.psmove.handle.get();
			const auto buttons = psmove_get_buttons(handle);
			const auto trigger = psmove_get_trigger(handle);

			memset(&digital_buttons, 0, sizeof(digital_buttons));

			if (buttons & Btn_MOVE)
				digital_buttons |= CELL_GEM_CTRL_MOVE;
			if (buttons & Btn_T)
				//cellGem.fatal("Press Trigger"),
				digital_buttons |= CELL_GEM_CTRL_T;

			if (buttons & Btn_CROSS)
				digital_buttons |= CELL_GEM_CTRL_CROSS;
			if (buttons & Btn_CIRCLE)
				digital_buttons |= CELL_GEM_CTRL_CIRCLE;
			if (buttons & Btn_SQUARE)
				digital_buttons |= CELL_GEM_CTRL_SQUARE;
			if (buttons & Btn_TRIANGLE)
				digital_buttons |= CELL_GEM_CTRL_TRIANGLE;
			if (buttons & Btn_SELECT)
				digital_buttons |= CELL_GEM_CTRL_SELECT;
			if (buttons & Btn_START)
				digital_buttons |= CELL_GEM_CTRL_START;

			analog_t = trigger * float(USHRT_MAX) / float(UCHAR_MAX);

			return true;
		}

		static bool sensor_to_gem(const gem_config::gem_controller& controller, vm::ptr<CellGemState>& gem_state)
		{
			const auto handle = controller.psmove.handle.get();

			if (!gem_state)
			{
				cellGem.fatal("input_to_gem error");
				return false;
			}

			if (controller.psmove.enable_orientation)
			{
				float w, x, y, z;
				psmove_get_orientation(handle, &w, &x, &y, &z);
				gem_state->quat[0] = x;
				gem_state->quat[1] = y;
				gem_state->quat[2] = z;
				gem_state->quat[3] = w;
			}

			const float temp = psmove_get_temperature(handle);
			gem_state->temperature = temp;

			//cellGem.fatal("w: %+01.3f, x: %+01.3f, y: %+01.3f, z: %+01.3f}", gem_state->quat[0], gem_state->quat[1], gem_state->quat[2], gem_state->quat[3]);

			return true;
		}

		static bool sensor_to_inertial(const gem_config::gem_controller& controller, vm::ptr<CellGemInertialState>& inertial_state)
		{
			if (!inertial_state)
			{
				cellGem.fatal("fail sensor to inertial");
				return false;
			}

			const auto handle = controller.psmove.handle.get();

			if (!psmove_has_calibration(handle))
			{
				fmt::throw_exception("You need to calibrate your Move Motion controller!");
			}

			// raw readings
			int rax, ray, raz;
			psmove_get_accelerometer(handle, &rax, &ray, &raz);

			int rgx, rgy, rgz;
			psmove_get_gyroscope(handle, &rgx, &rgy, &rgz);

			// processed readings
			float ax, ay, az;
			psmove_get_accelerometer_frame(handle, Frame_SecondHalf, &ax, &ay, &az);
			inertial_state->accelerometer[0] = ax;
			inertial_state->accelerometer[1] = az;
			inertial_state->accelerometer[2] = ay;
			inertial_state->accelerometer[3] = 0;

			float gx, gy, gz;
			psmove_get_gyroscope_frame(handle, Frame_SecondHalf, &gx, &gy, &gz);
			inertial_state->gyro[0] = gx;
			inertial_state->gyro[1] = gz;
			inertial_state->gyro[2] = gy;
			inertial_state->gyro[3] = 0;

			// biases
			auto ac_b = &inertial_state->accelerometer_bias;
			*ac_b[0] = ax - rax;
			*ac_b[1] = ay - ray;
			*ac_b[2] = az - raz;
			*ac_b[3] = 0;

			auto gr_b = &inertial_state->gyro_bias;
			*gr_b[0] = gx - rgx;
			*gr_b[1] = gy - rgy;
			*gr_b[2] = gz - rgz;
			*gr_b[3] = 0;

			// raw readings for biases
			/*int rax, ray, raz;
			psmove_get_accelerometer(handle, &rax, &ray, &raz);
			inertial_state->accelerometer_bias[0] = rax;
			inertial_state->accelerometer_bias[1] = ray;
			inertial_state->accelerometer_bias[2] = raz;
			inertial_state->accelerometer_bias[3] = 0;

			int rgx, rgy, rgz;
			psmove_get_gyroscope(handle, &rgx, &rgy, &rgz);
			inertial_state->gyro_bias[0] = rgx;
			inertial_state->gyro_bias[1] = rgy;
			inertial_state->gyro_bias[2] = rgz;
			inertial_state->gyro_bias[3] = 0;*/

			// Read temperature PSMove
			inertial_state->temperature = psmove_get_temperature(handle);

			//cellGem.fatal("Accel: { x: %+01.3f y: %+01.3f z: %+01.3f}", inertial_state->accelerometer[0], inertial_state->accelerometer[1], inertial_state->accelerometer[2], inertial_state->accelerometer[3]);
			//cellGem.fatal("Gyro: { x: %+01.3f y: %+01.3f z: %+01.3f}", inertial_state->gyro[0], inertial_state->gyro[1], inertial_state->gyro[2], inertial_state->gyro[3]);
			//cellGem.fatal("Gyro bias: { x: %+01.3f y: %+01.3f z: %+01.3f}", inertial_state->gyro_bias[0], inertial_state->gyro_bias[1], inertial_state->gyro_bias[2], inertial_state->gyro_bias[3]);

			return true;
		}

	} // namespace psmoveapi

	namespace map
	{
		/**
			* \brief Maps Move controller data (digital buttons, and analog Trigger data) to DS3 pad input.
			*        Unavoidably buttons conflict with DS3 mappings, which is problematic for some games.
			* \param port_no DS3 port number to use
			* \param digital_buttons Bitmask filled with CELL_GEM_CTRL_* values
			* \param analog_t Analog value of Move's Trigger. Currently mapped to R2.
			* \return true on success, false if port_no controller is invalid
			*/
		static bool ds3_input_to_pad(const u32 port_no, be_t<u16>& digital_buttons, be_t<u16>& analog_t)
		{
			std::lock_guard lock(pad::g_pad_mutex);

			const auto handler = pad::get_current_handler();

			const PadInfo& rinfo = handler->GetInfo();

			auto& pads = handler->GetPads();
			auto pad = pads[port_no];

			if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
				return false;

			for (Button& button : pad->m_buttons)
			{
				//	here we check btns, and set pad accordingly,
				if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
				{
					if (button.m_pressed)
						pad->m_digital_2 |= button.m_outKeyCode;
					else
						pad->m_digital_2 &= ~button.m_outKeyCode;

					switch (button.m_outKeyCode)
					{
					case CELL_PAD_CTRL_SQUARE:
						pad->m_press_square = button.m_value;
						break;
					case CELL_PAD_CTRL_CROSS:
						pad->m_press_cross = button.m_value;
						break;
					case CELL_PAD_CTRL_CIRCLE:
						pad->m_press_circle = button.m_value;
						break;
					case CELL_PAD_CTRL_TRIANGLE:
						pad->m_press_triangle = button.m_value;
						break;
					case CELL_PAD_CTRL_R1:
						pad->m_press_R1 = button.m_value;
						break;
					case CELL_PAD_CTRL_L1:
						pad->m_press_L1 = button.m_value;
						break;
					case CELL_PAD_CTRL_R2:
						pad->m_press_R2 = button.m_value;
						break;
					case CELL_PAD_CTRL_L2:
						pad->m_press_L2 = button.m_value;
						break;
					default: break;
					}
				}

				if (button.m_flush)
				{
					button.m_pressed = false;
					button.m_flush = false;
					button.m_value = 0;
				}
			}

			memset(&digital_buttons, 0, sizeof(digital_buttons));

			// map the Move key to R1 and the Trigger to R2

			if (pad->m_press_R1)
				digital_buttons |= CELL_GEM_CTRL_MOVE;
			if (pad->m_press_R2)
				digital_buttons |= CELL_GEM_CTRL_T;

			if (pad->m_press_cross)
				digital_buttons |= CELL_GEM_CTRL_CROSS;
			if (pad->m_press_circle)
				digital_buttons |= CELL_GEM_CTRL_CIRCLE;
			if (pad->m_press_square)
				digital_buttons |= CELL_GEM_CTRL_SQUARE;
			if (pad->m_press_triangle)
				digital_buttons |= CELL_GEM_CTRL_TRIANGLE;
			if (pad->m_digital_1)
				digital_buttons |= CELL_GEM_CTRL_SELECT;
			if (pad->m_digital_2)
				digital_buttons |= CELL_GEM_CTRL_START;

			analog_t = pad->m_press_R2;

			return true;
		}

		/**
			* \brief Maps external Move controller data to DS3 input
			*	      Implementation detail: CellGemExtPortData's digital/analog fields map the same way as
			*	      libPad, so no translation is needed.
			* \param port_no DS3 port number to use
			* \param ext External data to modify
			* \return true on success, false if port_no controller is invalid
			*/
		static bool ds3_input_to_ext(const u32 port_no, CellGemExtPortData& ext)
		{
			std::lock_guard lock(pad::g_pad_mutex);

			const auto handler = pad::get_current_handler();

			auto& pads = handler->GetPads();
			const PadInfo& rinfo = handler->GetInfo();

			auto pad = pads[port_no];

			if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
				return false;

			ext.status = 0; // CELL_GEM_EXT_CONNECTED | CELL_GEM_EXT_EXT0 | CELL_GEM_EXT_EXT1
			ext.analog_left_x = pad->m_analog_left_x;
			ext.analog_left_y = pad->m_analog_left_y;
			ext.analog_right_x = pad->m_analog_right_x;
			ext.analog_right_y = pad->m_analog_right_y;
			ext.digital1 = pad->m_digital_1;
			ext.digital2 = pad->m_digital_2;

			return true;
		}

		namespace mouse
		{
			/**
			* \brief Maps Move controller data (digital buttons, and analog Trigger data) to mouse input.
			*        Move Button: Mouse1
			*        Trigger:     Mouse2
			* \param mouse_no Mouse index number to use
			* \param digital_buttons Bitmask filled with CELL_GEM_CTRL_* values
			* \param analog_t Analog value of Move's Trigger.
			* \return true on success, false if mouse mouse_no is invalid
			*/
			static bool input_to_pad(const u32 mouse_no, be_t<u16>& digital_buttons, be_t<u16>& analog_t)
			{
				auto handler = fxm::get<MouseHandlerBase>();
				std::lock_guard lock(handler->mutex);
				if (!handler || mouse_no >= handler->GetMice().size())
				{
					cellGem.fatal("Mouse problem");
					return false;
				}

				memset(&digital_buttons, 0, sizeof(digital_buttons));
				MouseDataList& mouse_data_list = handler->GetDataList(mouse_no);

				if (mouse_data_list.size())
				{
					const MouseData& mouse_data = mouse_data_list.front();
					if (mouse_data.buttons & CELL_MOUSE_BUTTON_1)
						digital_buttons |= CELL_GEM_CTRL_T;
					if (mouse_data.buttons & CELL_MOUSE_BUTTON_2)
						digital_buttons |= CELL_GEM_CTRL_MOVE;
					if (mouse_data.buttons & CELL_MOUSE_BUTTON_3)
						digital_buttons |= CELL_GEM_CTRL_CROSS;
					if (mouse_data.buttons & CELL_MOUSE_BUTTON_3)
						cellGem.fatal("Start pressed"),
						digital_buttons |= CELL_GEM_CTRL_START;
					analog_t = mouse_data.buttons & CELL_MOUSE_BUTTON_1 ? 0xFFFF : 0;
					mouse_data_list.pop_front();
				}
				return true;
			}
			
			static bool input_to_gem(const u32 mouse_no, vm::ptr<CellGemState>& gem_state)
			{
				const auto handler = fxm::get<MouseHandlerBase>();
				if (!gem_state || !handler || mouse_no >= handler->GetMice().size())
				{
					return false;
				}

				auto& mouse = handler->GetMice().at(0);
				f32 x_pos = mouse.x_pos;
				f32 y_pos = mouse.y_pos;
				static constexpr auto aspect_ratio = 1.2;
				static constexpr auto screen_offset_x = 400.0;
				static constexpr auto screen_offset_y = screen_offset_x * aspect_ratio;
				static constexpr auto screen_scale = 3.0;
				gem_state->pos[0] = screen_offset_x / screen_scale + x_pos / screen_scale;
				gem_state->pos[1] = screen_offset_y / screen_scale + -y_pos / screen_scale * aspect_ratio;
				gem_state->pos[2] = 2000;
				gem_state->pos[3] = 0;
				gem_state->handle_pos[0] = screen_offset_x / screen_scale + x_pos / screen_scale;
				gem_state->handle_pos[1] = screen_offset_y / screen_scale + -y_pos / screen_scale * aspect_ratio;
				gem_state->handle_pos[2] = 2000;
				gem_state->handle_pos[3] = 0;
				//cellGem.fatal("pos[0]: %+01.3f pos[1]: %+01.3f, pos[2]: %+01.3f}", gem_state->pos[0], gem_state->pos[1], gem_state->pos[2]);
				//cellGem.fatal("handle_pos[0]: %+01.3f, [1]: %+01.3f, [2]: %+01.3f}", gem_state->handle_pos[0], gem_state->handle_pos[1], gem_state->handle_pos[2]);
				return true;
			}
		} // namespace mouse
	} // namespace map
} // namespace move

// *********************
// * cellGem functions *
// *********************

using namespace move;

error_code cellGemCalibrate(u32 gem_num)
{
	cellGem.fatal("cellGemCalibrate(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->controllers[gem_num].calibrated_magnetometer = true;
	auto& controller = gem->controllers[gem_num];

	if (g_cfg.io.move == move_handler::fake)
	{
		gem->controllers[gem_num].calibrated_magnetometer = true;
		gem->status_flags = CELL_GEM_FLAG_CALIBRATION_OCCURRED | CELL_GEM_FLAG_CALIBRATION_SUCCEEDED;
	}
	else if (g_cfg.io.move == move_handler::move)
	{

		const auto tracking_result = move::psmoveapi::tracker::enable(gem, gem_num);

		switch (tracking_result)
		{
		case Tracker_NOT_CALIBRATED: break;
		case Tracker_CALIBRATION_ERROR: gem->status_flags |= CELL_GEM_FLAG_CALIBRATION_FAILED_CANT_FIND_SPHERE | CELL_GEM_FLAG_CALIBRATION_FAILED_BRIGHT_LIGHTING; break;
		case Tracker_CALIBRATED:
		case Tracker_TRACKING: gem->status_flags |= CELL_GEM_FLAG_CALIBRATION_SUCCEEDED | CELL_GEM_FLAG_CALIBRATION_OCCURRED; break;
		default: break;
		}
	}

	return CELL_OK;
}

error_code cellGemClearStatusFlags(u32 gem_num, u64 mask)
{
	cellGem.todo("cellGemClearStatusFlags(gem_num=%d, mask=0x%x)", gem_num, mask);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->status_flags &= ~mask;

	return CELL_OK;
}

error_code cellGemConvertVideoFinish()
{
	cellGem.todo("cellGemConvertVideoFinish()");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellGemConvertVideoStart(vm::cptr<void> video_frame)
{
	cellGem.todo("cellGemConvertVideoStart(video_frame=*0x%x)", video_frame);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellGemEnableCameraPitchAngleCorrection(u32 enable_flag)
{
	cellGem.todo("cellGemEnableCameraPitchAngleCorrection(enable_flag=%d)", enable_flag);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	gem->enable_pitch_correction = !!enable_flag;

	return CELL_OK;
}

error_code cellGemEnableMagnetometer(u32 gem_num, int flag)
{
	cellGem.fatal("cellGemEnableMagnetometer(gem_num=%d, flag=0x%x)", gem_num, flag);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!gem->is_controller_ready(gem_num))
	{
		return CELL_GEM_NOT_CONNECTED;
	}

	gem->controllers[gem_num].enabled_magnetometer = !!flag;

	return CELL_OK;
}

error_code cellGemEnableMagnetometer2()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

error_code cellGemEnd()
{
	cellGem.warning("cellGemEnd()");

	const auto controller = fxm::make<gem_config::gem_controller>();
	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state.compare_and_swap_test(1, 0))
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (g_cfg.io.move == move_handler::move)
	{
		psmoveapi::end(controller.get());
	}
	else if (g_cfg.io.move == move_handler::fake &&
		g_cfg.io.mouse == mouse_handler::basic)
	{
		fxm::remove<MouseHandlerBase>();
	}

	return CELL_OK;
}

error_code cellGemFilterState(u32 gem_num, u32 enable)
{
	cellGem.warning("cellGemFilterState(gem_num=%d, enable=%d)", gem_num, enable);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->controllers[gem_num].enabled_filtering = !!enable;

	return CELL_OK;
}

error_code cellGemForceRGB(u32 gem_num, float r, float g, float b)
{
	cellGem.todo("cellGemForceRGB(gem_num=%d, r=%f, g=%f, b=%f)", gem_num, r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto& controller = gem->controllers[gem_num];
	controller.sphere_rgb = gem_config::gem_color(r, g, b);

	if (g_cfg.io.move == move_handler::move)
	{
		move::psmoveapi::update_color(controller);
	}

	return CELL_OK;
}

error_code cellGemGetAccelerometerPositionInDevice(u32 gem_num, vm::ptr<float> pos)
{
	cellGem.fatal("cellGemGetAccelerometerPositionInDevice(gem_num=%d, pos: +%f)", gem_num, pos);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	/*auto& controller = gem->controllers[gem_num];
	if (g_cfg.io.move == move_handler::move)
	{
		// Todo
	}*/

	return CELL_OK;
}

error_code cellGemGetAllTrackableHues(vm::ptr<u8> hues)
{
	cellGem.todo("cellGemGetAllTrackableHues(hues=*0x%x)");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!hues)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	for (u32 i = 0; i < 360; i++)
	{
		hues[i] = true;
	}

	return CELL_OK;
}

error_code cellGemGetCameraState(vm::ptr<CellGemCameraState> camera_state)
{
	cellGem.todo("cellGemGetCameraState(camera_state=0x%x)", camera_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!camera_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	camera_state->exposure_time = 1.0f / 60.0f; // TODO: use correct framerate
	camera_state->gain = 1.0;

	return CELL_OK;
}

error_code cellGemGetEnvironmentLightingColor(vm::ptr<f32> r, vm::ptr<f32> g, vm::ptr<f32> b)
{
	cellGem.todo("cellGemGetEnvironmentLightingColor(r=*0x%x, g=*0x%x, b=*0x%x)", r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	// default to 128
	*r = 128;
	*g = 128;
	*b = 128;

	return CELL_OK;
}

error_code cellGemGetHuePixels(vm::cptr<void> camera_frame, u32 hue, vm::ptr<u8> pixels)
{
	cellGem.todo("cellGemGetHuePixels(camera_frame=*0x%x, hue=%d, pixels=*0x%x)", camera_frame, hue, pixels);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!camera_frame || !pixels || hue > 359)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

error_code cellGemGetImageState(u32 gem_num, vm::ptr<CellGemImageState> gem_image_state)
{
	cellGem.todo("cellGemGetImageState(gem_num=%d, image_state=&0x%x)", gem_num, gem_image_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !gem_image_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake)
	{
		auto shared_data = g_fxo->get<gem_camera_shared>();

		if (g_cfg.io.mouse == mouse_handler::basic)
		{
			const auto handler = fxm::get<MouseHandlerBase>();
			auto& mouse = handler->GetMice().at(0);

			f32 x_pos = mouse.x_pos;
			f32 y_pos = mouse.y_pos;

			// Only game this seems to work on is PAIN, others use different functions
			static constexpr auto aspect_ratio = 1.2;

			static constexpr auto screen_offset_x = 400.0;
			static constexpr auto screen_offset_y = screen_offset_x * aspect_ratio;

			static constexpr auto screen_scale = 3.0;

			gem_image_state->u = screen_offset_x / screen_scale + x_pos / screen_scale;
			gem_image_state->v = screen_offset_y / screen_scale + y_pos / screen_scale * aspect_ratio;
		}
		else
		{
			gem_image_state->u = 0;
			gem_image_state->v = 0;
		}

		gem_image_state->frame_timestamp = shared_data->frame_timestamp.load();
		gem_image_state->timestamp = gem_image_state->frame_timestamp + 10;
		gem_image_state->r = 10;
		gem_image_state->projectionx = 1;
		gem_image_state->projectiony = 1;
		gem_image_state->distance = 2 * 1000; // 2 meters away from camera
		gem_image_state->visible = true;
		gem_image_state->r_valid = true;
	}
	else if (g_cfg.io.move == move_handler::move)
	{
		const auto tracker = gem->psmove.tracker.get();
		const auto handle = gem->controllers[gem_num].psmove.handle.get();

		if (tracker && handle)
			move::psmoveapi::tracker::map::gem_image_state(gem_image_state, tracker, handle);

		//auto& handle = gem->controllers[gem_num];
		//const auto handle = gem->controllers[gem_num].psmove.handle.get();

		//move::psmoveapi::poll(handle);

		//move::psmoveapi::tracker::map::gem_image_state(handle, gem_image_state);
		//move::psmoveapi::tracker::map::gem_image_state(image_state, tracker, handle);
	}

	return CELL_OK;
}

error_code cellGemGetInertialState(u32 gem_num, u32 state_flag, u64 timestamp, vm::ptr<CellGemInertialState> inertial_state)
{
	cellGem.todo("cellGemGetInertialState(gem_num=%d, state_flag=%d, timestamp=0x%x, inertial_state=0x%x)", gem_num, state_flag, timestamp, inertial_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || state_flag > CELL_GEM_INERTIAL_STATE_FLAG_NEXT || !inertial_state || !gem->is_controller_ready(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	// TODO(velocity): Abstract with gem state func
	if (g_cfg.io.move == move_handler::fake)
	{
		if (g_cfg.io.mouse == mouse_handler::basic)
		{
			move::map::mouse::input_to_pad(gem_num, inertial_state->pad.digitalbuttons, inertial_state->pad.analog_T);
		}
		else
		{
			move::map::ds3_input_to_pad(gem_num, inertial_state->pad.digitalbuttons, inertial_state->pad.analog_T);
		}

		move::map::ds3_input_to_ext(gem_num, inertial_state->ext);
	}
	else if (g_cfg.io.move == move_handler::move)
	{
		auto& handle = gem->controllers[gem_num];
		//PSMove_Ext_Data data;
		move::psmoveapi::poll(handle);

		move::psmoveapi::input_to_pad(handle, inertial_state->pad.digitalbuttons, inertial_state->pad.analog_T);
		move::psmoveapi::sensor_to_inertial(handle, inertial_state);

		if (g_cfg.io.move_ext == psmove_ext::wheel)
		{
			const auto move = handle.psmove.handle.get();
			PSMove_Ext_Data data;
			if (psmove_get_ext_data(move, &data))
			{
				move::psmoveapi::wheel_input_to_ext(handle, &data, inertial_state->ext);
			}
			else
			{
				cellGem.fatal("Don't get ext data");
			}
		}
		else if (g_cfg.io.move_ext == psmove_ext::shooter)
		{
			//move::map::shoot_input_to_ext(handle, inertial_state->ext);
		}
		else 
		{
			move::map::ds3_input_to_ext(gem_num, inertial_state->ext);
		}

	}

	// TODO: handle timestamp arg by storing previous states

	// TODO: should this be in if above?
	inertial_state->timestamp = gem->timer.GetElapsedTimeInMicroSec();

	inertial_state->counter = gem->inertial_counter++;

	return CELL_OK;
}

error_code cellGemGetInfo(vm::ptr<CellGemInfo> info)
{
	cellGem.todo("cellGemGetInfo(info=*0x%x)", info);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!info)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	// TODO: Support connecting PlayStation Move controllers
	info->max_connect = gem->attribute.max_connect;
	info->now_connect = gem->connected_controllers;

	for (int i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		info->status[i] = gem->controllers[i].status;
		info->port[i] = gem->controllers[i].port;
	}

	return CELL_OK;
}

error_code cellGemGetMemorySize(s32 max_connect)
{
	cellGem.warning("cellGemGetMemorySize(max_connect=%d)", max_connect);

	if (max_connect > CELL_GEM_MAX_NUM || max_connect <= 0)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return max_connect <= 2 ? 0x120000 : 0x140000;
}

error_code cellGemGetRGB(u32 gem_num, vm::ptr<float> r, vm::ptr<float> g, vm::ptr<float> b)
{
	cellGem.todo("cellGemGetRGB(gem_num=%d, r=*0x%x, g=*0x%x, b=*0x%x)", gem_num, r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto& sphere_color = gem->controllers[gem_num].sphere_rgb;
	*r = sphere_color.r;
	*g = sphere_color.g;
	*b = sphere_color.b;

	return CELL_OK;
}

error_code cellGemGetRumble(u32 gem_num, vm::ptr<u8> rumble)
{
	cellGem.todo("cellGemGetRumble(gem_num=%d, rumble=*0x%x)", gem_num, rumble);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !rumble)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	*rumble = gem->controllers[gem_num].rumble;

	return CELL_OK;
}

error_code cellGemGetState(u32 gem_num, u32 flag, u64 time_parameter, vm::ptr<CellGemState> gem_state)
{
	cellGem.todo("cellGemGetState(gem_num=%d, flag=0x%x, time=0x%llx, gem_state=*0x%x)", gem_num, flag, time_parameter, gem_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || flag > CELL_GEM_STATE_FLAG_TIMESTAMP || !gem_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake)
	{
		if (g_cfg.io.mouse == mouse_handler::basic)
		{
			move::map::mouse::input_to_pad(gem_num, gem_state->pad.digitalbuttons, gem_state->pad.analog_T);
			move::map::mouse::input_to_gem(gem_num, gem_state);
		}
		else
		{
			move::map::ds3_input_to_pad(gem_num, gem_state->pad.digitalbuttons, gem_state->pad.analog_T);
		}

		move::map::ds3_input_to_ext(gem_num, gem_state->ext);
		gem_state->quat[3] = 1.0;
	}
	else if (g_cfg.io.move == move_handler::move)
	{
		auto& handle = gem->controllers[gem_num];

		move::psmoveapi::poll(handle);

		move::psmoveapi::input_to_pad(handle, gem_state->pad.digitalbuttons, gem_state->pad.analog_T);
		move::psmoveapi::sensor_to_gem(handle, gem_state);
		move::psmoveapi::tracker::map::gem_state(handle, gem_state);

		if (g_cfg.io.move_ext == psmove_ext::wheel)
		{
			PSMove_Ext_Data data;
			auto move = handle.psmove.handle.get();

			if (psmove_get_ext_data(move, &data))
			{
				move::psmoveapi::wheel_input_to_ext(handle, &data, gem_state->ext);
			}			
		}
		else if (g_cfg.io.move_ext == psmove_ext::shooter)
		{
			//move::map::shoot_input_to_ext(handle, inertial_state->ext);
		}
		else
		{
			move::map::ds3_input_to_ext(gem_num, gem_state->ext);
		}
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::move)
	{
		gem_state->tracking_flags = CELL_GEM_TRACKING_FLAG_POSITION_TRACKED | CELL_GEM_TRACKING_FLAG_VISIBLE;
		gem_state->timestamp = gem->timer.GetElapsedTimeInMicroSec();

		return CELL_OK;
	}

	return CELL_GEM_NOT_CONNECTED;
}

error_code cellGemGetStatusFlags(u32 gem_num, vm::ptr<u64> flags)
{
	cellGem.todo("cellGemGetStatusFlags(gem_num=%d, flags=*0x%x)", gem_num, flags);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !flags)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	*flags = gem->status_flags;
	//cellGem.fatal("status_flags: flags=*0x%x)", flags);

	return CELL_OK;
}

error_code cellGemGetTrackerHue(u32 gem_num, vm::ptr<u32> hue)
{
	cellGem.warning("cellGemGetTrackerHue(gem_num=%d, hue=*0x%x)", gem_num, hue);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !hue)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (!gem->controllers[gem_num].enabled_tracking || gem->controllers[gem_num].hue > 359)
	{
		return CELL_GEM_ERROR_NOT_A_HUE;
	}

	*hue = gem->controllers[gem_num].hue;

	return CELL_OK;
}

error_code cellGemHSVtoRGB(f32 h, f32 s, f32 v, vm::ptr<f32> r, vm::ptr<f32> g, vm::ptr<f32> b)
{
	cellGem.todo("cellGemHSVtoRGB(h=%f, s=%f, v=%f, r=*0x%x, g=*0x%x, b=*0x%x)", h, s, v, r, g, b);

	if (s < 0.0f || s > 1.0f || v < 0.0f || v > 1.0f || !r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	h = std::clamp(h, 0.0f, 360.0f);

	// TODO: convert

	return CELL_OK;
}

error_code cellGemInit(vm::cptr<CellGemAttribute> attribute)
{
	cellGem.fatal("cellGemInit(attribute=*0x%x)", attribute);

	const auto gem = g_fxo->get<gem_config>();

	if (!attribute || !attribute->spurs_addr || attribute->max_connect > CELL_GEM_MAX_NUM)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (!gem->state.compare_and_swap_test(0, 1))
	{
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;
	}

	gem->attribute = *attribute;

	if (g_cfg.io.move == move_handler::fake &&
		g_cfg.io.mouse == mouse_handler::basic)
	{
		// init mouse handler
		const auto handler = fxm::import<MouseHandlerBase>(Emu.GetCallbacks().get_mouse_handler);
		handler->Init(std::min(attribute->max_connect.value(), static_cast<u32>(CELL_GEM_MAX_NUM)));
	}
	else if (g_cfg.io.move == move_handler::move)
	{
		// Initialize psmoveapi
		move::psmoveapi::init(gem);

		if (g_cfg.io.force_init_tracker)
		{
			//move::psmoveapi::tracker::init(gem.get());
		}
	}

	for (auto gem_num = 0; gem_num < CELL_GEM_MAX_NUM; gem_num++)
	{
		gem->reset_controller(gem, gem_num);
	}

	// TODO: is this correct?
	gem->timer.Start();

	return CELL_OK;
}

error_code cellGemInvalidateCalibration(s32 gem_num)
{
	cellGem.todo("cellGemInvalidateCalibration(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake)
	{
		gem->controllers[gem_num].calibrated_magnetometer = false;
		// TODO: gem->status_flags
	}

	if (g_cfg.io.move == move_handler::move)
	{
		move::psmoveapi::tracker::disable(gem, gem_num);
	}

	return CELL_OK;
}

error_code cellGemIsTrackableHue(u32 hue)
{
	cellGem.todo("cellGemIsTrackableHue(hue=%d)", hue);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state || hue > 359)
	{
		return false;
	}

	return true;
}

error_code cellGemPrepareCamera(s32 max_exposure, f32 image_quality)
{
	cellGem.fatal("cellGemPrepareCamera(max_exposure=%d, image_quality=%f)", max_exposure, image_quality);

	const auto gem = g_fxo->get<gem_config>();
	u32 gem_num;

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	max_exposure = std::clamp(max_exposure, static_cast<s32>(CELL_GEM_MIN_CAMERA_EXPOSURE), static_cast<s32>(CELL_GEM_MAX_CAMERA_EXPOSURE));
	image_quality = std::clamp(image_quality, 0.0f, 1.0f);

	// TODO: prepare camera

	if (g_cfg.io.camera == camera_handler::fake &&
		g_cfg.io.move == move_handler::move)
	{
		move::psmoveapi::tracker::init(gem);

		if (g_cfg.io.force_reset_tracker)
		{
			move::psmoveapi::tracker::force_enable(gem, gem_num);
		}
	}
	else if (g_cfg.io.camera == camera_handler::pseye &&
		g_cfg.io.move == move_handler::move)
	{

	}

	return CELL_OK;
}

error_code cellGemPrepareVideoConvert(vm::cptr<CellGemVideoConvertAttribute> vc_attribute)
{
	cellGem.todo("cellGemPrepareVideoConvert(vc_attribute=*0x%x)", vc_attribute);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!vc_attribute)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	const auto vc = *vc_attribute;

	if (!vc_attribute || vc.version == 0 || vc.output_format == 0 ||
		(vc.conversion_flags & CELL_GEM_COMBINE_PREVIOUS_INPUT_FRAME && !vc.buffer_memory))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (vc.video_data_out & 0x1f || vc.buffer_memory & 0xff)
	{
		return CELL_GEM_ERROR_INVALID_ALIGNMENT;
	}

	gem->vc_attribute = vc;

	return CELL_OK;
}

error_code cellGemReadExternalPortDeviceInfo(u32 gem_num, vm::ptr<u32> ext_id, vm::ptr<u8[CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE]> ext_info)
{
	cellGem.fatal("cellGemReadExternalPortDeviceInfo(gem_num=%d, ext_id=*0x%x, ext_info=%s)", gem_num, ext_id, ext_info);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		cellGem.fatal("Gem Ext error");
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !ext_id)
	{
		cellGem.fatal("check gem Ext error");
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (gem->controllers[gem_num].status & CELL_GEM_STATUS_DISCONNECTED)
	{
		cellGem.fatal("gem disconnected!");
		return CELL_GEM_NOT_CONNECTED;
	}

	if (!(gem->controllers[gem_num].ext_status & CELL_GEM_EXT_CONNECTED))
	{
		cellGem.fatal("Gem ext no device connected!");
		return CELL_GEM_NO_EXTERNAL_PORT_DEVICE;
	}

	auto& handle = gem->controllers[gem_num];
	if (g_cfg.io.move == move_handler::move && (!(g_cfg.io.move_ext == psmove_ext::null)))
	{
		move::psmoveapi::get_ext_info(handle, ext_id, ext_info);
	}

	return CELL_OK;
}

error_code cellGemReset(u32 gem_num)
{
	cellGem.fatal("cellGemReset(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->reset_controller(gem, gem_num);

	auto& controller = gem->controllers[gem_num];
	if (g_cfg.io.move == move_handler::move)
	{
		if (g_cfg.io.force_reset_tracker)
		{ 
			move::psmoveapi::tracker::force_enable(gem, gem_num);
		}
	}

	// TODO: is this correct?
	gem->timer.Start();

	return CELL_OK;
}

error_code cellGemSetRumble(u32 gem_num, u8 rumble)
{
	cellGem.todo("cellGemSetRumble(gem_num=%d, rumble=0x%x)", gem_num, rumble);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto& controller = gem->controllers[gem_num];
	controller.rumble = rumble;

	if (g_cfg.io.move == move_handler::move)
	{
		move::psmoveapi::update_rumble(controller);
	}

	return CELL_OK;
}

error_code cellGemSetYaw(u32 gem_num, f32 z_direction)
{
	cellGem.fatal("cellGemSetYaw(gem_num=gem_num=%d, z_direction=%+01.3f)", gem_num, z_direction);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		cellGem.fatal("SetYaw, gem_num uninitialized");
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!z_direction)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto& controller = gem->controllers[gem_num];
	auto handle = controller.psmove.handle.get();


	if (g_cfg.io.move == move_handler::move)
	{
		float w, x, y, z;
		psmove_get_orientation(handle, &w, &x, &y, &z);
		z_direction = z;
		//move::psmoveapi::reset_orientation(handle);
	}

	return CELL_OK;
}

error_code cellGemTrackHues(vm::cptr<u32> req_hues, vm::ptr<u32> res_hues)
{
	cellGem.todo("cellGemTrackHues(req_hues=*0x%x, res_hues=*0x%x)", req_hues, res_hues);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!req_hues)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	for (u32 i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		if (req_hues[i] == CELL_GEM_DONT_CARE_HUE)
		{

		}
		else if (req_hues[i] == CELL_GEM_DONT_TRACK_HUE)
		{
			gem->controllers[i].enabled_tracking = false;
			gem->controllers[i].enabled_LED = false;
		}
		else
		{
			if (req_hues[i] > 359)
			{
				cellGem.warning("cellGemTrackHues: req_hues[%d]=%d -> this can lead to unexpected behavior", i, req_hues[i]);
			}
		}
	}

	return CELL_OK;
}

error_code cellGemUpdateFinish()
{
	cellGem.todo("cellGemUpdateFinish()");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (g_cfg.io.move == move_handler::move)
	{
		move::psmoveapi::tracker::update(gem);
	}

	return CELL_OK;
}

error_code cellGemUpdateStart(vm::cptr<void> camera_frame, u64 timestamp)
{
	cellGem.todo("cellGemUpdateStart(camera_frame=*0x%x, timestamp=%d)", camera_frame, timestamp);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!camera_frame)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

error_code cellGemWriteExternalPort(u32 gem_num, vm::ptr<u8[CELL_GEM_EXTERNAL_PORT_OUTPUT_SIZE]> data)
{
	cellGem.todo("cellGemWriteExternalPort(gem_num=%d, data=%s)", gem_num, data);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

s32 libgem_0D03DBDC()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 libgem_0E61785C()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 libgem_155CE38B()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 libgem_615698B8()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 libgem_633C3196()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 libgem_D7B1837D()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellGem)("libgem", []()
{
	REG_FUNC(libgem, cellGemCalibrate);
	REG_FUNC(libgem, cellGemClearStatusFlags);
	REG_FUNC(libgem, cellGemConvertVideoFinish);
	REG_FUNC(libgem, cellGemConvertVideoStart);
	REG_FUNC(libgem, cellGemEnableCameraPitchAngleCorrection);
	REG_FUNC(libgem, cellGemEnableMagnetometer);
	REG_FUNC(libgem, cellGemEnableMagnetometer2);
	REG_FUNC(libgem, cellGemEnd);
	REG_FUNC(libgem, cellGemFilterState);
	REG_FUNC(libgem, cellGemForceRGB);
	REG_FUNC(libgem, cellGemGetAccelerometerPositionInDevice);
	REG_FUNC(libgem, cellGemGetAllTrackableHues);
	REG_FUNC(libgem, cellGemGetCameraState);
	REG_FUNC(libgem, cellGemGetEnvironmentLightingColor);
	REG_FUNC(libgem, cellGemGetHuePixels);
	REG_FUNC(libgem, cellGemGetImageState);
	REG_FUNC(libgem, cellGemGetInertialState);
	REG_FUNC(libgem, cellGemGetInfo);
	REG_FUNC(libgem, cellGemGetMemorySize);
	REG_FUNC(libgem, cellGemGetRGB);
	REG_FUNC(libgem, cellGemGetRumble);
	REG_FUNC(libgem, cellGemGetState);
	REG_FUNC(libgem, cellGemGetStatusFlags);
	REG_FUNC(libgem, cellGemGetTrackerHue);
	REG_FUNC(libgem, cellGemHSVtoRGB);
	REG_FUNC(libgem, cellGemInit);
	REG_FUNC(libgem, cellGemInvalidateCalibration);
	REG_FUNC(libgem, cellGemIsTrackableHue);
	REG_FUNC(libgem, cellGemPrepareCamera);
	REG_FUNC(libgem, cellGemPrepareVideoConvert);
	REG_FUNC(libgem, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(libgem, cellGemReset);
	REG_FUNC(libgem, cellGemSetRumble);
	REG_FUNC(libgem, cellGemSetYaw);
	REG_FUNC(libgem, cellGemTrackHues);
	REG_FUNC(libgem, cellGemUpdateFinish);
	REG_FUNC(libgem, cellGemUpdateStart);
	REG_FUNC(libgem, cellGemWriteExternalPort);

	// Need found real name
	REG_FNID(libgem, 0x0D03DBDC, libgem_0D03DBDC);
	REG_FNID(libgem, 0x0E61785C, libgem_0E61785C);
	REG_FNID(libgem, 0x155CE38B, libgem_155CE38B);
	REG_FNID(libgem, 0x615698B8, libgem_615698B8);
	REG_FNID(libgem, 0x633C3196, libgem_633C3196);
	REG_FNID(libgem, 0xD7B1837D, libgem_D7B1837D);
});
