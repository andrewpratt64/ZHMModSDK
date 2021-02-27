#pragma once
// Andrew Pratt 2021
// TestMod

#include<vector>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZActor.h"
#include "TestModUtil.h"


class TestMod : public IPluginInterface
{
protected:
	class SVector3
	{
	public:
		float32 x; // 0x0
		float32 y; // 0x4
		float32 z; // 0x8
	};


	class SMatrix43
	{
	public:
		SVector3 XAxis; // 0x0
		SVector3 YAxis; // 0xC
		SVector3 ZAxis; // 0x18
		SVector3 Trans; // 0x24
	};


	class ZSpatialEntity :
		public IEntity,
		public ZEntityImpl
	{
	public:
		enum class ERoomBehaviour
		{
			ROOM_STATIC = 0,
			ROOM_DYNAMIC = 1,
			ROOM_STATIC_OUTSIDE_CLIENT = 2,
		};

	public:
		ZSpatialEntity::ERoomBehaviour m_eRoomBehaviour; // 0x0
		bool m_bForceVisible; // 0x0
		SMatrix43 m_mTransform; // 0x0
		bool m_bVisible; // 0x0
		bool m_bIsPrivate; // 0x0
		bool m_bVisibleInBoxReflection; // 0x0
		bool m_bEnableTAA; // 0x0
		uint8 m_nViewportVisibility; // 0x0
		TEntityRef<ZSpatialEntity> m_eidParent; // 0x70
	};


	class IBoneAnimator
	{
	public:
	};

	class ZAnimSetEntity :
		public IEntity,
		public ZEntityImpl
	{
	public:
		TResourcePtr<void*> m_animSetResourceID; // 0x18
	};


	class ZActorInstanceEntity :
		public IEntity,
		public IBoneAnimator,
		public ZEntityImpl
	{
	public:
		enum class EFFXMode
		{
			eFFX_MODE_DISABLE = 0,
			eFFX_MODE_OVERWRITE = 1
		};

		TResourcePtr<void*> m_actorResourceID; // 0x20
		TEntityRef<IBoneAnimator> m_BoneAnimator; // 0x28
		ZString m_animationName; // 0x38
		bool m_bLoop; // 0x48
		TArray<TEntityRef<ZAnimSetEntity>> m_animSets; // 0x50
		TArray<TEntityRef<ZAnimSetEntity>> m_AdditionalAnimSets; // 0x68
		ZActorInstanceEntity::EFFXMode m_eBlendMode; // 0x80
	};

	
	class IRoleEventSource {};
	class IRoleListener {};
	class IBoolCondition {};

