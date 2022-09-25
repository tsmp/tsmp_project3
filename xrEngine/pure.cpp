#include "pch_xrengine.h"

#define DECLARE_RP(name) void __fastcall rp_##name(void *p) { ((pure##name *)p)->On##name(); }

DECLARE_RP(Frame);
DECLARE_RP(Render);
DECLARE_RP(AppActivate);
DECLARE_RP(AppDeactivate);
DECLARE_RP(AppStart);
DECLARE_RP(AppEnd);
DECLARE_RP(DeviceReset);
