#pragma once


struct bt_set_device_info {
	// todo
};

// syscalls
error_code sys_bluetooth_aud_serial_unk1();
error_code sys_bt_read_firmware_version();
error_code sys_bt_complete_wake_on_host();
error_code sys_bt_disable_bluetooth();
error_code sys_bt_enable_bluetooth();
error_code sys_bt_bccmd();
error_code sys_bt_read_hq();
error_code sys_bt_hid_get_remote_status();
error_code sys_bt_register_controller();
error_code sys_bt_clear_registered_contoller();
error_code sys_bt_connect_accept_controller();
error_code sys_bt_get_local_bdaddress();
error_code sys_bt_hid_get_data();
error_code sys_bt_hid_set_report();
error_code sys_bt_sched_log();
error_code sys_bt_cancel_connect_accept_controller();
