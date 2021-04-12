#include "TestMod.h"

#include<string>
#include<string_view>
#include<fstream>
#include<vector>
#include<algorithm>
#include<regex>

#include "json.hpp"

#include "Events.h"
#include "Functions.h"
#include "Logging.h"
#include "Pins.h"

#include <Glacier/ZActor.h>
#include <Glacier/SGameUpdateEvent.h>
#include "Glacier/ZString.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZObject.h"


TestMod::TestMod() :
	m_closestNpc{nullptr},
	m_bRenderUI{false},
	m_bNoLimitEntNames{false}
	//m_bIgnoreNextAchOnEventSent{false}
{
	std::ifstream s_iCoordJson("D:/EpicGames/HITMAN3/Retail/ent_coords.json");
	if (s_iCoordJson.good())
		m_coordJson = nlohmann::json::parse(s_iCoordJson);
	s_iCoordJson.close();
}

TestMod::~TestMod()
{
	/*const ZMemberDelegate<TestMod, void(const SGameUpdateEvent&)> s_Delegate(this, &TestMod::OnFrameUpdate);
	Hooks::ZGameLoopManager_UnregisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);*/
}


void TestMod::PreInit()
{
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &TestMod::OnLoadScene);
	Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &TestMod::WndProc);
	//Hooks::GetApplicationOptionBool->AddDetour(this, &TestMod::GetOption);
	//Hooks::ZActor_ZActor->AddDetour(this, &TestMod::ZActor_ZActor);
	//Hooks::SignalInputPin->AddDetour(this, &TestMod::SignalInputPin);
	//Hooks::SignalInputPin->AddDetour(this, &TestMod::SignalOutputPin);
	//Hooks::ZApplicationEngineWin32_OnDebugInfo->AddDetour(this, &TestMod::ZApplicationEngineWin32_OnDebugInfo);
	//Hooks::ZAchievementManagerSimple_OnEventSent->AddDetour(this, &TestMod::ZAchievementManagerSimple_OnEventSent);
}


void TestMod::OnEngineInitialized()
{
	/*const ZMemberDelegate<TestMod, void(const SGameUpdateEvent&)> s_Delegate(this, &TestMod::OnFrameUpdate);
	Hooks::ZGameLoopManager_RegisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);*/
}


