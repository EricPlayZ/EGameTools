#include <algorithm>
#include <EGSDK\Engine\CVars.h>

namespace EGSDK::Engine {
	std::unordered_map<const CVar*, CVar::VarValue> CVar::varValues;
	CVar::CVar(const std::string& name) : VarBase(name) {}
	CVar::CVar(const std::string& name, VarType type) : VarBase(name, type) {}
	VarValueType& CVar::GetValue() {
		std::lock_guard lock(mutex);
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
		std::lock_guard lock(mutex);
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
		std::lock_guard lock(mutex);
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

	std::unique_ptr<CVar>& CVarMap::try_emplace(std::unique_ptr<CVar> cVar) {
		std::lock_guard lock(mutex);
		const std::string& name = cVar->GetName();
		auto [it, inserted] = vars.try_emplace(name, std::move(cVar));
		if (inserted) {
			varsOrdered.emplace_back(name);
			if (uint32_t valueOffset = it->second->valueOffset.data; valueOffset || valueOffset != 0xCDCDCDCD)
				varsByValueOffset[valueOffset] = it->second.get();
		}
		return it->second;
	}
	bool CVarMap::none_of(uint32_t valueOffset) {
		std::lock_guard lock(mutex);
		return varsByValueOffset.find(valueOffset) == varsByValueOffset.end();
	}

	CVar* CVarMap::Find(uint32_t valueOffset) const {
		std::lock_guard lock(mutex);
		auto it = varsByValueOffset.find(valueOffset);
		return it == varsByValueOffset.end() ? nullptr : it->second;
	}
	void CVarMap::Erase(const std::string& name) {
		std::lock_guard lock(mutex);
		auto it = vars.find(name);
		if (it == vars.end())
			return;

		auto orderIt = std::find(varsOrdered.begin(), varsOrdered.end(), name);
		if (orderIt != varsOrdered.end())
			varsOrdered.erase(orderIt);

		varsByValueOffset.erase(it->second->valueOffset.data);
		vars.erase(it);
	}
}