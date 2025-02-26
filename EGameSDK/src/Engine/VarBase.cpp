#include <EGSDK\Engine\VarBase.h>

namespace EGSDK::Engine {
	std::unordered_map<const VarBase*, std::string> VarBase::varNames{};
	std::unordered_map<const VarBase*, VarType> VarBase::varTypes{};
	std::mutex VarBase::writeMutex{};
	std::shared_mutex VarBase::readMutex{};

	VarBase::VarBase(std::string_view name, VarType type) {
		SetName(name);
		SetType(type);
	}
	VarBase::~VarBase() {
		varNames.erase(this);
		varTypes.erase(this);
	}

	const char* VarBase::GetName() const {
		std::shared_lock lock(readMutex);
		auto it = varNames.find(this);
		return it != varNames.end() ? it->second.c_str() : nullptr;
	}
	void VarBase::SetName(std::string_view newName) {
		std::lock_guard lock(writeMutex);
		varNames[this] = newName;
	}

	VarType VarBase::GetType() const {
		std::shared_lock lock(readMutex);
		auto it = varTypes.find(this);
		return it != varTypes.end() ? it->second : VarType::NONE;
	}
	void VarBase::SetType(VarType newType) {
		std::lock_guard lock(writeMutex);
		varTypes[this] = newType;
	}
}