void TestMod::OnDraw3D(IRenderer* p_Renderer)
{
	if (!p_Renderer || !m_bRenderUI) return;

	try
	{
		p_Renderer->DrawText2D(
			"DEBUG ON",
			SVector2(0.0f, 0.0f),
			SVector4(1.0f, 1.0f, 1.0f, 1.0f),
			0.0f,
			1.0f,
			TextAlignment::Left
		);

		auto* s_Cam = Functions::GetCurrentCamera->Call();
		ZEntityRef s_CamRef;
		s_Cam->GetID(&s_CamRef);

		auto* s_CamSpatial = s_CamRef.QueryInterface<ZSpatialEntity>();
		SMatrix s_CamT;
		Functions::ZSpatialEntity_WorldTransform->Call(s_CamSpatial, &s_CamT);
		SVector3 s_CamWorldPos = SVector3(
			s_CamT.mat[3].x,
			s_CamT.mat[3].y,
			s_CamT.mat[3].z
		);


		int s_ChunkPos[3] = {
			32 * static_cast<int>(s_CamWorldPos.x * 0.03125),
			32 * static_cast<int>(s_CamWorldPos.y * 0.03125),
			32 * static_cast<int>(s_CamWorldPos.z * 0.03125)
		};
		SVector3 s_ChunkWorldPos = SVector3(
			s_ChunkPos[0],
			s_ChunkPos[1],
			s_ChunkPos[2]
		);

		p_Renderer->DrawBox3D(
			s_ChunkWorldPos,
			s_ChunkWorldPos + SVector3(-32.0f, -32.0f, 32.0f),
			SVector4(0.0f, 1.0f, 0.0f, 1.0f)
		);

		nlohmann::json s_ents;
		try
		{
			s_ents = m_coordJson
				.at(std::to_string(s_ChunkPos[0]))
				.at(std::to_string(s_ChunkPos[1]))
				.at(std::to_string(s_ChunkPos[2]));
		}
		catch (nlohmann::json::out_of_range& e)
		{
			s_ents = NULL;
		}

		if (s_ents != NULL && s_ents.is_array())
		{
			// Draw # of ents in chunk
			p_Renderer->DrawText2D(
				ZString(std::to_string(s_ents.size())),
				SVector2(0.0f, 1.0f),
				SVector4(0.0f, 1.0f, 1.0f, 1.0f),
				0.0f,
				1.0f,
				TextAlignment::Right
			);

			// Draw chunk coords
			p_Renderer->DrawText2D(
				ZString(
					fmt::format(
						"Chunk {}, {}, {}",
						s_ChunkPos[0],
						s_ChunkPos[1],
						s_ChunkPos[2]
					)
				),
				SVector2(0.0f, 1.0f),
				SVector4(0.3f, 0.7f, 1.0f, 1.0f),
				0.0f,
				0.0f,
				TextAlignment::Center
			);

			// TODO: Why do I have to negate the x and y axis on everything?
			for (const auto& s_ent : s_ents)
			{
				SVector3 s_entPos = SVector3(s_ent.at("x"), s_ent.at("y"), s_ent.at("z"));

				// Draw ent pos
				p_Renderer->DrawLine3D(
					SVector3(s_entPos - SVector3(0.0f, 0.0f, 0.5f)),
					SVector3(s_entPos + SVector3(0.0f, 0.0f, 0.5f)),
					SVector4(1.0f, 0.0f, 0.0f, 1.0f),
					SVector4(0.0f, 0.0f, 1.0f, 1.0f)
				);

				// Draw ent size
				try
				{
					p_Renderer->DrawBox3D(
						s_entPos,
						s_entPos + SVector3(-s_ent.at("dx").get<float>(), -s_ent.at("dy").get<float>(), s_ent.at("dz")),
						SVector4(1.0f, 1.0f, 0.0f, 1.0f)
					);
				}
				catch (nlohmann::json::out_of_range& e) {}

				// Draw ent name
				if (m_bNoLimitEntNames || s_CamWorldPos.DistSqrdTo(s_entPos) <= 64.0f)
				{
					SVector2 s_entTxtPos;
					if (p_Renderer->WorldToScreen(s_entPos, s_entTxtPos))
						p_Renderer->DrawText2D(
							ZString(
								fmt::format(
									"({}){}",
									s_ent.value("index", "?"),
									s_ent.value("name", "<entity>")
								)
							),
							s_entTxtPos,
							SVector4(1.0, 0.0, 0.3, 1.0)
						);
				}
			}
		}
	}
	catch (...) {}
}


/*void TestMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	ZActor* s_Actor = Globals::ActorManager->GetActorByName("Edward \"Ted\" Mendez");

	if (s_Actor != nullptr)
	{
		ZEntityRef s_ActorRef;
		s_Actor->GetID(&s_ActorRef);
		auto* s_ActorSpatial = s_ActorRef.QueryInterface<ZSpatialEntity>();
		SMatrix s_ActorT;
		Functions::ZSpatialEntity_WorldTransform->Call(s_ActorSpatial, &s_ActorT);


		auto* s_Cam = Functions::GetCurrentCamera->Call();
		ZEntityRef s_CamRef;
		s_Cam->GetID(&s_CamRef);

		auto* s_CamSpatial = s_CamRef.QueryInterface<ZSpatialEntity>();
		SMatrix s_CamT;
		Functions::ZSpatialEntity_WorldTransform->Call(s_CamSpatial, &s_CamT);

		SMatrix& s_MatchT = s_ActorT;
		SMatrix43 s_NewT;
		s_NewT.XAxis = SVector3(s_MatchT.mat[0].x, s_MatchT.mat[0].y, s_MatchT.mat[0].z);
		s_NewT.YAxis = SVector3(s_MatchT.mat[1].x, s_MatchT.mat[1].y, s_MatchT.mat[1].z);
		s_NewT.ZAxis = SVector3(s_MatchT.mat[2].x, s_MatchT.mat[2].y, s_MatchT.mat[2].z);
		s_NewT.Trans = SVector3(s_MatchT.mat[3].x, s_MatchT.mat[3].y, s_MatchT.mat[3].z);

		ZEntityRef s_CamSpatialRef;
		s_CamSpatial->GetID(&s_CamSpatialRef);
		//s_CamSpatialRef.SetProperty<SMatrix43>("m_mTransform", s_NewT);
	}
}*/


