#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSsc);

s32 CellSscAttr()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscCb()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscCfgParam()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscClose()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscCtrlParam()
{
	cellSsc.fatal("CellSscCtrlParam UNIMPLEMENTED");
	return CELL_OK;
}

s32 CellSscGpuAttr()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscGpuClose()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscGpuHandle()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscGpuOpen()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscGpuQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscGpuResource()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscHandle()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscOpen()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 cellSscQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

s32 CellSscResource()
{
	UNIMPLEMENTED_FUNC(cellSsc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSsc)("cellSsc", []()
{
	REG_FUNC(cellSsc, CellSscAttr);
	REG_FUNC(cellSsc, CellSscCb);
	REG_FUNC(cellSsc, CellSscCfgParam);
	REG_FUNC(cellSsc, cellSscClose);
	REG_FUNC(cellSsc, CellSscCtrlParam);
	REG_FUNC(cellSsc, CellSscGpuAttr);
	REG_FUNC(cellSsc, cellSscGpuClose);
	REG_FUNC(cellSsc, CellSscGpuHandle);
	REG_FUNC(cellSsc, cellSscGpuOpen);
	REG_FUNC(cellSsc, cellSscGpuQueryAttr);
	REG_FUNC(cellSsc, CellSscGpuResource);
	REG_FUNC(cellSsc, CellSscHandle);
	REG_FUNC(cellSsc, cellSscOpen);
	REG_FUNC(cellSsc, cellSscQueryAttr);
	REG_FUNC(cellSsc, CellSscResource);
});
