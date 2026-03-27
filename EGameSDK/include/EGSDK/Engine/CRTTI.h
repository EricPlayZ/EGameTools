#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
#include <EGSDK\Exports.h>
#include <EGSDK\Utils\Memory.h>
#include <Windows.h>

namespace EGSDK::Engine {

    // Techland uses pointer tagging. High bits store metadata.
    // Standard x64 user-mode pointers are 48 bits.
    template <typename T>
    inline T* UntagPointer(T* ptr) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) & 0x0000FFFFFFFFFFFF);
    }

    struct CRTTIField {
        void* vtable;           // 0x00
        uintptr_t* m_pName;     // 0x08
        uintptr_t* m_pDesc;     // 0x10
        uint16_t m_Type;        // 0x18
        uint16_t m_Flags;       // 0x1A
        uint32_t m_Unk1C;       // 0x1C
        uint32_t m_Unk20;       // 0x20
        uint32_t m_Unk24;       // 0x24
        uintptr_t m_Unk28;      // 0x28
        uintptr_t m_Unk30;      // 0x30
        uintptr_t m_Unk38;      // 0x38
        uintptr_t m_Unk40;      // 0x40
        uintptr_t m_Unk48;      // 0x48
        uint32_t m_Offset;      // 0x50 (This is a QWORD index!)

        const char* GetName() const {
            if (!m_pName || Utils::Memory::IsBadReadPtr(m_pName)) return "Unknown";
            return UntagPointer(reinterpret_cast<const char*>(*m_pName));
        }

        uint32_t GetOffset() const {
            return m_Offset * 8;
        }
    };

    struct CRTTIObject;

    struct CRTTIMethod {
        void** vtable;      // 0x00
        const char* m_Name; // 0x08
        void* m_Parent;     // 0x10
        uint32_t m_Flags;   // 0x18
        uint32_t m_Pad1C;   // 0x1C
        void* m_Function;   // 0x20

        const char* GetName() const {
            if (!m_Name || Utils::Memory::IsBadReadPtr((void*)m_Name)) return "Unknown";
            return UntagPointer(const_cast<char*>(m_Name));
        }

        template<typename... Args>
        void Invoke(CRTTIObject* obj, Args... args) const {
            if (!m_Function) return;
            void* pFunc = UntagPointer(m_Function);
            typedef void(__fastcall* Func_t)(CRTTIObject*, Args...);
            ((Func_t)pFunc)(obj, args...);
        }
    };

    struct CRTTI {
        void* vtable;           // 0x00
        uintptr_t m_Unk8;       // 0x08
        uintptr_t* m_pName;     // 0x10
        uint32_t m_Size;        // 0x18
        uint32_t m_Unk1C;       // 0x1C
        uint32_t m_Unk20;       // 0x20
        uint32_t m_Unk24;       // 0x24
        CRTTIField** m_Fields;  // 0x28
        uint32_t m_FieldCount;  // 0x30
        uint8_t pad_34[0x14];   // 0x34
        CRTTIMethod** m_Methods;// 0x48
        uint32_t m_MethodCount; // 0x50
        uint8_t pad_54[0x94];   // 0x54
        CRTTI* m_BaseClass;     // 0xE8

        const char* GetName() const {
            if (!m_pName || Utils::Memory::IsBadReadPtr(m_pName)) return "Unknown";
            return UntagPointer(reinterpret_cast<const char*>(*m_pName));
        }

        uint32_t GetSize() const { return m_Size; }

        CRTTIField* GetField(const char* name) const {
            if (!name) return nullptr;
            
            CRTTIField** list = UntagPointer(m_Fields);
            uint32_t count = 0;

            // Address-based heuristic: if list points to the count slot, it's inline (1 element)
            if (reinterpret_cast<uintptr_t>(list) == (reinterpret_cast<uintptr_t>(this) + 0x30)) {
                count = 1;
            } else {
                count = m_FieldCount;
            }

            if (!list || count == 0 || Utils::Memory::IsBadReadPtr(list)) goto check_base;

            for (uint32_t i = 0; i < count; ++i) {
                CRTTIField* f = UntagPointer(list[i]);
                if (f && !Utils::Memory::IsBadReadPtr(f)) {
                    if (strcmp(f->GetName(), name) == 0) return f;
                }
            }
            
        check_base:
            if (m_BaseClass) {
                CRTTI* pBase = UntagPointer(m_BaseClass);
                if (pBase && !Utils::Memory::IsBadReadPtr(pBase)) return pBase->GetField(name);
            }

            return nullptr;
        }

        CRTTIMethod* GetMethod(const char* name) const {
            if (!name) return nullptr;
            
            CRTTIMethod** list = UntagPointer(m_Methods);
            uint32_t count = 0;

            if (reinterpret_cast<uintptr_t>(list) == (reinterpret_cast<uintptr_t>(this) + 0x50)) {
                count = 1;
            } else {
                count = m_MethodCount;
            }

            if (!list || count == 0 || Utils::Memory::IsBadReadPtr(list)) goto check_base;

            for (uint32_t i = 0; i < count; ++i) {
                CRTTIMethod* m = UntagPointer(list[i]);
                if (m && !Utils::Memory::IsBadReadPtr(m)) {
                    if (strcmp(m->GetName(), name) == 0) return m;
                }
            }

        check_base:
            if (m_BaseClass) {
                CRTTI* pBase = UntagPointer(m_BaseClass);
                if (pBase && !Utils::Memory::IsBadReadPtr(pBase)) return pBase->GetMethod(name);
            }

            return nullptr;
        }
    };

    struct CRTTIObject {
        virtual ~CRTTIObject() = default; // Index 0
        virtual void UnkFunc1() = 0;      // Index 1
        virtual CRTTI* GetRTTI() const = 0; // Index 2
    };

    class DynamicObject {
    public:
        DynamicObject(void* instance) : m_Instance(instance), m_RTTI(nullptr) {
            if (m_Instance && !Utils::Memory::IsBadReadPtr(m_Instance)) {
                void** vtable = *reinterpret_cast<void***>(m_Instance);
                if (vtable && !Utils::Memory::IsBadReadPtr(vtable)) {
                    // Chrome Engine GetRTTI (index 2 or 3) returns rtti::Type*
                    // rtti::Type has CRTTI* at offset 0.
                    int indices[] = { 2, 3, 5, 6, 1 };
                    for (int idx : indices) {
                        __try {
                            typedef void** (__fastcall* GetRTTI_t)(void*);
                            GetRTTI_t GetRTTI_Fn = (GetRTTI_t)vtable[idx];
                            if (GetRTTI_Fn && !Utils::Memory::IsBadReadPtr(GetRTTI_Fn)) {
                                void** rttiType = GetRTTI_Fn(m_Instance);
                                if (rttiType && !Utils::Memory::IsBadReadPtr(rttiType)) {
                                    CRTTI* p = reinterpret_cast<CRTTI*>(*rttiType);
                                    if (p && !Utils::Memory::IsBadReadPtr(p)) {
                                        // Verify it's a CRTTI object
                                        static void* crttiVtable = nullptr;
                                        if (!crttiVtable) {
                                            HMODULE hEngine = GetModuleHandleA("engine_x64_rwdi.dll");
                                            if (hEngine) crttiVtable = (void*)((uintptr_t)hEngine + 0x1721c58);
                                        }
                                        if (*(void**)p == crttiVtable) {
                                            m_RTTI = p;
                                            break;
                                        }
                                    }
                                }
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                }
            }
        }

        DynamicObject(void* instance, CRTTI* rtti) : m_Instance(instance), m_RTTI(rtti) {}
        
        // Helper for when you know the class name
        static DynamicObject Create(void* instance, const char* className) {
            // We can't call RTTIManager::GetClass here due to circular header dependency
            // but we can provide a constructor that takes the CRTTI pointer directly.
            return DynamicObject(instance, nullptr); 
        }

        bool IsValid() const { return m_Instance != nullptr && m_RTTI != nullptr && !Utils::Memory::IsBadReadPtr(m_RTTI); }

        template <typename T>
        T* GetPropertyPtr(const char* name) {
            if (!IsValid()) return nullptr;
            CRTTIField* field = m_RTTI->GetField(name);
            if (!field) return nullptr;

            // Use the engine's virtual function to get the property pointer
            // Index 2 in CRTTIField vtable is the standard "GetPtr(instance)" method
            __try {
                void** vtable = *reinterpret_cast<void***>(field);
                typedef T* (__fastcall* GetPtr_t)(CRTTIField*, void*);
                GetPtr_t GetPtr_Fn = (GetPtr_t)vtable[2];
                
                if (GetPtr_Fn && !Utils::Memory::IsBadReadPtr(GetPtr_Fn)) {
                    return GetPtr_Fn(field, m_Instance);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}

            return nullptr;
        }

        template <typename T>
        T GetProperty(const char* name, T defaultValue = T()) {
            if (!IsValid()) return defaultValue;
            CRTTIField* field = m_RTTI->GetField(name);
            if (!field) return defaultValue;

            // 1. Try direct pointer access (works for Native fields)
            __try {
                void** vtable = *reinterpret_cast<void***>(field);
                typedef T* (__fastcall* GetPtr_t)(CRTTIField*, void*);
                GetPtr_t GetPtr_Fn = (GetPtr_t)vtable[2];
                if (GetPtr_Fn && !Utils::Memory::IsBadReadPtr(GetPtr_Fn)) {
                    T* ptr = GetPtr_Fn(field, m_Instance);
                    if (ptr && !Utils::Memory::IsBadReadPtr(ptr)) return *ptr;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}

            // 2. Fallback to ToString (works for ALL fields, including Virtual)
            // Signature for return-by-pointer: void __fastcall ToString(void* result, CRTTIField* this, void* instance)
            __try {
                void** vtable = *reinterpret_cast<void***>(field);
                typedef void* (__fastcall* ToString_t)(void* result, CRTTIField* field, void* instance);
                ToString_t ToString_Fn = (ToString_t)vtable[4];
                
                if (ToString_Fn && !Utils::Memory::IsBadReadPtr(ToString_Fn)) {
                    // Techland's ttl::string is a simple struct with a pointer
                    struct { void* ptr; size_t len; size_t cap; } tempStr = {0};
                    ToString_Fn(&tempStr, field, m_Instance);
                    
                    if (tempStr.ptr && !Utils::Memory::IsBadReadPtr(tempStr.ptr)) {
                        // For now we just log it to prove it works
                        // In a real SDK we'd parse the string back to T
                        // SPDLOG_INFO("ToString result: {}", (const char*)tempStr.ptr);
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}

            return defaultValue;
        }

        template <typename T>
        bool SetProperty(const char* name, const T& value) {
            if (!IsValid()) return false;
            CRTTIField* field = m_RTTI->GetField(name);
            if (!field) return false;

            // 1. Try direct pointer access
            __try {
                void** vtable = *reinterpret_cast<void***>(field);
                typedef T* (__fastcall* GetPtr_t)(CRTTIField*, void*);
                GetPtr_t GetPtr_Fn = (GetPtr_t)vtable[2];
                if (GetPtr_Fn && !Utils::Memory::IsBadReadPtr(GetPtr_Fn)) {
                    T* ptr = GetPtr_Fn(field, m_Instance);
                    if (ptr && !Utils::Memory::IsBadReadPtr(ptr)) {
                        *ptr = value;
                        return true;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}

            return false;
        }

        template <typename... Args>
        void CallMethod(const char* name, Args... args) {
            if (!IsValid()) return;
            CRTTIMethod* method = m_RTTI->GetMethod(name);
            if (!method) return;
            method->Invoke(reinterpret_cast<CRTTIObject*>(m_Instance), args...);
        }

    private:
        void* m_Instance;
        CRTTI* m_RTTI;
    };
}