DECLARE_PLUGIN_DETOUR(TestMod, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& sceneData)
{
	m_closestNpc = nullptr;

	return HookResult<void>(HookAction::Continue());
}


DECLARE_PLUGIN_DETOUR(TestMod, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam, LPARAM p_Lparam)
{
	if (p_Message == WM_KEYDOWN)
	{
		try
		{
			if (p_Wparam == VK_MULTIPLY)
			{
				m_bRenderUI = !m_bRenderUI;
			}
			else if (p_Wparam == VK_OEM_MINUS)
			{
				m_bNoLimitEntNames = !m_bNoLimitEntNames;
			}

			else if (p_Wparam == VK_NUMPAD7)
			{
				ZActor* s_Actor = Globals::ActorManager->GetActorByName("Edward \"Ted\" Mendez");

				if (s_Actor != nullptr)
				{
					ZEntityRef s_ActorRef;
					s_Actor->GetID(&s_ActorRef);
					auto* s_ActorSpatial = s_ActorRef.QueryInterface<ZSpatialEntity>();
					SMatrix s_ActorT;
					Functions::ZSpatialEntity_WorldTransform->Call(s_ActorSpatial, &s_ActorT);


					auto* s_Cam = Functions::GetCurrentCamera->Call();
					ZEntityRef s_CamRef;
					s_Cam->GetID(&s_CamRef);

					auto* s_CamSpatial = s_CamRef.QueryInterface<ZSpatialEntity>();
					SMatrix s_CamT;
					Functions::ZSpatialEntity_WorldTransform->Call(s_CamSpatial, &s_CamT);

					//s_CamRef.SetProperty("m_mTransform", s_ActorT);
					ZEntityRef s_ActorSpatialRef;
					s_ActorSpatial->GetID(&s_ActorSpatialRef);
					s_ActorSpatialRef.SetProperty("m_mTransform", s_CamT);
					s_ActorRef.SetProperty("m_mTransform", s_CamT);


					/*auto* s_CamSpatial = s_CamRef.QueryInterface<ZSpatialEntity>();
					SMatrix s_CamT;
					Functions::ZSpatialEntity_WorldTransform->Call(s_CamSpatial, &s_CamT);

					ZEntityRef s_CamSpatialRef;
					s_CamSpatial->GetID(&s_CamSpatialRef);
					s_CamSpatialRef.SetProperty<SMatrix>("m_mTransform", s_ActorT);*/
				}
			}
		}
		catch (std::exception& e)
		{
			Logger::Error("Error: {}", e.what());
		}
		catch (...)
		{
			Logger::Error("Unknown error");
		}
	}

	return HookResult<LRESULT>(HookAction::Continue());
}


/*DECLARE_PLUGIN_DETOUR(TestMod, bool, GetOption, const ZString& p_OptionName, bool p_Default)
{
	static std::vector<std::string_view> s_loggedOptions;

	if (std::find(s_loggedOptions.begin(), s_loggedOptions.end(), p_OptionName.ToStringView()) == s_loggedOptions.end())
	{
		std::string_view s_DefaultStr = (p_Default) ? "T" : "F";
		Logger::Info("GetOption:\t\{}, \"{}\"", s_DefaultStr, p_OptionName);
		s_loggedOptions.push_back(p_OptionName);
	}

	return HookResult<bool>(HookAction::Continue());
}*/

