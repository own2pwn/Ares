/* Ares-Setup */
#include "../Ares.h"

/* Factories */
CreateInterfaceFn ClientFactoryFn = NULL;
CreateInterfaceFn EngineFactoryFn = NULL;
CreateInterfaceFn MatSurfaceFactoryFn = NULL;
CreateInterfaceFn VGUI2FactoryFn = NULL;

/* CreateMove */
typedef bool (*CreateMoveFn)(void*, float, CUserCmd*); // Function prototype
CreateMoveFn oCreateMove;
bool hkCreateMove(void* thisptr, float flInputSampleTime, CUserCmd* Cmd) {

	g_pClientModeHook->GetOriginalFunction<CreateMoveFn>(25)(thisptr, flInputSampleTime, Cmd); /* Call the original ClientMode::CreateMove th_off
	ensure csgo runs fine */

	if (!Cmd->command_number) return false;

	auto Local = (CBaseEntity*)g_pIEntList->GetClientEntity(g_pEngineClient->GetLocalPlayer());

	// Example auto hop.
	if (Cmd->buttons & IN_JUMP && (!(Local->GetFlags() & (1 << 0)))) {
		Cmd->buttons &= ~IN_JUMP;
	}

    return false;
}

/* PaintTraverse */
typedef void(*PaintTraverseFn)(void*, unsigned long long, bool, bool);
PaintTraverseFn oPaintTraverse;
void hkPaintTraverse(void* thisptr, unsigned long long vguiPanel, bool forceRepaint, bool allowForce) {

	g_pPanelHook->GetOriginalFunction<PaintTraverseFn>(42)(thisptr, vguiPanel, forceRepaint, allowForce);

	if (vguiPanel == g_pEngineVGui->GetPanel(PANEL_ROOT)) {
		// Render:
	}

}

/* Paint */
typedef void(*PaintFn)(IEngineVGui*, PaintMode_t);
PaintFn oPaint;
typedef void(*StartDrawingFn)(ISurface*);
typedef void(*FinishDrawingFn)(ISurface*);
extern StartDrawingFn StartDrawing;
extern FinishDrawingFn FinishDrawing;
void hkPaint(IEngineVGui* thisptr, PaintMode_t mode) { 

	/* Call the original to make sure cs go runs properly */
	g_pVGuiHook->GetOriginalFunction<PaintFn>(14)(thisptr, mode);	

	if (mode & PaintMode_t::PAINT_UIPANELS) {
	}

}

/* SetupSurfaceDrawing */
void Setup::SetupSurfaceDrawing() {
	uintptr_t start_func_address = Pattern::FindPattern(Pattern::GetLibraryAddress("vguimatsurface_client.so"), 0xFFFFFFFFF, (unsigned char*)"\x55\x48\x89\xE5\x53\x48\x89\xFB\x48\x83\xEC\x28\x80", "xxxxxxxxxxxxx");	
	StartDrawing = reinterpret_cast<StartDrawingFn>(start_func_address);

	uintptr_t finish_func_address = Pattern::FindPattern(Pattern::GetLibraryAddress("vguimatsurface_client.so"), 0xFFFFFFFFF, (unsigned char*)"\x55\x31\xFF\x48\x89", "xxxxx");
	FinishDrawing = reinterpret_cast<FinishDrawingFn>(finish_func_address);
}

/* SetupInterfaces */
void Setup::SetupInterfaces() {

    if (g_pClient == nullptr) { // Prevent repeat calling

        // Client:
        ClientFactoryFn = (CreateInterfaceFn)dlsym(dlopen("./csgo/bin/linux64/client_client.so", RTLD_NOW), "CreateInterface");
        g_pClient = (IBaseClientDll*)ClientFactoryFn("VClient018", NULL);
        g_pIEntList = (IClientEntityList*)ClientFactoryFn("VClientEntityList003", NULL);

        // Engine:
        EngineFactoryFn = (CreateInterfaceFn)dlsym(dlopen("./bin/linux64/engine_client.so", RTLD_NOW), "CreateInterface");
        g_pEngineClient = (IVEngineClient*)EngineFactoryFn("VEngineClient014", NULL);
	g_pEngineVGui = (IEngineVGui*)EngineFactoryFn("VEngineVGui001", NULL);

	// VGui:
        MatSurfaceFactoryFn = (CreateInterfaceFn)dlsym(dlopen("./bin/linux64/vguimatsurface_client.so", RTLD_NOW), "CreateInterface");		
        VGUI2FactoryFn = (CreateInterfaceFn)dlsym(dlopen("./bin/linux64/vgui2_client.so", RTLD_NOW), "CreateInterface");
	g_pSurface = (ISurface*)MatSurfaceFactoryFn("VGUI_Surface031", NULL);
	g_pPanel = (IVPanel*)VGUI2FactoryFn("VGUI_Panel009", NULL);

    }

}

/* SetupHooks */
void Setup::SetupHooks() {

	uintptr_t client_client = 0;

	dl_iterate_phdr([] (struct dl_phdr_info* info, size_t size, void* data) {
		if (strcasestr(info->dlpi_name, "client_client.so")) {
			*reinterpret_cast<uintptr_t*>(data) = info->dlpi_addr + info->dlpi_phdr[0].p_vaddr;
			return 1;
		}

		return 0;
	}, &client_client);

    	uintptr_t init_address = Pattern::FindPattern(client_client, 0xFFFFFFFFF, (unsigned char*)"\x48\x8D\x05\x00\x00\x00\x00\x48\x89\xE5\x48\x89\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5D\x48", "xxx????xxxxxx????x????xx");

	if (!init_address)
		return;   

    	g_pClientMode = reinterpret_cast<IClientMode*>(GetAbsoluteAddress(init_address, 3, 7)); /* Grab
	clientmode */

	g_pClientModeHook = new CVMT(g_pClientMode);
	g_pPanelHook = new CVMT(g_pPanel);
	g_pVGuiHook = new CVMT(g_pEngineVGui);

	oCreateMove = (CreateMoveFn)g_pClientModeHook->HookFunction((void*)hkCreateMove, 25);

}
