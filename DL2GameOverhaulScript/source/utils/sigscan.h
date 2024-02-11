#pragma once
#include <WTypesbase.h>
#include <string_view>
#include <vector>

namespace Utils {
	namespace SigScan {
		enum PatternType {
			Address,
			Pointer,
			PointerBYTE,
			PointerWORD,
			PointerDWORD,
			PointerQWORD,
			RelativePointer,
			RelativePointerBYTE,
			RelativePointerWORD,
			RelativePointerDWORD,
			RelativePointerQWORD,
		};

		struct Pattern {
			const char* pattern;
			PatternType type;
		};

		class PatternScanner {
		public:
			static LPVOID FindPattern(LPVOID startAddress, DWORD64 searchSize, const Pattern& pattern);
			static LPVOID FindPattern(const std::string_view ModuleName, const Pattern& pattern);
			static LPVOID FindPattern(const Pattern& pattern);

			static std::vector<LPVOID> FindPatterns(const std::string_view ModuleName, const Pattern& pattern);
			static std::vector<LPVOID> FindPatterns(LPVOID startAddress, DWORD64 searchSize, const Pattern& pattern);

			static LPVOID FindPattern(LPVOID startAddress, DWORD64 searchSize, const Pattern* patterns, float* ratio = nullptr);
			static LPVOID FindPattern(const std::string_view ModuleName, Pattern* patterns, float* ratio = nullptr);
		private:
			template <typename T> static LPVOID ResolveRelativePtr(LPVOID Address) {
				if (!Address)
					return nullptr;

				T offset = *reinterpret_cast<T*>(Address);
				if (!offset)
					return nullptr;

				return reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(Address) + offset + sizeof(T));
			}

			template <typename T> static LPVOID ResolvePtr(LPVOID Address) {
				if (!Address)
					return nullptr;

				return reinterpret_cast<LPVOID>(*reinterpret_cast<T*>(Address));
			}
		};
	}
}