/*
DECLARE_PLUGIN_DETOUR(TestMod, void, ZActor_ZActor, ZActor* p_Actor, ZComponentCreateInfo* p_CreateInfo)
{
	static ZActor* s_fooActor = nullptr;
	try
	{
		//Logger::Info("Created: {}\t= {}", p_Actor->m_sActorName, reinterpret_cast<size_t>(p_CreateInfo));

		ZString& s_Name = p_Actor->m_sActorName;

		//if (s_Name == "Esperance Cano Alvarez" && s_fooActor == nullptr)
		//{
		//	ZEntityRef s_ActorRef;
		//	p_Actor->GetID(&s_ActorRef);
		//
		//}
	}
	catch (...) {}

	return HookResult<void>(HookAction::Continue());
}


DECLARE_PLUGIN_DETOUR(TestMod, bool, SignalInputPin, ZEntityRef p_Entity, uint32_t p_PinId, const ZObjectRef& p_Data)
{
	static std::vector<std::string_view> s_loggedNames;

	//if (true || p_PinId == static_cast<uint32_t>(ZHMPin::Kill))
	ZString s_PinName;
	if (!SDK()->GetPinName(p_PinId, s_PinName))
		s_PinName = std::to_string(p_PinId).c_str();

	if (std::find(s_loggedNames.begin(), s_loggedNames.end(), s_PinName.ToStringView()) == s_loggedNames.end())
	{
		s_loggedNames.push_back(s_PinName.ToStringView());
		std::string_view s_IsActorStr = (p_Entity.HasInterface<ZActor>()) ? "IS actor" : "NOT actor";
		Logger::Info("Hit Input pin {} @{}", s_PinName, s_IsActorStr);
	}

	return HookResult<bool>(HookAction::Continue());
}


DECLARE_PLUGIN_DETOUR(TestMod, bool, SignalOutputPin, ZEntityRef p_Entity, uint32_t p_PinId, const ZObjectRef& p_Data)
{
	static std::vector<std::string_view> s_loggedNames;

	//if (true || p_PinId == static_cast<uint32_t>(ZHMPin::Kill))
	ZString s_PinName;
	if (!SDK()->GetPinName(p_PinId, s_PinName))
		s_PinName = std::to_string(p_PinId).c_str();

	if (std::find(s_loggedNames.begin(), s_loggedNames.end(), s_PinName.ToStringView()) == s_loggedNames.end())
	{
		s_loggedNames.push_back(s_PinName.ToStringView());
		Logger::Info("Hit Output pin {} @{}", s_PinName, (*p_Entity.m_pEntity)->m_nEntityId);
	}

	return HookResult<bool>(HookAction::Continue());
}


DECLARE_PLUGIN_DETOUR(TestMod, void, ZApplicationEngineWin32_OnDebugInfo, ZApplicationEngineWin32* th, const ZString& info, const ZString& details)
{
	Logger::Info("DEBUG INFO: {}\n\t{}", info, details);

	return HookResult<void>(HookAction::Continue());

}*/

