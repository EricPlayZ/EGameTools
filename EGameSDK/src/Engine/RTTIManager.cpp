#include <spdlog\spdlog.h>
#include <EGSDK\Engine\RTTIManager.h>
#include <EGSDK\Utils\Memory.h>
#include <Windows.h>
#include <vector>

namespace EGSDK::Engine {

    typedef CRTTI* (__fastcall* FindClass_t)(void* pManager, const char* name, int type, bool unk);
    typedef void* (__fastcall* GetRTTIManager_t)();

    CRTTI* RTTIManager::GetClass(const char* name) {
        if (!name) return nullptr;
        static FindClass_t FindClass_Fn = nullptr;
        static GetRTTIManager_t GetRTTIManager_Fn = nullptr;
        if (!FindClass_Fn) {
            HMODULE hEngine = GetModuleHandleA("engine_x64_rwdi.dll");
            if (hEngine) {
                FindClass_Fn = (FindClass_t)GetProcAddress(hEngine, "?FindClass@CRTTIManager@@UEBAPEBVCRTTI@@PEBDW4TYPE@FFindClass@1@_N@Z");
                GetRTTIManager_Fn = (GetRTTIManager_t)GetProcAddress(hEngine, "GetRTTIManager");
            }
        }
        if (FindClass_Fn && GetRTTIManager_Fn) {
            void* pManager = GetRTTIManager_Fn();
            if (pManager) return FindClass_Fn(pManager, name, 3, false);
        }
        return nullptr;
    }

    void DumpClass(FILE* f, CRTTI* pClass) {
        if (!pClass || Utils::Memory::IsBadReadPtr(pClass)) return;
        fprintf(f, "Class: %s (Size: 0x%X)\n", pClass->GetName(), pClass->GetSize());
        if (pClass->m_BaseClass) {
            CRTTI* pBase = UntagPointer(pClass->m_BaseClass);
            if (!Utils::Memory::IsBadReadPtr(pBase)) fprintf(f, "  Inherits: %s\n", pBase->GetName());
        }
        fprintf(f, "\n");
    }

    void TraverseMap(FILE* f, void* pNode, std::vector<void*>& visited) {
        if (!pNode || Utils::Memory::IsBadReadPtr(pNode)) return;
        for (auto v : visited) if (v == pNode) return;
        visited.push_back(pNode);
        CRTTI** ppClass = reinterpret_cast<CRTTI**>((uintptr_t)pNode - 8);
        if (!Utils::Memory::IsBadReadPtr(ppClass)) DumpClass(f, *ppClass);
        void** pLeft = reinterpret_cast<void**>((uintptr_t)pNode + 0);
        void** pRight = reinterpret_cast<void**>((uintptr_t)pNode + 16);
        if (!Utils::Memory::IsBadReadPtr(pLeft)) TraverseMap(f, *pLeft, visited);
        if (!Utils::Memory::IsBadReadPtr(pRight)) TraverseMap(f, *pRight, visited);
    }

    void RTTIManager::Dump(const char* filePath) {
        FILE* f = fopen(filePath, "w");
        if (!f) return;
        static GetRTTIManager_t GetRTTIManager_Fn = nullptr;
        if (!GetRTTIManager_Fn) {
            HMODULE hEngine = GetModuleHandleA("engine_x64_rwdi.dll");
            if (hEngine) GetRTTIManager_Fn = (GetRTTIManager_t)GetProcAddress(hEngine, "GetRTTIManager");
        }
        if (GetRTTIManager_Fn) {
            void* pManager = GetRTTIManager_Fn();
            if (pManager) {
                std::vector<void*> visited;
                void** pRoot1 = reinterpret_cast<void**>((uintptr_t)pManager + 0x68);
                void** pRoot2 = reinterpret_cast<void**>((uintptr_t)pManager + 0x78);
                if (!Utils::Memory::IsBadReadPtr(pRoot1)) TraverseMap(f, *pRoot1, visited);
                if (!Utils::Memory::IsBadReadPtr(pRoot2)) TraverseMap(f, *pRoot2, visited);
            }
        }
        fclose(f);
    }

    void* RTTIManager::GetFirstInstance(CRTTI* pClass) {
        if (!pClass || Utils::Memory::IsBadReadPtr(pClass)) return nullptr;

        const char* targetName = pClass->GetName();
        SPDLOG_INFO("[RTTI Discovery] === START: {} ({}) ===", targetName, (void*)pClass);

        HMODULE hEngine = GetModuleHandleA("engine_x64_rwdi.dll");
        if (!hEngine) return nullptr;

        // Traverse CSerializableObject::GetLoadedGSObjectsList()
        typedef void* (__fastcall* GetList_t)();
        static GetList_t GetList_Fn = (GetList_t)GetProcAddress(hEngine, "?GetLoadedGSObjectsList@CSerializableObject@@SAPEAV?$list@PEAVCGSObject@@Vallocator@ttl@@@ttl@@XZ");
        
        if (GetList_Fn) {
            void* pList = GetList_Fn();
            SPDLOG_INFO("[RTTI Discovery] Global Object List addr: {}", pList);

            if (pList && !Utils::Memory::IsBadReadPtr(pList)) {
                // Techland ttl::list structure: [header_node]
                // header_node: [next, prev]
                // nodes: [next, prev, value]
                void* head = pList;
                void* curr = *reinterpret_cast<void**>(head); // first node (next)
                
                uint32_t count = 0;
                while (curr && curr != head && count < 50000) {
                    count++;
                    if (Utils::Memory::IsBadReadPtr(curr)) break;

                    void* pInstance = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(curr) + 16);
                    if (pInstance && !Utils::Memory::IsBadReadPtr(pInstance)) {
                        __try {
                            void** vtable = *reinterpret_cast<void***>(pInstance);
                            if (vtable && !Utils::Memory::IsBadReadPtr(vtable)) {
                                // Scan multiple indices for GetRTTI
                                for (int idx : {2, 3, 5, 6, 1}) {
                                    typedef void** (__fastcall* GetRTTI_t)(void*);
                                    GetRTTI_t GetRTTI_Fn = (GetRTTI_t)vtable[idx];
                                    if (GetRTTI_Fn && !Utils::Memory::IsBadReadPtr(GetRTTI_Fn)) {
                                        void** rttiType = GetRTTI_Fn(pInstance);
                                        if (rttiType && !Utils::Memory::IsBadReadPtr(rttiType)) {
                                            CRTTI* p = reinterpret_cast<CRTTI*>(*rttiType);
                                            if (p && !Utils::Memory::IsBadReadPtr(p) && p == pClass) {
                                                SPDLOG_INFO("[RTTI Discovery] SUCCESS: Found {} at {}", targetName, pInstance);
                                                return pInstance;
                                            }
                                        }
                                    }
                                }
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                    curr = *reinterpret_cast<void**>(curr); // move to next node
                }
                SPDLOG_INFO("[RTTI Discovery] Checked {} active objects.", count);
            }
        } else {
            SPDLOG_ERROR("[RTTI Discovery] FAILED to find GetLoadedGSObjectsList!");
        }

        SPDLOG_WARN("[RTTI Discovery] FAILED to find instance for {}", targetName);
        return nullptr;
    }

}
