#include <EGT\Menu\VarList.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\Engine\CVars.h>

namespace EGT {
    namespace Menu {
        template <typename VarManagerT>
        VarList<VarManagerT>::VarList(const std::string& title) : listTitle(title) {}

        template <typename VarManagerT>
        void VarList<VarManagerT>::Render() {
            ImGui::BeginDisabled(!VarManagerT::AreAnyVarsPresent());
            if (ImGui::CollapsingHeader(listTitle.c_str(), ImGuiTreeNodeFlags_None)) {
                ImGui::Indent();

                //if (ImGui::Button("Save variables to file", "Saves current player variables to chosen file inside the file dialog"))
                //    ImGuiFileDialog::Instance()->OpenDialog("ChooseSCRPath", "Choose Folder", nullptr, { saveSCRPath.empty() ? "." : saveSCRPath });
                //ImGui::SameLine();
                //if (ImGui::Button("Load variables from file", "Loads player variables from chosen file inside the file dialog")) {
                //    std::filesystem::path _loadSCRFilePath = loadSCRFilePath;
                //    ImGuiFileDialog::Instance()->OpenDialog("ChooseSCRLoadPath", "Choose File", ".scr", { !_loadSCRFilePath.empty() && std::filesystem::is_directory(_loadSCRFilePath.parent_path()) ? _loadSCRFilePath.parent_path().string() : "." });
                //}

                ImGui::Checkbox("Restore variables to saved variables", &restoreVarsToSavedVarsEnabled, "Sets whether or not \"Restore variables to default\" should restore variables to the ones saved by \"Save current variables as default\"");
                //ImGui::Checkbox("Debug Mode", &debugEnabled, "Shows text boxes alongside player variables, which will show the address in memory of each variable");

                ImGui::BeginDisabled(!VarManagerT::AreAnyCustomVarsPresent() || VarManagerT::AreAllCustomVarsManagedByBool());
                if (ImGui::Button("Restore variables to default"))
                    RestoreVarsToDefault();
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Save current variables as default", "Saves the current variables as default for whenever you use \"Restore variables to default\""))
                    SaveVarsAsDefault();

                ImGui::Separator();
                ImGui::InputTextWithHint("##SearchFilter", "Search variables...", searchFilter, sizeof(searchFilter));

                UpdateFilteredList();

                ImGui::Text("Total listed variables: %zu", filteredVars.size());

                ImGuiListClipper clipper{};
                clipper.Begin(filteredVars.size());

                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                        RenderVar(filteredVars[i]);
                }

