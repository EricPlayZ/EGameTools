#include <pch.h>

namespace Utils {
	namespace Memory {
		const MODULEINFO GetModuleInfo(const char* szModule) {
			const HMODULE hModule = GetModuleHandle(szModule);
			if (hModule == 0)
				return MODULEINFO();

			MODULEINFO moduleInfo{};
			GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));
			return moduleInfo;
		}
		const FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName) {
			const HMODULE moduleHandle = GetModuleHandleA(module.data());
			if (!moduleHandle)
				return nullptr;

			return GetProcAddress(moduleHandle, funcName.data());
		}

		const bool IsAddressValidMod(const DWORD64 ptr, const char* moduleName) {
			const MODULEINFO moduleInf = GetModuleInfo(moduleName);

			const DWORD64 moduleEntryPoint = reinterpret_cast<DWORD64>(moduleInf.EntryPoint);
			const DWORD64 moduleEndPoint = moduleEntryPoint + moduleInf.SizeOfImage;
			return ptr && (ptr <= moduleEndPoint && ptr >= moduleEntryPoint);
		}

		static std::string bytesToIDAPattern(BYTE* bytes, size_t size) {
			std::stringstream idaPattern;
			idaPattern << std::hex << std::uppercase << std::setfill('0');

			for (size_t i = 0; i < size; i++) {
				const int currentByte = bytes[i];
				if (currentByte != 255)
					idaPattern << std::setw(2) << currentByte;
				else
					idaPattern << "??";

				if (i != size - 1)
					idaPattern << " ";
			}

			return idaPattern.str();
		}
		DWORD64 CalcTargetAddrOfRelInst(DWORD64 addrOfInst, size_t opSize) {
			int offset = *reinterpret_cast<int*>(addrOfInst + opSize);

			return addrOfInst + opSize + 4 + offset;
		}
		std::vector<DWORD64> GetXrefsTo(DWORD64 address, DWORD64 start, size_t size) {
			std::vector<DWORD64> xrefs = {};

			const std::string idaPattern = bytesToIDAPattern(reinterpret_cast<BYTE*>(&address), 8);
			const DWORD64 end = start + size;

			while (start && start < end) {
				DWORD64 xref = reinterpret_cast<DWORD64>(Utils::SigScan::PatternScanner::FindPattern(reinterpret_cast<LPVOID>(start), size, { idaPattern.c_str(), Utils::SigScan::PatternType::Address }));

				if (!xref)
					break;

				xrefs.push_back(xref);
				start = xref + 8;
			}

			return xrefs;
		}
	}
}