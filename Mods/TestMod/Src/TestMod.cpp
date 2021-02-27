// Andrew Pratt 2021
// TestMod

#include "TestMod.h"
#include<fstream>
#include<algorithm>
#include<iterator>
#include<iostream>
#include<random>
#include "Common.h"
#include "Logging.h"
#include "Events.h"
#include "Glacier/ZActor.h"


TestMod::TestMod()
{
}


TestMod::~TestMod()
{
	const ZMemberDelegate<TestMod, void(const SGameUpdateEvent&)> s_Delegate(this, &TestMod::OnFrameUpdate);
	Hooks::ZGameLoopManager_UnregisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}


void TestMod::Init()
{
	Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &TestMod::WndProc);

	Hooks::GetPropertyValue->AddDetour(this, &TestMod::GetPropertyValue);
	Hooks::SetPropertyValue->AddDetour(this, &TestMod::SetPropertyValue);

	Hooks::SignalOutputPin->AddDetour(this, &TestMod::SignalOutputPin);
	Hooks::SignalInputPin->AddDetour(this, &TestMod::SignalInputPin);

	Events::OnConsoleCommand->AddListener(this, &TestMod::OnConsoleCommand);
}


void TestMod::OnEngineInitialized()
{
	const ZMemberDelegate<TestMod, void(const SGameUpdateEvent&)> s_Delegate(this, &TestMod::OnFrameUpdate);
	Hooks::ZGameLoopManager_RegisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}



void TestMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
}



DECLARE_PLUGIN_DETOUR(TestMod, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam, LPARAM p_Lparam)
{
	if (p_Message == WM_KEYDOWN)
	{
		if (p_Wparam == VK_NUMPAD7)
		{
			bool s_bGetByName = false;
			const char* s_NAME = "Edward \"Ted\" Mendez";
			const uint64_t s_ENT_ID = 448134596;

			ZActor* s_Actor;
			if (s_bGetByName)
				s_Actor = Globals::ActorManager->GetActorByName(s_NAME);
			else
				s_Actor = Globals::ActorManager->GetActorById(s_ENT_ID);

			if (s_Actor == nullptr)
			{
				if (s_bGetByName)
					Logger::Warn("Couldn\'t find actor named \"{}\"", s_NAME);
				else
					Logger::Warn("Couldn\'t find actor with id {}", s_ENT_ID);

				return HookResult<LRESULT>(HookAction::Continue());
			}

			ZEntityRef s_EntRef;
			s_Actor->GetID(&s_EntRef);

			//s_Actor->m_bUnk29 = true;
			//DumpEntityProperties(s_EntRef);
			DumpEntityInterfaces(s_EntRef);
		}


		else if (p_Wparam == VK_NUMPAD8)
		{
			try
			{
				std::vector<ZRepositoryID> s_ActorOutfits;
			
				// Get all outfits
				for (int i = 0; i < *Globals::NextActorId; ++i)
				{
					auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

					ZEntityRef s_EntRef;
					s_Actor->GetID(&s_EntRef);

					s_ActorOutfits.push_back(s_EntRef.GetProperty<ZRepositoryID>("m_OutfitRepositoryID").Get());
				}

				// Shuffle outfits
				std::random_device s_Rd;
				std::shuffle(s_ActorOutfits.begin(), s_ActorOutfits.end(), std::mt19937(s_Rd()));

				// Apply new outfits
				for (int i = 0; i < *Globals::NextActorId; ++i)
				{
					auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

					ZEntityRef s_EntRef;
					s_Actor->GetID(&s_EntRef);

					ZString s_ActorName = s_EntRef.GetProperty<ZString>("m_sActorName").Get();
					ZRepositoryID s_ActorOldOutfit = s_EntRef.GetProperty<ZRepositoryID>("m_OutfitRepositoryID").Get();
					ZRepositoryID s_ActorNewOutfit = s_ActorOutfits.back();
					s_ActorOutfits.pop_back();

					s_EntRef.SetProperty("m_OutfitRepositoryID", s_ActorNewOutfit);

					Logger::Info("Set outfit for {} from {} to {}", s_ActorName.c_str(), s_ActorOldOutfit.ToString(), s_ActorNewOutfit.ToString());
				}
			}
			catch (const std::exception e)
			{
				Logger::Error("Error occurred while swapping outfits: {}", e.what());
			}
			catch (...)
			{
				Logger::Error("Unknown error occurred while swapping outfits");
			}
		}


		
		else if (p_Wparam == VK_SUBTRACT)
		{
			Logger::Info("");
		}
	}

	return HookResult<LRESULT>(HookAction::Continue());
}



