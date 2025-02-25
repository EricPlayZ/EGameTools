#include <algorithm>
#include <EGSDK\Engine\CVars.h>

namespace EGSDK::Engine {
	std::unordered_map<const CVar*, CVar::VarValue> CVar::varValues;
	CVar::CVar(const std::string& name) : VarBase(name) {}
	CVar::CVar(const std::string& name, VarType type) : VarBase(name, type) {}
	VarValueType& CVar::GetValue() {
		std::shared_lock lock(readMutex);
		auto it = varValues.find(this);
		if (it == varValues.end()) {
			switch (GetType()) {
				case VarType::Float:
					return varValues[this].value = 0.0f;
					break;
				case VarType::Int:
					return varValues[this].value = 0;
					break;
				case VarType::Vec3:
					return varValues[this].value = Vec3();
					break;
				case VarType::Vec4:
					return varValues[this].value = Vec4();
					break;
				default:
					return varValues[this].value = 0;
					break;
			}
		}
		auto& varData = it->second;
		if (varData.valuePtrs.empty())
			return varData.value;
		auto ptr = varData.valuePtrs[0];
		if (!ptr)
			return varData.value;
		switch (GetType()) {
			case VarType::Float:
				return varData.value = *reinterpret_cast<float*>(ptr);
			case VarType::Int:
				return varData.value = *reinterpret_cast<int*>(ptr);
			case VarType::Vec3:
				return varData.value = *reinterpret_cast<Vec3*>(ptr);
			case VarType::Vec4:
				return varData.value = *reinterpret_cast<Vec4*>(ptr);
			default:
				return it->second.value;
		}
	}
	void CVar::SetValue(const VarValueType& value) {
		std::lock_guard lock(writeMutex);
		auto& varData = varValues[this];

		std::visit([&](auto&& val) {
			using T = std::decay_t<decltype(val)>;
			if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4>) {
				for (auto* ptr : varData.valuePtrs) {
					if (ptr) {
						if constexpr (std::is_same_v<T, float>)
							*reinterpret_cast<float*>(ptr) = val;
						else if constexpr (std::is_same_v<T, int>)
							*reinterpret_cast<int*>(ptr) = val;
						else if constexpr (std::is_same_v<T, Vec3>)
							*reinterpret_cast<Vec3*>(ptr) = val;
						else if constexpr (std::is_same_v<T, Vec4>)
							*reinterpret_cast<Vec4*>(ptr) = val;
					}
				}
				varData.value = val;
			}
		}, value);
	}
	void CVar::AddValuePtr(uint64_t* ptr) {
		std::lock_guard lock(writeMutex);
		varValues[this].valuePtrs.push_back(ptr);
	}

	FloatCVar::FloatCVar(const std::string& name) : CVar(name) {
		SetType(VarType::Float);
	}
	IntCVar::IntCVar(const std::string& name) : CVar(name) {
		SetType(VarType::Int);
	}
	Vec3CVar::Vec3CVar(const std::string& name) : CVar(name) {
		SetType(VarType::Vec3);
	}
	Vec4CVar::Vec4CVar(const std::string& name) : CVar(name) {
		SetType(VarType::Vec4);
	}

	std::unique_ptr<CVar>& CVarMap::AddVar(std::unique_ptr<CVar> cVar) {
		std::lock_guard lock(writeMutex);
		const char* name = cVar->GetName();
		auto [it, inserted] = vars.try_emplace(name, std::move(cVar));
		if (inserted) {
			varsOrdered.push_back(name);
			if (uint32_t valueOffset = it->second->valueOffset.data; valueOffset || valueOffset != 0xCDCDCDCD)
				varsByValueOffset[valueOffset] = it->second.get();
		}
		else
			cVar.release();
		return it->second;
	}
	CVar* CVarMap::Find(uint32_t valueOffset) const {
		std::shared_lock lock(readMutex);
		auto it = varsByValueOffset.find(valueOffset);
		return it == varsByValueOffset.end() ? nullptr : it->second;
	}
	void CVarMap::Erase(std::string_view name) {
		std::lock_guard lock(writeMutex);
		auto it = vars.find(name);
		if (it == vars.end())
			return;

		auto orderIt = std::find(varsOrdered.begin(), varsOrdered.end(), name);
		if (orderIt != varsOrdered.end())
			varsOrdered.erase(orderIt);

		varsByValueOffset.erase(it->second->valueOffset.data);
		vars.erase(it);
	}

	bool CVarMap::none_of(uint32_t valueOffset) {
		std::shared_lock lock(readMutex);
		return varsByValueOffset.find(valueOffset) == varsByValueOffset.end();
	}

	CVarRef::CVarRef(uint32_t valueOffset, CVarMap& map) : valueOffset(valueOffset), Base(map.Find(valueOffset)) {}
	uint32_t CVarRef::GetValueOffset() const {
		return valueOffset;
	}
	void CVarRef::AddValuePtr(uint64_t* ptr) {
		if (!ptr)
			return;
		Base::ptr->AddValuePtr(ptr);
	}

	std::optional<CVarRef> CVars::GetVarRef(uint32_t valueOffset) {
		return _GetVarRef(valueOffset, Base::vars);
	}
	std::optional<CVarRef> CVars::GetCustomVarRef(uint32_t valueOffset) {
		return _GetVarRef(valueOffset, Base::customVars);
	}
	std::optional<CVarRef> CVars::GetDefaultVarRef(uint32_t valueOffset) {
		return _GetVarRef(valueOffset, Base::defaultVars);
	}
	std::optional<CVarRef> CVars::GetCustomDefaultVarRef(uint32_t valueOffset) {
		return _GetVarRef(valueOffset, Base::defaultCustomVars);
	}
	std::optional<CVarRef> CVars::_GetVarRef(uint32_t valueOffset, CVarMap& map) {
		CVarRef varRef(valueOffset, map);
		return varRef.GetPtr() ? std::optional<CVarRef>(varRef) : std::nullopt;
	}
}