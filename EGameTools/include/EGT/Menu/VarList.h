#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <ImGui\imguiex.h>
#include <EGSDK\Engine\VarBase.h>
#include <EGSDK\Engine\VarMapBase.h>
#include <EGSDK\Engine\VarManagerBase.h>
#include <EGT\ImGui_impl\DeferredActions.h>

namespace EGT {
    namespace Menu {
        template <typename VarManagerT>
        class VarList {
        public:
            using VarT = typename VarManagerT::VarT;
            using VarPtr = std::unique_ptr<VarT>;

            VarList(const std::string& title);
            VarList(const VarList&) = delete;
            VarList& operator=(const VarList&) = delete;
            VarList(VarList&&) noexcept = default;
            VarList& operator=(VarList&&) noexcept = default;

            void Render();
        private:
            std::string listTitle{};

            std::vector<VarT*> filteredVars{};
            bool restoreVarsToSavedVarsEnabled = false;
            char searchFilter[64] = "";
            char lastSearchFilter[64] = "";

            void RestoreVarToDefault(VarT* varPtr);
            void RestoreVarsToDefault();
            void SaveVarAsDefault(const VarPtr& varPtr);
            void SaveVarsAsDefault();

            bool ShouldDisplayVar(const VarPtr& varPtr);
            void UpdateFilteredList();

            void RenderVar(VarT* var);
        };
    }
}