DECLARE_PLUGIN_DETOUR(TestMod, bool, GetPropertyValue, ZEntityRef p_Entity, uint32_t p_PropertyId, void* p_Output)
{
	if ((*p_Entity.m_pEntity)->m_nEntityId == 18436612475885264502)
	{
		Logger::Info("<<FOUND!>> GET");
		DumpEntityProperties(p_Entity);
	}

	return HookResult<bool>(HookAction::Continue());

	if (!m_bCallingFromMod && TestImportantProperties(p_Entity))
	{
		ZString s_PropertyValue = *reinterpret_cast<ZString*>(p_Output);
		Logger::Info("Yo {}\n>>>>>>STATE={}", (*p_Entity.m_pEntity)->m_nEntityId, s_PropertyValue.c_str());
		Logger::Info("HIT AT &&Get\t{}", p_PropertyId);
	}

	/*if (p_PropertyId == 448134596)
	{
		DumpEntityProperties(p_Entity);
		Logger::Info("Hit! G");
	}*/

	//TestImportantProperties(p_Entity);

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(TestMod, bool, SetPropertyValue, ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, bool p_InvokeChangeHandlers)
{
	if ((*p_Entity.m_pEntity)->m_nEntityId == 18436612475885264502)
	{
		Logger::Info("<<FOUND!>> set");
		DumpEntityProperties(p_Entity);
	}

	return HookResult<bool>(HookAction::Continue());

	m_bCallingFromMod = true;
	if (TestImportantProperties(p_Entity))
	{
		//ZString s_PropertyValue = p_Entity.GetProperty<ZString>("m_sMatch").Get();
		//uint32_t s_PropertyValue = p_Entity.GetProperty<uint32_t>("m_eEmotionState").Get();
		float32 s_PropertyValue = p_Entity.GetProperty<float32>("m_fMass").Get();

		Logger::Info("DUMPING\n{");
		try
		{
			//Logger::Info("Yo {}\n>>>>>>STATE={}", (*p_Entity.m_pEntity)->m_nEntityId, ToString(s_PropertyValue.m_pInterfaceRef->m_mTransform));
			Logger::Info("Yo {}\n>>>>>>STATE={}", (*p_Entity.m_pEntity)->m_nEntityId, s_PropertyValue);

			/*bool s_PropertyIgnoreLowNoise = p_Entity.GetProperty<bool>("m_bIgnoreLowNoise").Get();
			bool s_PropertyIgnoreSillyHitman = p_Entity.GetProperty<bool>("m_bIgnoreSillyHitman").Get();
			bool s_PropertyIgnoreAnnoyingHitman = p_Entity.GetProperty<bool>("m_bIgnoreAnnoyingHitman").Get();
			bool s_PropertyIgnoreDistractions = p_Entity.GetProperty<bool>("m_bIgnoreDistractions").Get();
			bool s_PropertyIgnoreAccidents = p_Entity.GetProperty<bool>("m_bIgnoreAccidents").Get();
			bool s_PropertyNeverSpectate = p_Entity.GetProperty<bool>("m_bNeverSpectate").Get();
			bool s_PropertyWantsPrivacy = p_Entity.GetProperty<bool>("m_bWantsPrivacy").Get();
			bool s_PropertySuppressSocialGreeting = p_Entity.GetProperty<bool>("m_bSuppressSocialGreeting").Get();
			
			Logger::Info("\tm_bIgnoreLowNoise:\t{}", s_PropertyIgnoreLowNoise);
			Logger::Info("\tm_bIgnoreSillyHitman:\t{}", s_PropertyIgnoreSillyHitman);
			Logger::Info("\tm_bIgnoreAnnoyingHitman:\t{}", s_PropertyIgnoreAnnoyingHitman);
			Logger::Info("\tm_bIgnoreDistractions:\t{}", s_PropertyIgnoreDistractions);
			Logger::Info("\tm_bIgnoreAccidents:\t{}", s_PropertyIgnoreAccidents);
			Logger::Info("\tm_bNeverSpectate:\t{}", s_PropertyNeverSpectate);
			Logger::Info("\tm_bWantsPrivacy:\t{}", s_PropertyWantsPrivacy);
			Logger::Info("\tm_bSuppressSocialGreeting:\t{}", s_PropertySuppressSocialGreeting);*/
		}
		catch (...)
		{
			Logger::Warn("Yo {}\n>>>>>><failed>", (*p_Entity.m_pEntity)->m_nEntityId);
		}


		Logger::Info("HIT AT !!Set\t{}", p_PropertyId);
		Logger::Info("}");
	}

	if (p_PropertyId == 448134596)
	{
		DumpEntityProperties(p_Entity);
		Logger::Info("S Hit!");
	}

	//TestImportantProperties(p_Entity);

	m_bCallingFromMod = false;
	return HookResult<bool>(HookAction::Continue());
}



DECLARE_PLUGIN_DETOUR(TestMod, bool, SignalOutputPin, ZEntityRef p_Entity, uint32_t p_PinId, const ZObjectRef& p_Data)
{
	if ((*p_Entity.m_pEntity)->m_nEntityId == 18436612475885264502)
	{
		Logger::Info("<<FOUND!>> OPIN");
		DumpEntityProperties(p_Entity);
	}

	return HookResult<bool>(HookAction::Continue());

	return HookResult<bool>(HookAction::Continue());
}


DECLARE_PLUGIN_DETOUR(TestMod, bool, SignalInputPin, ZEntityRef p_Entity, uint32_t p_PinId, const ZObjectRef& p_Data)
{
	if ((*p_Entity.m_pEntity)->m_nEntityId == 18436612475885264502)
	{
		Logger::Info("<<FOUND!>> ipin");
		DumpEntityProperties(p_Entity);
	}

	return HookResult<bool>(HookAction::Continue());

	return HookResult<bool>(HookAction::Continue());
}



float32 TestMod::cosf32(double p_X)
{
	return static_cast<float32>(cos(p_X));
}

float32 TestMod::cosf32(float32 p_X)
{
	return cosf32(static_cast<double>(p_X));
}

float32 TestMod::sinf32(double p_X)
{
	return static_cast<float32>(sin(p_X));
}

float32 TestMod::sinf32(float32 p_X)
{
	return sinf32(static_cast<double>(p_X));
}


void TestMod::DumpEntityProperties(ZEntityRef& p_Ent)
{
	auto& s_Properties = (*p_Ent.m_pEntity)->m_pProperties01;

	if (!s_Properties)
	{
		Logger::Info("<No Properties>");
		return;
	}

	Logger::Info("{");
	for (auto& s_Property : *s_Properties)
	{
		uint32_t s_PropertyId = s_Property.m_nPropertyId;
		ZClassProperty* s_PropertyInfo = s_Property.m_pType->getPropertyInfo();

		const char* s_PropertyName;
		try
		{
			s_PropertyName = s_PropertyInfo->m_pName;
			if (!s_PropertyName)
				s_PropertyName = "<ERROR>";
			else if (s_PropertyName[0] == 0)
				s_PropertyName = "<NO NAME>";
		}
		catch (...)
		{
			s_PropertyName = "<ERROR>";
		}

		const char* s_PropertyTypeName;
		try
		{
			s_PropertyTypeName = s_PropertyInfo->m_pType->m_pType->m_pTypeName;
			if (!s_PropertyTypeName)
				s_PropertyTypeName = "<NULL TYPE NAME>";
			else if (s_PropertyTypeName[0] == 0)
				s_PropertyTypeName = "<NO TYPE NAME>";
		}
		catch (...)
		{
			s_PropertyTypeName = "<ERROR TYPE>";
		}

		if (!s_PropertyName)
			s_PropertyName = "<NULL>";

		if (!s_PropertyTypeName)
			s_PropertyTypeName = "<NULL TYPE NAME>";

		Logger::Info("\tId = {}\n\t{} {}\n", s_PropertyId, s_PropertyTypeName, s_PropertyName);

		if (strcmp(s_PropertyName, "m_rHitmanCharacter") == 0)
		{
			Logger::Info("HITMAN!!!");
		}
	}
	Logger::Info("}");
}


void TestMod::DumpEntityInterfaces(ZEntityRef& p_Ent)
{
	auto& s_Interfaces = (*p_Ent.m_pEntity)->m_pInterfaces;

	if (!s_Interfaces)
	{
		Logger::Info("<No Interfaces>");
		return;
	}

	Logger::Info("{");
	for (auto& s_Interface : *s_Interfaces)
	{
		char* s_InterfaceName;
		try
		{
			s_InterfaceName = s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
			if (!s_InterfaceName)
				s_InterfaceName = "<NULL NAME>";
			else if (s_InterfaceName[0] == 0)
				s_InterfaceName = "<NO NAME>";
		}
		catch (...)
		{
			s_InterfaceName = "<ERROR>";
		}

		Logger::Info("\tInterface: {}", s_InterfaceName);
	}
	Logger::Info("}");
}


bool TestMod::TestImportantProperties(ZEntityRef& p_Ent)
{
	auto& s_Properties = (*p_Ent.m_pEntity)->m_pProperties01;

	if (!s_Properties)
	{
		//Logger::Info("<No Properties>");
		return false;
	}

	for (auto& s_Property : *s_Properties)
	{
		uint32_t s_PropertyId = s_Property.m_nPropertyId;
		ZClassProperty* s_PropertyInfo = s_Property.m_pType->getPropertyInfo();

		const char* s_PropertyName;
		try
		{
			s_PropertyName = s_PropertyInfo->m_pName;
			if (!s_PropertyName)
				s_PropertyName = "<ERROR>";
			else if (s_PropertyName[0] == 0)
				s_PropertyName = "<NO NAME>";
		}
		catch (...)
		{
			s_PropertyName = "<ERROR>";
		}

		const char* s_PropertyTypeName;
		try
		{
			s_PropertyTypeName = s_PropertyInfo->m_pType->m_pType->m_pTypeName;
			if (!s_PropertyTypeName)
				s_PropertyTypeName = "<NULL TYPE NAME>";
			else if (s_PropertyTypeName[0] == 0)
				s_PropertyTypeName = "<NO TYPE NAME>";
		}
		catch (...)
		{
			s_PropertyTypeName = "<ERROR TYPE>";
		}

		if (!s_PropertyName)
			s_PropertyName = "<NULL>";

		if (!s_PropertyTypeName)
			s_PropertyTypeName = "<NULL TYPE NAME>";

		if (!(strlen(s_PropertyTypeName) > 9 && strcmp(std::string(s_PropertyTypeName).substr(1, 9).c_str(), "EntityRef") == 0))
			continue;


		if (strcmp(s_PropertyTypeName, "SVector3") == 0)
		{
			Logger::Info("\tId = {}\n\t{} {}\n", s_PropertyId, s_PropertyTypeName, s_PropertyName);
			//DumpEntityProperties(p_Ent);
			return true;

			/*try
			{
				Logger::Info("  m_fStopMoveDistance: {}", *reinterpret_cast<float32*>(reinterpret_cast<char*>(&p_Ent) + 0xcc));
			}
			catch (...)
			{
				Logger::Info("fail");
			}*/
		}
	}
	return false;
}



DECLARE_PLUGIN_LISTENER(TestMod, OnConsoleCommand)
{
	//Logger::Debug("On console command!");
}



#pragma region PropertyDataToStringSection
std::string TestMod::PropertyDataToString(const std::string& p_TypeName, void* p_Data)
{
	std::string s_DataStr;

	// TODO: Make this cleaner
	if (p_TypeName == "float32")
		s_DataStr = std::to_string(*static_cast<float32*>(p_Data));

	else if (p_TypeName == "bool")
		s_DataStr = std::to_string(*static_cast<bool*>(p_Data));

	else if (p_TypeName == "int16")
		s_DataStr = std::to_string(*static_cast<int32*>(p_Data));

	else if (p_TypeName == "int32")
		s_DataStr = std::to_string(*static_cast<int32*>(p_Data));

	else if (p_TypeName == "int64")
		s_DataStr = std::to_string(*static_cast<int32*>(p_Data));

	else if (p_TypeName == "SVector3")
	{
		auto s_CastedData = static_cast<SVector3*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "SMatrix43")
	{
		auto s_CastedData = static_cast<SMatrix43*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "ZString")
	{
		auto s_CastedData = static_cast<ZString*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "ZActBehaviorEntity.EState")
	{
		auto s_CastedData = static_cast<ZActBehaviorEntity::EState*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "ZResourcePtr")
	{
		auto s_CastedData = static_cast<ZResourcePtr*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "TEntityRef<ZSpatialEntity>")
	{
		auto s_CastedData = static_cast<TEntityRef<ZSpatialEntity>*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "TEntityRef<ZResourcePtr>")
	{
		auto s_CastedData = static_cast<TEntityRef<ZResourcePtr>*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else if (p_TypeName == "TEntityRef<ZActorInstanceEntity>")
	{
		auto s_CastedData = static_cast<TEntityRef<ZActorInstanceEntity>*>(p_Data);

		s_DataStr = ToString(*s_CastedData);
	}

	else
		s_DataStr = "???";


	return s_DataStr;
}



std::string TestMod::ToString(const SVector3& v)
{
	std::string s_DataStr = "vec3(";
	s_DataStr += std::to_string(v.x);
	s_DataStr += ", ";
	s_DataStr += std::to_string(v.y);
	s_DataStr += ", ";
	s_DataStr += std::to_string(v.z);
	s_DataStr += ")";

	return s_DataStr;
}

std::string TestMod::ToString(const SMatrix43& v)
{
	// TODO: Single line version?
	std::string s_DataStr = "mat4x3:\n{\n  x: ";
	s_DataStr += ToString(v.XAxis);
	s_DataStr += "\n  y: ";
	s_DataStr += ToString(v.YAxis);
	s_DataStr += "\n  z: ";
	s_DataStr += ToString(v.ZAxis);
	s_DataStr += "\n  t: ";
	s_DataStr += ToString(v.Trans);
	s_DataStr += "\n}";

	return s_DataStr;
}

std::string TestMod::ToString(const ZString& v)
{
	const char* s_DataStr = v.c_str();
	if (!s_DataStr) return "<CSTR ERROR>";
	return s_DataStr;
}

std::string TestMod::ToString(const ZActBehaviorEntity::EState& v)
{
	switch (v)
	{
	case ZActBehaviorEntity::EState::UNDEFINED:
		return "UNDEFINED";
		break;
	case ZActBehaviorEntity::EState::IDLE:
		return "IDLE";
		break;
	case ZActBehaviorEntity::EState::STOPPING:
		return "STOPPING";
		break;
	case ZActBehaviorEntity::EState::PREPARING:
		return "PREPARING";
		break;
	case ZActBehaviorEntity::EState::MOVING:
		return "MOVING";
		break;
	case ZActBehaviorEntity::EState::ENTERING:
		return "ENTERING";
		break;
	case ZActBehaviorEntity::EState::RUNNING:
		return "RUNNING";
		break;
	case ZActBehaviorEntity::EState::TIMEDOUT:
		return "TIMEDOUT";
		break;
	case ZActBehaviorEntity::EState::COMPLETE:
		return "COMPLETE";
		break;
	default:
		return "<INVALID!>";
	}
}

std::string TestMod::ToString(const ZSpatialEntity& v)
{
	// TODO: Single line version?
	std::string s_DataStr = "ZSpatialEntity\n{\n  ZSpatialEntity::ERoomBehaviour\tm_eRoomBehaviour:\t";
	s_DataStr += ToString(v.m_eRoomBehaviour);
	s_DataStr += "\n  bool\tm_bForceVisible:\t";
	s_DataStr += std::to_string(v.m_bForceVisible);
	s_DataStr += "\n  SMatrix43\tm_mTransform:\t";
	s_DataStr += ToString(v.m_mTransform);
	s_DataStr += "\n  bool\tm_bVisible:\t";
	s_DataStr += std::to_string(v.m_bVisible);
	s_DataStr += "\n  bool\tm_bIsPrivate:\t";
	s_DataStr += std::to_string(v.m_bIsPrivate);
	s_DataStr += "\n  bool\tm_bVisibleInBoxReflection:\t";
	s_DataStr += std::to_string(v.m_bVisibleInBoxReflection);
	s_DataStr += "\n  bool\tm_bEnableTAA:\t";
	s_DataStr += std::to_string(v.m_bVisible);
	s_DataStr += "\n  uint8\tm_nViewportVisibility:\t";
	s_DataStr += std::to_string(v.m_nViewportVisibility);
	s_DataStr += "\n  TEntityRef<ZSpatialEntity>\tm_eidParent:\t";
	s_DataStr += ToString(v.m_eidParent);
	s_DataStr += "\n}";

	return s_DataStr;
}

std::string TestMod::ToString(const ZSpatialEntity::ERoomBehaviour& v)
{
	switch (v)
	{
	case ZSpatialEntity::ERoomBehaviour::ROOM_STATIC:
		return "ROOM_STATIC";
		break;
	case ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC:
		return "IDLE";
		break;
	case ZSpatialEntity::ERoomBehaviour::ROOM_STATIC_OUTSIDE_CLIENT:
		return "STOPPING";
		break;
	default:
		return "<INVALID!>";
	}
}

std::string TestMod::ToString(const ZResourcePtr& v)
{
	return std::to_string(v.m_nResourceIndex);
}

std::string TestMod::ToString(const ZActorInstanceEntity& v)
{
	std::string s_DataStr = "ZActorInstanceEntity(\"";
	s_DataStr += ToString(v.m_animationName);
	return std::string();
}


std::string TestMod::ToString(const TEntityRef<ZSpatialEntity>& v)
{
	std::string s_DataStr = "Entity Ref: ";
	//return std::string(s_DataStr + ToString(*v.m_pInterfaceRef));
	return "Ref to ZSpatialEntity";
}

std::string TestMod::ToString(const TEntityRef<ZResourcePtr>& v)
{
	std::string s_DataStr = "Entity Ref: ";
	return std::string(s_DataStr + ToString(*v.m_pInterfaceRef));
}

std::string TestMod::ToString(const TEntityRef<ZActorInstanceEntity>& v)
{
	std::string s_DataStr = "Entity Ref: ";
	return std::string(s_DataStr + ToString(*v.m_pInterfaceRef));
}
#pragma endregion




DECLARE_ZHM_PLUGIN(TestMod);