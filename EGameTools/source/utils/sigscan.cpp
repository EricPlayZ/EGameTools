#include <pch.h>

namespace Utils {
	namespace SigScan {
		LPVOID PatternScanner::FindPattern(const std::string_view moduleName, const Pattern& pattern) {
			HMODULE hModule = GetModuleHandleA(moduleName.data());
			if (!hModule)
				return nullptr;

			return PatternScanner::FindPattern(hModule, Utils::Memory::GetModuleInfo(moduleName.data()).SizeOfImage, pattern);
		}
		LPVOID PatternScanner::FindPattern(const Pattern& pattern) {
			MODULEINFO hModuleInfo;
			GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &hModuleInfo, sizeof(hModuleInfo));
			return PatternScanner::FindPattern(GetModuleHandleA(nullptr), hModuleInfo.SizeOfImage, pattern);
		}

		std::vector<LPVOID> PatternScanner::FindPatterns(const std::string_view moduleName, const Pattern& pattern) {
			HMODULE hModule = GetModuleHandleA(moduleName.data());
			if (!hModule)
				return {};

			return PatternScanner::FindPatterns(hModule, Utils::Memory::GetModuleInfo(moduleName.data()).SizeOfImage, pattern);
		}
		std::vector<LPVOID> PatternScanner::FindPatterns(LPVOID startAddress, DWORD64 searchSize, const Pattern& pattern) {
			std::vector<LPVOID> ret;

			LPVOID base = startAddress;
			DWORD64 size = searchSize;
			LPVOID addr = base;
			do {
				addr = PatternScanner::FindPattern(reinterpret_cast<LPVOID>((reinterpret_cast<DWORD64>(addr) + 1)), size - (reinterpret_cast<DWORD64>(addr) - reinterpret_cast<DWORD64>(base) + 1), pattern);
				if (addr)
					ret.push_back(addr);
			} while (!addr);

			return ret;
		}

		LPVOID PatternScanner::FindPattern(LPVOID startAddress, DWORD64 searchSize, const Pattern& pattern) {
			size_t len = strlen(pattern.pattern);
			if (len == 0)
				return nullptr;

			DWORD64 pos = 0;

			size_t byteCount = 1;
			uint32_t i = 0;
			while (i < len - 1) {
				if (pattern.pattern[i] == ' ')
					byteCount++;
				i++;
			}

			BYTE* patt = reinterpret_cast<BYTE*>(malloc(byteCount + 1));
			if (!patt)
				return nullptr;

			BYTE* mask = reinterpret_cast<BYTE*>(malloc(byteCount + 1));
			if (!mask) {
				free(patt);
				return nullptr;
			}

			int offset = 0;
			int bytesCounted = 0;
			i = 0;
			while (i < len - 1) {
				if (pattern.pattern[i] == '[') {
					i++;
					offset = bytesCounted;
				}

				if (pattern.pattern[i] == '\0')
					break;

				if (pattern.pattern[i] == '?' && pattern.pattern[i + 1] == '?') {
					mask[bytesCounted] = '?';
					patt[bytesCounted] = '\0';
				} else {
					BYTE hn = pattern.pattern[i] > '9' ? pattern.pattern[i] - 'A' + 10 : pattern.pattern[i] - '0';
					BYTE ln = pattern.pattern[i + 1] > '9' ? pattern.pattern[i + 1] - 'A' + 10 : pattern.pattern[i + 1] - '0';
					BYTE n = (hn << 4) | ln;

					mask[bytesCounted] = 'x';
					patt[bytesCounted] = n;
				}

				bytesCounted++;

				i += 2;
				while (i < len && (pattern.pattern[i] == ' ' || pattern.pattern[i] == '\t' || pattern.pattern[i] == '\r' || pattern.pattern[i] == '\n'))
					i++;
			}
			if (bytesCounted <= byteCount)
				mask[bytesCounted] = '\0';

			LPVOID ret = nullptr;
			DWORD64 retAddress = reinterpret_cast<DWORD64>(startAddress);
			DWORD64 endAddress = retAddress + searchSize;
			size_t searchLen = bytesCounted;

			while (retAddress < endAddress) {
				__try {
					bool found = true;
					for (size_t j = 0; j < searchLen; j++) {
						BYTE* currentByte = reinterpret_cast<BYTE*>(retAddress + j);
						if (mask[j] == 'x' && *currentByte != patt[j]) {
							found = false;
							break;
						}
					}

					if (found) {
						ret = reinterpret_cast<LPVOID>(retAddress + offset);
						break;
					}

					retAddress++;
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					retAddress++;
				}
			}

			free(patt);
			free(mask);

			switch (pattern.type) {
			case PatternType::Pointer:
				ret = PatternScanner::ResolvePtr<DWORD64>(ret);
				break;

			case PatternType::PointerBYTE:
				ret = PatternScanner::ResolvePtr<BYTE>(ret);
				break;

			case PatternType::PointerWORD:
				ret = PatternScanner::ResolvePtr<WORD>(ret);
				break;

			case PatternType::PointerQWORD:
				ret = PatternScanner::ResolvePtr<DWORD64>(ret);
				break;

			case PatternType::RelativePointer:
				ret = PatternScanner::ResolveRelativePtr<int32_t>(ret);
				break;

			case PatternType::RelativePointerBYTE:
				ret = PatternScanner::ResolveRelativePtr<BYTE>(ret);
				break;

			case PatternType::RelativePointerWORD:
				ret = PatternScanner::ResolveRelativePtr<WORD>(ret);
				break;

			case PatternType::RelativePointerQWORD:
				ret = PatternScanner::ResolveRelativePtr<DWORD64>(ret);
				break;
			default:
				break;
			}

			return ret;
		}

		LPVOID PatternScanner::FindPattern(LPVOID startAddress, DWORD64 searchSize, const Pattern* patterns, float* ratio) {
			int totalCount = 0;
			struct result {
				LPVOID addr;
				int count;
			};
			std::list<result> results;

			int bestCount = 0;
			LPVOID ret = nullptr;

			while (true) {
				Pattern pattern = patterns[totalCount];
				if (!pattern.pattern)
					break;

				totalCount++;

				LPVOID addr = PatternScanner::FindPattern(startAddress, searchSize, pattern);
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
		LPVOID PatternScanner::FindPattern(const std::string_view ModuleName, Pattern* patterns, float* ratio) {
			return PatternScanner::FindPattern(GetModuleHandle(ModuleName.data()), Utils::Memory::GetModuleInfo(ModuleName.data()).SizeOfImage, patterns, ratio);
		}
	}
}