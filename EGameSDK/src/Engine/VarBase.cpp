#include <EGSDK\Engine\VarBase.h>

namespace EGSDK::Engine {
	std::unordered_map<const VarBase*, std::string> VarBase::varNames{};
	std::unordered_map<const VarBase*, VarType> VarBase::varTypes{};
	std::recursive_mutex VarBase::mutex{};

	VarBase::VarBase(const std::string& name, VarType type) {
		std::lock_guard lock(mutex);
		SetName(name);
		SetType(type);
	}
	VarBase::~VarBase() {
		std::lock_guard lock(mutex);
		varNames.erase(this);
		varTypes.erase(this);
	}

	const char* VarBase::GetName() const {
		std::lock_guard lock(mutex);
		auto it = varNames.find(this);
		return it != varNames.end() ? it->second.c_str() : nullptr;
	}
	void VarBase::SetName(const std::string& newName) {
		std::lock_guard lock(mutex);
		varNames[this] = newName;
	}

	VarType VarBase::GetType() const {
		std::lock_guard lock(mutex);
		auto it = varTypes.find(this);
		return it != varTypes.end() ? it->second : VarType::NONE;
	}
	void VarBase::SetType(VarType newType) {
		std::lock_guard lock(mutex);
		varTypes[this] = newType;
	}
}