                ImGui::Unindent();
            }
            ImGui::EndDisabled();
        }

        template <typename VarManagerT>
        void VarList<VarManagerT>::RestoreVarToDefault(VarT* varPtr) {
            auto var = VarManagerT::GetVarRef(varPtr->GetName());
            if (!var)
                return;

            ImGui_impl::DeferredActions::Add([this, var]() mutable {
                switch (var->GetType()) {
                    case EGSDK::Engine::VarType::String:
                        break; // TO IMPLEMENT
                    case EGSDK::Engine::VarType::Float:
                        var->RestoreVarToDefault<float>(restoreVarsToSavedVarsEnabled);
                        break;
                    case EGSDK::Engine::VarType::Int:
                        var->RestoreVarToDefault<int>(restoreVarsToSavedVarsEnabled);
                        break;
                    case EGSDK::Engine::VarType::Vec3:
                        var->RestoreVarToDefault<vec3>(restoreVarsToSavedVarsEnabled);
                        break;
                    case EGSDK::Engine::VarType::Vec4:
                        var->RestoreVarToDefault<vec4>(restoreVarsToSavedVarsEnabled);
                        break;
                    case EGSDK::Engine::VarType::Bool:
                        var->RestoreVarToDefault<bool>(restoreVarsToSavedVarsEnabled);
                        break;
                    default:
                        break;
                }
            });
        }
        template <typename VarManagerT>
        void VarList<VarManagerT>::RestoreVarsToDefault() {
            (restoreVarsToSavedVarsEnabled ? VarManagerT::vars : VarManagerT::customVars).ForEach([this](VarPtr& varPtr) {
                RestoreVarToDefault(varPtr.get());
            });
        }
        template <typename VarManagerT>
        void VarList<VarManagerT>::SaveVarAsDefault(const VarPtr& varPtr) {
            auto var = VarManagerT::GetVarRefFromPtr(varPtr.get());
            if (!var)
                return;

            switch (var->GetType()) {
                case EGSDK::Engine::VarType::String:
                    break; // TO IMPLEMENT
                case EGSDK::Engine::VarType::Float:
                    var->SaveVarAsDefault<float>();
                    break;
                case EGSDK::Engine::VarType::Int:
                    var->SaveVarAsDefault<int>();
                    break;
                case EGSDK::Engine::VarType::Vec3:
                    var->SaveVarAsDefault<vec3>();
                    break;
                case EGSDK::Engine::VarType::Vec4:
                    var->SaveVarAsDefault<vec4>();
                    break;
                case EGSDK::Engine::VarType::Bool:
                    var->SaveVarAsDefault<bool>();
                    break;
                default:
                    break;
            }
        }
        template <typename VarManagerT>
        void VarList<VarManagerT>::SaveVarsAsDefault() {
            VarManagerT::vars.ForEach([this](VarPtr& varPtr) {
                SaveVarAsDefault(varPtr);
            });
            //ImGui::OpenPopup("Saved current player variables!");
        }

        template <typename VarManagerT>
        bool VarList<VarManagerT>::ShouldDisplayVar(const VarPtr& varPtr) {
            if (!searchFilter[0])
                return true;
            if (varPtr->GetType() == EGSDK::Engine::VarType::NONE)
                return false;

            // Convert searchFilter and variable name to lowercase
            std::string lowerFilter = EGSDK::Utils::Values::to_lower(searchFilter);
            std::string lowerKey = EGSDK::Utils::Values::to_lower(varPtr->GetName());
            return lowerKey.find(lowerFilter) != std::string::npos;
        }
        template <typename VarManagerT>
        void VarList<VarManagerT>::UpdateFilteredList() {
            if (filteredVars.empty() || _strcmpi(searchFilter, lastSearchFilter)) {
                strcpy_s(lastSearchFilter, searchFilter);
                filteredVars.clear();

                VarManagerT::vars.ForEach([this](VarPtr& varPtr) {
                    if (ShouldDisplayVar(varPtr))
                        filteredVars.push_back(varPtr.get());
                });
            }
        }

        template <typename VarManagerT>
        void VarList<VarManagerT>::RenderVar(VarT* varPtr) {
            auto var = VarManagerT::GetVarRefFromPtr(varPtr);
            if (!var)
                return;

            ImGui::BeginDisabled(var->IsManagedByBool());
            switch (var->GetType()) {
                case EGSDK::Engine::VarType::String:
                {
                    static char buffer[256];
                    //std::strncpy(buffer, value->c_str(), sizeof(buffer) - 1);
                    //buffer[sizeof(buffer) - 1] = '\0';

                    ImGui::BeginDisabled();
                    if (ImGui::InputText(var->GetName(), buffer, sizeof(buffer)))
                        var->SetValueFromList(std::string(buffer));
                    ImGui::EndDisabled();
                    ImGui::SetItemTooltip("STRING VARIABLES NOT IMPLEMENTED");
                    break; // TO IMPLEMENT
                }
                case EGSDK::Engine::VarType::Float:
                {
                    auto value = var->GetValue<float>();
                    if (!value) {
                        ImGui::EndDisabled();
                        return;
                    }
                    float newValue = *value;
                    if (ImGui::InputFloat(var->GetName(), &newValue))
                        var->SetValueFromList(newValue);
                    break;
                }
                case EGSDK::Engine::VarType::Int:
                {
                    auto value = var->GetValue<int>();
                    if (!value) {
                        ImGui::EndDisabled();
                        return;
                    }
                    auto newValue = *value;
                    if (ImGui::InputInt(var->GetName(), &newValue))
                        var->SetValueFromList(newValue);
                    break;
                }
                case EGSDK::Engine::VarType::Vec3:
                {
                    auto value = var->GetValue<vec3>();
                    if (!value) {
                        ImGui::EndDisabled();
                        return;
                    }
                    auto newValue = *value;
                    if (ImGui::InputFloat3(var->GetName(), reinterpret_cast<float*>(&newValue)))
                        var->SetValueFromList(newValue);
                    break;
                }
                case EGSDK::Engine::VarType::Vec4:
                {
                    auto value = var->GetValue<vec4>();
                    if (!value) {
                        ImGui::EndDisabled();
                        return;
                    }
                    auto newValue = *value;
                    if (ImGui::InputFloat4(var->GetName(), reinterpret_cast<float*>(&newValue)))
                        var->SetValueFromList(newValue);
                    break;
                }
                case EGSDK::Engine::VarType::Bool:
                {
                    auto value = var->GetValue<bool>();
                    if (!value) {
                        ImGui::EndDisabled();
                        return;
                    }
                    bool newValue = *value;
                    if (ImGui::Checkbox(var->GetName(), &newValue))
                        var->SetValueFromList(newValue);
                    break;
                }
                default:
                    break;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            std::string restoreBtnName = "Restore##" + std::string(var->GetName());

            ImGui::BeginDisabled(!var->HasCustomValue() || var->IsManagedByBool());
            if (ImGui::Button(restoreBtnName.c_str(), "Restores variable to default"))
                RestoreVarToDefault(varPtr);
            ImGui::EndDisabled();

            //if (debugEnabled)
            //    RenderDebugInfo(playerVarPtr);
        }

        template class VarList<EGSDK::GamePH::PlayerVariables>;
        template class VarList<EGSDK::Engine::CVars>;
    }
}
