#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_bluetooth.h"

logs::channel sys_bluetooth("sys_bluetooth");

error_code sys_bluetooth_aud_serial_unk1()
{
	sys_bluetooth.fatal("sys_bluetooth_aud_serial_unk1");

	return CELL_OK;
}

error_code sys_bt_read_firmware_version()
{
	sys_bluetooth.fatal("sys_bt_read_firmware_version");

	return CELL_OK;
}

error_code sys_bt_complete_wake_on_host()
{
	sys_bluetooth.fatal("sys_bt_complete_wake_on_host");

	return CELL_OK;
}

error_code sys_bt_disable_bluetooth()
{
	sys_bluetooth.fatal("sys_bt_disable_bluetooth");

	return CELL_OK;
}


error_code sys_bt_enable_bluetooth()
{
	sys_bluetooth.fatal("sys_bt_enable_bluetooth");

	return CELL_OK;
}

error_code sys_bt_bccmd()
{
	sys_bluetooth.fatal("sys_bt_bccmd");

	return CELL_OK;
}

error_code sys_bt_read_hq()
{
	sys_bluetooth.fatal("sys_bt_read_hq");

	return CELL_OK;
}

error_code sys_bt_hid_get_remote_status()
{
	sys_bluetooth.fatal("sys_bt_hid_get_remote_status");

	return CELL_OK;
}
error_code sys_bt_register_controller()
{
	sys_bluetooth.fatal("sys_bt_register_controller");

	return CELL_OK;
}
error_code sys_bt_clear_registered_contoller()
{
	sys_bluetooth.fatal("sys_bt_clear_registered_contoller");

	return CELL_OK;
}

error_code sys_bt_connect_accept_controller()
{
	sys_bluetooth.fatal("sys_bt_connect_accept_controller");

	return CELL_OK;
}

error_code sys_bt_get_local_bdaddress()
{
	sys_bluetooth.fatal("sys_bt_get_local_bdaddress");

	return CELL_OK;
}

error_code sys_bt_hid_get_data()
{
	sys_bluetooth.fatal("sys_bt_hid_get_data");
	return CELL_OK;
}

error_code sys_bt_hid_set_report()
{
	sys_bluetooth.fatal("sys_bt_hid_set_report");

	return CELL_OK;
}

error_code sys_bt_sched_log()
{
	sys_bluetooth.fatal("sys_bt_sched_log");

	return CELL_OK;
}

error_code sys_bt_cancel_connect_accept_controller()
{
	sys_bluetooth.fatal("sys_bt_cancel_connect_accept_controller");

	return CELL_OK;
}
