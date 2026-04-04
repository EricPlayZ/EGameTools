#include <Windows.h>
#include <Psapi.h>
#include <list>
#include <memscan\memscan.h>
#include <EGSDK\Utils\Sigscan.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::Utils {
	namespace SigScan {
		void* PatternScanner::FindPattern(const std::string_view moduleName, const Pattern& pattern) {
			HMODULE hModule = GetModuleHandle(moduleName.data());
			if (!hModule)
				return nullptr;

			return PatternScanner::FindPattern(hModule, Utils::Memory::GetModuleInfo(moduleName.data()).SizeOfImage, pattern);
		}
		void* PatternScanner::FindPattern(const Pattern& pattern) {
			MODULEINFO hModuleInfo;
			GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &hModuleInfo, sizeof(hModuleInfo));
			return PatternScanner::FindPattern(GetModuleHandle(nullptr), hModuleInfo.SizeOfImage, pattern);
		}

		std::vector<void*> PatternScanner::FindPatterns(const std::string_view moduleName, const Pattern& pattern) {
			HMODULE hModule = GetModuleHandle(moduleName.data());
			if (!hModule)
				return {};

			return PatternScanner::FindPatterns(hModule, Utils::Memory::GetModuleInfo(moduleName.data()).SizeOfImage, pattern);
		}
		std::vector<void*> PatternScanner::FindPatterns(void* startAddress, uint64_t searchSize, const Pattern& pattern) {
			std::vector<void*> ret;

			void* base = startAddress;
			uint64_t size = searchSize;
			void* addr = base;
			do {
				addr = PatternScanner::FindPattern(reinterpret_cast<void*>((reinterpret_cast<uint64_t>(addr) + 1)), size - (reinterpret_cast<uint64_t>(addr) - reinterpret_cast<uint64_t>(base) + 1), pattern);
				if (addr)
					ret.push_back(addr);
			} while (!addr);

			return ret;
		}

		void* PatternScanner::FindPattern(void* startAddress, uint64_t searchSize, const Pattern& pattern) {
			int offset = 0;
			std::string patt = Memory::ConvertSigToScannerSig(pattern.pattern, &offset);

			const auto scanner = memscan::mapped_region_t(reinterpret_cast<uint64_t>(startAddress), reinterpret_cast<uint64_t>(startAddress) + searchSize);
			auto patternFind = scanner.find_pattern<ms_uptr_t>(patt);

			void* ret = nullptr;

			if (patternFind.has_value())
				ret = reinterpret_cast<void*>(patternFind.value() + offset);

			switch (pattern.type) {
			case PatternType::Pointer:
				ret = PatternScanner::ResolvePtr<uint64_t>(ret);
				break;

			case PatternType::PointerBYTE:
				ret = PatternScanner::ResolvePtr<uint8_t>(ret);
				break;

			case PatternType::PointerWORD:
				ret = PatternScanner::ResolvePtr<uint16_t>(ret);
				break;

			case PatternType::PointerQWORD:
				ret = PatternScanner::ResolvePtr<uint64_t>(ret);
				break;

			case PatternType::RelativePointer:
				ret = PatternScanner::ResolveRelativePtr<int32_t>(ret);
				break;

			case PatternType::RelativePointerBYTE:
				ret = PatternScanner::ResolveRelativePtr<uint8_t>(ret);
				break;

			case PatternType::RelativePointerWORD:
				ret = PatternScanner::ResolveRelativePtr<uint16_t>(ret);
				break;

			case PatternType::RelativePointerQWORD:
				ret = PatternScanner::ResolveRelativePtr<uint64_t>(ret);
				break;

			case PatternType::MemberDisplacement32:
				if (!ret || Utils::Memory::IsBadReadPtr(ret, sizeof(int32_t)))
					ret = nullptr;
				else {
					const int32_t displacement = *reinterpret_cast<const int32_t*>(ret);
					ret = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(displacement)));
				}
				break;

			case PatternType::MemberDisplacement8:
				if (!ret || Utils::Memory::IsBadReadPtr(ret, sizeof(uint8_t)))
					ret = nullptr;
				else
					ret = reinterpret_cast<void*>(static_cast<uintptr_t>(*reinterpret_cast<const uint8_t*>(ret)));
				break;

			default:
				break;
			}

			return ret;
		}

		void* PatternScanner::FindPattern(void* startAddress, uint64_t searchSize, const Pattern* patterns, float* ratio) {
			int totalCount = 0;
			struct result {
				void* addr;
				int count;
			};
			std::list<result> results;

			int bestCount = 0;
			void* ret = nullptr;

			while (true) {
				Pattern pattern = patterns[totalCount];
				if (!pattern.pattern)
					break;

				totalCount++;

				void* addr = PatternScanner::FindPattern(startAddress, searchSize, pattern);
				if (!addr)
					continue;

				bool found = false;
				auto it = std::find_if(results.begin(), results.end(), [addr](result& res) { return res.addr == addr; });
				if (it != results.end()) {
					it->count++;

					if (it->count > bestCount) {
						ret = addr;
						bestCount = it->count;
					}

					found = true;
				}

				if (!found) {
					result res = { addr, 1 };
					results.push_back(res);

					if (bestCount == 0) {
						bestCount = 1;
						ret = addr;
					}
				}
			}

			if (ratio)
				*ratio = static_cast<float>(bestCount) / totalCount;

			return ret;
		}
		void* PatternScanner::FindPattern(const std::string_view ModuleName, Pattern* patterns, float* ratio) {
			return PatternScanner::FindPattern(GetModuleHandle(ModuleName.data()), Utils::Memory::GetModuleInfo(ModuleName.data()).SizeOfImage, patterns, ratio);
		}
	}
}