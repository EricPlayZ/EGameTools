#pragma once
#include <EGSDK\Exports.h>
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace EGSDK::Utils {

    class EGameSDK_API OffsetExtractor {
    public:
        // Retrieves an offset dynamically by scanning the binary for the SEventPlayer registration pattern
        static uint32_t GetFieldOffset(const std::string& fieldName);
        
        static void Initialize();

    private:
        static std::unordered_map<std::string, uint32_t> m_Offsets;
        static bool m_Initialized;
    };

}