// TODO: There's gotta be a way where you don't search through a long ass string
/*DECLARE_PLUGIN_DETOUR(TestMod, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventId, const ZDynamicObject& event)
{
	//if (m_bIgnoreNextAchOnEventSent) return HookResult<void>(HookAction::Continue());

	try
	{
		ZString s_EventData;
		Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&event), &s_EventData);
		Logger::Info("s_EventData _PRE={}", s_EventData);

		// Cast to std::string so regex can be used
		auto s_EventDataStr = static_cast<std::string>(s_EventData.ToStringView());

		if (event.Is< TArray<SDynamicObjectKeyValuePair> >())
		{
			auto* s_EventArray = event.As< TArray<SDynamicObjectKeyValuePair> >();

			SDynamicObjectKeyValuePair* s_EventName = nullptr;
			SDynamicObjectKeyValuePair* s_EventValue = nullptr;

			//Logger::Info("Keys for event {}:", eventId);
			for (auto& s_Pair : *s_EventArray)
			{
				//Logger::Info("\t>\"{}\"", s_Pair.sKey);

				if (s_Pair.sKey == "Name")
				{
					s_EventName = &s_Pair;
					if (s_EventValue != nullptr) break;
				}
				else if (s_Pair.sKey == "Value")
				{
					s_EventValue = &s_Pair;
					if (s_EventName != nullptr) break;
				}
			}

			if (s_EventName != nullptr && s_EventValue != nullptr)
			{
				ZString s_NameData;
				Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&s_EventName->value), &s_NameData);

				Logger::Info("{} == \"Kill\"? {}", s_NameData, (s_NameData == "\"Kill\"")? "Yes" : "No");
				if (s_NameData == "\"Kill\"")
				{
					if (s_EventValue->value.Is< TArray<SDynamicObjectKeyValuePair> >())
					{
						auto* s_EventValueArray = s_EventValue->value.As< TArray<SDynamicObjectKeyValuePair> >();
						auto* s_NewEventArray = &TArray<SDynamicObjectKeyValuePair>();

						for (auto& s_Pair : *s_EventValueArray)
						{
							if (s_Pair.sKey != "RepositoryId") continue;

							ZString s_NewRepoIdStr("\"ee454990-0c4b-49e5-9572-a67887325283\"");

							ZString s_RepoIdData;
							Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&s_Pair.value), &s_RepoIdData);
							if (s_RepoIdData == s_NewRepoIdStr) break;

							Logger::Info("ID was {}", s_RepoIdData);

							auto* s_ZStringSTypeID = (*Globals::TypeRegistry)->m_types.find("ZString")->second;

							if (s_Pair.value.Is<ZString>())
							{
								//ZString s_NewRepoIdStr("\"ee454990-0c4b-49e5-9572-a67887325283\"");
								//s_Pair.value.Assign(s_ZStringSTypeID, &s_NewRepoIdStr);
								//s_Pair.value.m_pData = &s_NewRepoIdStr;

								//auto* s_foo = s_Pair.value.As<ZString>();
								//s_foo = &ZVariantRef<ZString>(&s_NewRepoIdStr);
								//s_foo = &s_NewRepoIdStr;

								for (auto& s_EventPair : *s_EventArray)
								{
									SDynamicObjectKeyValuePair s_NewPair;
									 //s_EventPair.sKey.CopyFrom(s_NewPair.sKey);
									s_NewPair.sKey = s_EventPair.sKey;

									if (s_EventPair.sKey == "Value")
									{

										TArray<SDynamicObjectKeyValuePair>* s_NewEventValueArray = &TArray<SDynamicObjectKeyValuePair>();
										for (auto& s_EventValPair : *s_EventValueArray)
										{
											SDynamicObjectKeyValuePair s_NewValPair;
											//s_EventValPair.sKey.CopyFrom(s_NewValPair.sKey);
											s_NewValPair.sKey = s_EventValPair.sKey;


											if (s_EventValPair.sKey == "RepositoryId")
												s_NewValPair.value.Assign(s_Pair.value.m_pTypeID, &s_NewRepoIdStr);

											else
												s_NewValPair.value.Assign(s_Pair.value.m_pTypeID, &s_Pair.value);
											s_NewEventValueArray->push_back(s_NewValPair);
										}

										s_NewPair.value.Assign(s_EventPair.value.m_pTypeID, s_NewEventValueArray);
									}

									else
										s_NewPair.value.Assign(s_EventPair.value.m_pTypeID, s_EventPair.value.m_pData);

									s_NewEventArray->push_back(s_NewPair);
								}
							}

							ZDynamicObject s_NewEvent = ZDynamicObject();
							s_NewEvent.Assign(event.m_pTypeID, s_NewEventArray);

							Hooks::ZAchievementManagerSimple_OnEventSent->Call(th, eventId + 1, std::move(s_NewEvent));


							m_bIgnoreNextAchOnEventSent = true;
							Hooks::ZAchievementManagerSimple_OnEventSent->Call(th, eventId + 1, std::move(s_NewEvent));
							m_bIgnoreNextAchOnEventSent = false;

							break;
						}
					}
				}
			}
		}
		else Logger::Info("Event {} is not a {}", eventId, "TArray<SDynamicObjectKeyValuePair>");

		ZString s_NewEventData;
		Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&event), &s_NewEventData);
		Logger::Info("s_EventData POST={}", s_NewEventData);

		//ZString* s_EventAsStr = event.As<ZString>();
		//if (s_EventAsStr == nullptr)
		//	s_EventAsStr = &ZString("<nullptr>");
		//
		//// Bad way to make sure it's not null since we just tested but whatever for now
		//if (s_EventAsStr != nullptr)
		//Logger::Info("event={}", *s_EventAsStr);

		if (std::regex_search(
			s_EventDataStr,
			std::regex(
				"\\{.*\\\"Name\\\"\\:\\\"Kill\\\"\\,",
				std::regex::ECMAScript
			)
		))
		{
			std::smatch s_RegexMatchActorName;
			std::string s_ActorName = "<ERROR>";

			if (std::regex_search(
				s_EventDataStr,
				s_RegexMatchActorName,
				std::regex(
					"\\{.*\\\"Value\\\"\\:\\{.*\\\"ActorName\\\"\\:\\\"([^\\\"]+)\\\"\\,",
					std::regex::ECMAScript
				)
			))
				s_ActorName = s_RegexMatchActorName[1].str();

			Logger::Info("{} just ate shit.", s_ActorName);
		}
	}
	catch (std::exception& e)
	{
		Logger::Error("Error: {}", e.what());
	}
	catch (...)
	{
		Logger::Error("Unknown error");
	}

	return HookResult<void>(HookAction::Continue());
}*/


DECLARE_ZHM_PLUGIN(TestMod);
