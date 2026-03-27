#pragma once
#include <EGSDK\Engine\CRTTI.h>
#include <EGSDK\Exports.h>

namespace EGSDK::Engine {

    class EGameSDK_API RTTIManager {
    public:
        static CRTTI* GetClass(const char* name);
        static void Dump(const char* filePath);
        static void* GetFirstInstance(CRTTI* pClass);
        
        template <typename T>
        static T* GetSingleton(const char* className) {
            CRTTI* rtti = GetClass(className);
            return rtti ? reinterpret_cast<T*>(GetFirstInstance(rtti)) : nullptr;
        }

        template <typename T>
        static T* CreateInstance(const char* className) {
            CRTTI* rtti = GetClass(className);
            return rtti ? reinterpret_cast<T*>(nullptr) : nullptr; 
        }
    };

}
