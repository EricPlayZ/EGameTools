#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGSDK\GamePH\TimeWeather\EWeather.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::GamePH {
	namespace TimeWeather {
		void CSystem::SetForcedWeather(int weather) {
			Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?SetForcedWeather@CSystem@TimeWeather@@QEAAXW4TYPE@EWeather@@VApiDebugAccess@2@@Z", this, weather);
		}
		void CSystem::ClearForcedWeather() {
			Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?ClearForcedWeather@CSystem@TimeWeather@@QEAAXVApiDebugAccess@2@@Z", this);
		}
		EWeather CSystem::GetCurrentWeather() const {
			return Utils::Memory::SafeCallFunction<EWeather>("engine_x64_rwdi.dll", "?GetCurrentWeather@CSystem@TimeWeather@@QEBA?AW4TYPE@EWeather@@XZ", EWeather::Default, this);
		}

		void CSystem::ReloadSubsystems() {
			Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?ReloadSubsystems@CSystem@TimeWeather@@QEAAXXZ", this);
		}
		void CSystem::RequestTimeWeatherInterpolation(int weather, float a3, float a4) {
			Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?RequestTimeWeatherInterpolation@CSystem@TimeWeather@@QEAAXW4TYPE@EWeather@@USTime24H@2@M@Z", this, weather, a3, a4);
		}
		void CSystem::FinishTimeWeatherInterpolation() {
			Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?FinishTimeWeatherInterpolation@CSystem@TimeWeather@@QEAAXXZ", this);
		}
		bool CSystem::IsFullyBlended() {
			return Utils::Memory::SafeCallFunction<bool>("engine_x64_rwdi.dll", "?IsFullyBlended@CSystem@TimeWeather@@QEBA_NVApiEditorAccess@2@@Z", false, this);
		}

		static CSystem* GetOffset_CSystem() {
			LevelDI* pLevelDI = LevelDI::Get();
			return pLevelDI ? pLevelDI->GetTimeWeatherSystem() : nullptr;
		}
		CSystem* CSystem::Get() {
			return ClassHelpers::SafeGetter<CSystem>(GetOffset_CSystem, false, false);
		}
	}
}