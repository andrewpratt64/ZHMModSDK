#pragma once
// Andrew Pratt 2021
// TestMod

#include<string>
#include<filesystem>

#include "json.hpp"

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZActor.h"

namespace fs = std::filesystem;

class TestMod : public IPluginInterface
{

public:
	TestMod();
	~TestMod() override;

	void PreInit() override;
	void OnEngineInitialized() override;
	void OnDraw3D(IRenderer* p_Renderer) override;

protected:
	ZActor* m_closestNpc;

	bool m_bRenderUI;
	bool m_bNoLimitEntNames;
	nlohmann::json m_coordJson;

private:
	//bool m_bIgnoreNextAchOnEventSent;

private:
	//void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

	DEFINE_PLUGIN_DETOUR(TestMod, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
	DEFINE_PLUGIN_DETOUR(TestMod, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
	//DEFINE_PLUGIN_DETOUR(TestMod, bool, GetOption, const ZString&, bool);
	//DEFINE_PLUGIN_DETOUR(TestMod, void, ZActor_ZActor, ZActor*, ZComponentCreateInfo*);
	//DEFINE_PLUGIN_DETOUR(TestMod, bool, SignalInputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	//DEFINE_PLUGIN_DETOUR(TestMod, bool, SignalOutputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	//DEFINE_PLUGIN_DETOUR(TestMod, void, ZApplicationEngineWin32_OnDebugInfo, ZApplicationEngineWin32*, const ZString&, const ZString&);
	//DEFINE_PLUGIN_DETOUR(TestMod, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event);
};

DEFINE_ZHM_PLUGIN(TestMod)
