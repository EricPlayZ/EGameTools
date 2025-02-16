#include <EGSDK\Engine\VarBase.h>

namespace EGSDK::Engine {
	std::unordered_map<const VarBase*, std::string> VarBase::varNames{};
	std::unordered_map<const VarBase*, VarType> VarBase::varTypes{};
	std::recursive_mutex VarBase::mutex{};

	VarBase::VarBase(const std::string& name) {
		std::lock_guard lock(mutex);
		varNames[this] = name;
		varTypes[this] = VarType::NONE;
	}
	VarBase::VarBase(const std::string& name, VarType type) {
		std::lock_guard lock(mutex);
		varNames[this] = name;
		varTypes[this] = type;
	}
	VarBase::~VarBase() {
		std::lock_guard lock(mutex);
		varNames.erase(this);
		varTypes.erase(this);
	}

	const char* VarBase::GetName() const {
		std::lock_guard lock(mutex);
		auto it = varNames.find(this);
		if (it != varNames.end())
			return it->second.c_str();
		return nullptr;
	}
	void VarBase::SetName(const std::string& newName) {
		std::lock_guard lock(mutex);
		varNames[this] = newName;
	}

	VarType VarBase::GetType() const {
		std::lock_guard lock(mutex);
		auto it = varTypes.find(this);
		if (it != varTypes.end())
			return it->second;
		return VarType::NONE;
	}
	void VarBase::SetType(VarType newType) {
		std::lock_guard lock(mutex);
		varTypes[this] = newType;
	}
}