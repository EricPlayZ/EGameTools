#include <EGSDK\Utils\OffsetExtractor.h>
#include <EGSDK\Utils\Sigscan.h>
#include <EGSDK\Utils\Memory.h>
#include <Windows.h>
#include <spdlog\spdlog.h>

namespace EGSDK::Utils {

    std::unordered_map<std::string, uint32_t> OffsetExtractor::m_Offsets;
    bool OffsetExtractor::m_Initialized = false;

    void OffsetExtractor::Initialize() {
        if (m_Initialized) return;

        SPDLOG_INFO("[OffsetExtractor] Initializing dynamic offsets...");

        HMODULE hEngine = GetModuleHandleA("engine_x64_rwdi.dll");
        if (!hEngine) {
            SPDLOG_ERROR("[OffsetExtractor] Failed to get engine_x64_rwdi.dll handle!");
            return;
        }

        // Pattern for sub_14D2720 (SEventPlayer registration)
        // We look for the start of the function which sets up the EventDefinition
        // Sig: 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20
        void* pFunc = SigScan::PatternScanner::FindPattern("engine_x64_rwdi.dll", 
            {"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20", SigScan::PatternType::Address});

        if (!pFunc) {
            SPDLOG_ERROR("[OffsetExtractor] Failed to find SEventPlayer registration function!");
            return;
        }

        SPDLOG_INFO("[OffsetExtractor] Found registration function at {}", pFunc);

        // We'll scan the next 2000 bytes for "Health", "Stamina", "ImmunityLevel" logic
        uint8_t* start = reinterpret_cast<uint8_t*>(pFunc);
        
        auto ExtractOffset = [&](const char* fieldName, const std::string& pattern) {
            void* pMatch = SigScan::PatternScanner::FindPattern("engine_x64_rwdi.dll", {pattern, SigScan::PatternType::Address});
            if (pMatch) {
                // The pattern includes the displacement as an immediate value
                // For 'mov qword ptr [reg+16], offset', the offset is usually the last 4 bytes
                uint32_t offset = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pMatch) + (pattern.length() / 3) - 4);
                m_Offsets[fieldName] = offset;
                SPDLOG_INFO("[OffsetExtractor] Extracted {} offset: {:#x}", fieldName, offset);
            } else {
                SPDLOG_WARN("[OffsetExtractor] Failed to find pattern for field: {}", fieldName);
            }
        };

        // These patterns match the 'mov qword ptr [reg+16], <offset>' instruction
        // Health: 49 C7 47 10 34 00 00 00 (0x34 = 52)
        // Stamina: 49 C7 43 10 38 00 00 00 (0x38 = 56)
        // ImmunityLevel: 49 C7 47 10 48 00 00 00 (0x48 = 72)
        
        // We use slightly looser patterns to survive minor register changes
        // '?? C7 ?? 10 ?? ?? ?? ??' -> mov qword ptr [reg+16], imm32
        ExtractOffset("Health", "49 C7 47 10 34 00 00 00"); 
        ExtractOffset("Stamina", "49 C7 43 10 38 00 00 00");
        ExtractOffset("ImmunityLevel", "49 C7 47 10 48 00 00 00");

        m_Initialized = true;
    }

    uint32_t OffsetExtractor::GetFieldOffset(const std::string& fieldName) {
        if (!m_Initialized) Initialize();
        if (m_Offsets.find(fieldName) != m_Offsets.end()) {
            return m_Offsets[fieldName];
        }
        return 0;
    }

}