	class ZBehaviorEntityBase :
		public ZEntityImpl,
		public IRoleEventSource,
		public IRoleListener
	{
	public:
		TArray<TEntityRef<IBoolCondition>> m_conditions; // 0x38
		ZString m_sMatch; // 0x50
	};
	
	
	class ZActBehaviorEntity :
		public ZBehaviorEntityBase/*,
		public IEntity,
		public IRoleEventSource,
		public IRoleListener,
		public IWaypoint,
		public IActListener,
		public ISavableEntity,
		public ZBehaviorEntityBase,
		public IWaypoint,
		public IActListener,
		public ISavableEntity*/
	{
	public:
		enum class EState
		{
			UNDEFINED = 0,
			IDLE = 1,
			STOPPING = 2,
			PREPARING = 3,
			MOVING = 4,
			ENTERING = 5,
			RUNNING = 6,
			TIMEDOUT = 7,
			COMPLETE = 8,
		};

	public:
		EActorEmotionState m_eEmotionState; // 0xA0
		bool m_bFastTransition; // 0xA4
		TEntityRef<ZSpatialEntity> m_rMoveToTransform; // 0xA8
		TEntityRef<ZSpatialEntity> m_rOrientToTransform; // 0xB8
		/*ZActBehaviorEntity.EMovementType m_MovementType; // 0xC8
		float32 m_fStopMoveDistance; // 0xCC
		ZActBehaviorEntity.ERotationAlignment m_AlignRotation; // 0xD0
		TEntityRef<ZChildNetworkActEntity> m_aEnterAct; // 0xD8
		TEntityRef<ZChildNetworkActEntity> m_rAct; // 0xE8
		bool m_bUpperBodyAct; // 0xF8
		float32 m_fActTimeout; // 0xFC
		TArray<TEntityRef<IBoolCondition>> m_StartingConditions; // 0x100
		TArray<TEntityRef<IBoolCondition>> m_EnteringConditions; // 0x118
		TArray<TEntityRef<ZSpatialEntity>> m_aApproachPath; // 0x130
		TArray<TEntityRef<IBoolCondition>> m_ResumingConditions; // 0x148
		ZActBehaviorEntity.EApproachAlignment m_eApproachAlignment; // 0x160
		SColorRGB m_Color; // 0x164
		ZActBehaviorEntity.EState m_eState; // 0x170
		TArray<TEntityRef<ZKeywordEntity>> m_rKeywords; // 0x178
		TEntityRef m_rConvertionHelperTarget; // 0x190
		bool m_bUseConversationHelper; // 0x1A0
		TEntityRef<IBoolCondition> m_rConversationHelperCondition; // 0x1A8
		bool m_bConversationHelperFastTrans; // 0x1B8
		bool m_bAlignPosition; // 0x1B9
		bool m_bIgnoreEndCollision; // 0x1BA
		bool m_bRemoveWhenDone; // 0x1BB
		bool m_bRequestItemAndVarientReset; // 0x1BC
		bool m_bDisableLookAt; // 0x1BD
		bool m_bHolsterDuringAct; // 0x1BE
		bool m_bResetAnimSetAfterAct; // 0x1BF
		bool m_bSendInterruptWhileStopping; // 0x1C0
		bool m_bSendInterruptWhilePreparing; // 0x1C1
		EActorBumpType m_eActorBumpType; // 0x1C4*/
	};

	

public:
	TestMod();
	~TestMod() override;

	void Init() override;
	void OnEngineInitialized() override;

private:
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

	float32 cosf32(double p_X);
	float32 cosf32(float32 p_X);
	float32 sinf32(double p_X);
	float32 sinf32(float32 p_X);

	void DumpEntityProperties(ZEntityRef& p_Ent);
	void DumpEntityInterfaces(ZEntityRef& p_Ent);
	bool TestImportantProperties(ZEntityRef& p_Ent);
	
private:
	DEFINE_PLUGIN_DETOUR(TestMod, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)

	DEFINE_PLUGIN_DETOUR(TestMod, bool, GetPropertyValue, ZEntityRef, uint32_t, void*)
	DEFINE_PLUGIN_DETOUR(TestMod, bool, SetPropertyValue, ZEntityRef, uint32_t, const ZObjectRef&, bool)

	DEFINE_PLUGIN_DETOUR(TestMod, bool, SignalOutputPin, ZEntityRef, uint32_t, const ZObjectRef&)
	DEFINE_PLUGIN_DETOUR(TestMod, bool, SignalInputPin, ZEntityRef, uint32_t, const ZObjectRef&)

	DEFINE_PLUGIN_LISTENER(TestMod, OnConsoleCommand)

#pragma region PropertyDataToStringSection
		std::string PropertyDataToString(const std::string& p_TypeName, void* p_Data);

	std::string ToString(const SVector3& v);
	std::string ToString(const SMatrix43& v);
	std::string ToString(const ZString& v);
	std::string ToString(const ZActBehaviorEntity::EState& v);
	std::string ToString(const ZSpatialEntity& v);
	std::string ToString(const ZSpatialEntity::ERoomBehaviour& v);
	std::string ToString(const ZResourcePtr& v);
	std::string ToString(const ZActorInstanceEntity& v);

	std::string ToString(const TEntityRef<ZSpatialEntity>& v);
	std::string ToString(const TEntityRef<ZResourcePtr>& v);
	std::string ToString(const TEntityRef<ZActorInstanceEntity>& v);
#pragma endregion


private:
	bool m_bCallingFromMod = false;
};

DEFINE_ZHM_PLUGIN(TestMod)