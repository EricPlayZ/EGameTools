#pragma once
#include <stdint.h>
#include <string_view>
#include <vector>
#include <EGSDK\Exports.h>

namespace EGSDK::Utils {
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
			// Match at '['; read SIB displacement as struct member byte offset (encoded in result pointer bits for GetOffset pipeline).
			MemberDisplacement32,
			MemberDisplacement8,
		};

		struct EGameSDK_API Pattern {
			const char* pattern = nullptr;
			PatternType type;
		};

		class EGameSDK_API PatternScanner {
		public:
			static void* FindPattern(void* startAddress, uint64_t searchSize, const Pattern& pattern);
			static void* FindPattern(const std::string_view moduleName, const Pattern& pattern);
			static void* FindPattern(const Pattern& pattern);

			static std::vector<void*> FindPatterns(const std::string_view moduleName, const Pattern& pattern);
			static std::vector<void*> FindPatterns(void* startAddress, uint64_t searchSize, const Pattern& pattern);

			static void* FindPattern(void* startAddress, uint64_t searchSize, const Pattern* patterns, float* ratio = nullptr);
			static void* FindPattern(const std::string_view moduleName, Pattern* patterns, float* ratio = nullptr);
		private:
			template <typename T> static void* ResolveRelativePtr(void* Address) {
				if (!Address)
					return nullptr;

				T offset = *reinterpret_cast<T*>(Address);
				if (!offset)
					return nullptr;

				return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(Address) + offset + sizeof(T));
			}

			template <typename T> static void* ResolvePtr(void* Address) {
				if (!Address)
					return nullptr;

				return reinterpret_cast<void*>(*reinterpret_cast<T*>(Address));
			}
		};
	}
}