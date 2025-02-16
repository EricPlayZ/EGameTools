#pragma once
#include <atomic>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Engine\VarBase.h>
#include <EGSDK\Engine\VarMapBase.h>
#include <EGSDK\Engine\VarManagerBase.h>

namespace EGSDK::GamePH {
	class EGameSDK_API PlayerVar : public Engine::VarBase {
	public:
		union {
			EGSDK::ClassHelpers::StaticBuffer<0x8, const char*> strValue;
			EGSDK::ClassHelpers::StaticBuffer<0x8, float> floatValue;
			EGSDK::ClassHelpers::StaticBuffer<0x8, bool> boolValue;
			EGSDK::ClassHelpers::StaticBuffer<0x10, const char*> defaultStrValue; // remove 0x2 bit to access ptr
			EGSDK::ClassHelpers::StaticBuffer<0xC, float> defaultFloatValue;
			EGSDK::ClassHelpers::StaticBuffer<0x9, bool> defaultBoolValue;
		};
		explicit PlayerVar(const std::string& name);
		explicit PlayerVar(const std::string& name, Engine::VarType type);

		Engine::VarValueType GetValue();
		Engine::VarValueType GetDefaultValue();
		void SetValue(const Engine::VarValueType& value);
	};

	class EGameSDK_API StringPlayerVariable : public PlayerVar {
	public:
		explicit StringPlayerVariable(const std::string& name);
	};
	class EGameSDK_API FloatPlayerVariable : public PlayerVar {
	public:
		explicit FloatPlayerVariable(const std::string& name);
	};
	class EGameSDK_API BoolPlayerVariable : public PlayerVar {
	public:
		explicit BoolPlayerVariable(const std::string& name);
	};

	class EGameSDK_API PlayerVarMap : public Engine::VarMapBase<PlayerVar> {
	public:
		using Engine::VarMapBase<PlayerVar>::Find;
		using Engine::VarMapBase<PlayerVar>::none_of;
		using Engine::VarMapBase<PlayerVar>::empty;
	};

	class EGameSDK_API PlayerVariables : public Engine::VarManagerBase<PlayerVarMap, PlayerVar> {
	public:
		static std::atomic<bool> gotPlayerVars;

#ifdef EGameSDK_EXPORTS
		static void GetPlayerVars();
		static bool SortPlayerVars();
#endif

		static PlayerVariables* Get();
	};
}