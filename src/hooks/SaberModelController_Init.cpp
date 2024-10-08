#include "FingerSaber.hpp"
#include "main.hpp"
#include "logging.hpp"
#include "ModConfig.hpp"

#include "GlobalNamespace/SaberModelController.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/SaberType.hpp"
#include "GlobalNamespace/PauseController.hpp"
#include "System/Action.hpp"

#include "UnityEngine/Color.hpp"
#include "UnityEngine/Material.hpp"
#include "GlobalNamespace/ColorManager.hpp"

#include "math.hpp"
static int handInitCount = 0; // Hands always initialized as pair i.e. twice(even when playing one handed mode)

MAKE_HOOK_MATCH(
    SaberModelController_Init,
    &GlobalNamespace::SaberModelController::Init,
    void,
    GlobalNamespace::SaberModelController* self,
    UnityEngine::Transform* parent,
    GlobalNamespace::Saber* saber
) {

    INFO("SaberModelController::Init()");

    SaberModelController_Init(self, parent, saber);

    // Exiting early if sabers are not players. Needed for multiplayer.
    if (parent->get_parent()->get_parent()->get_name() != "VRGameCore")
        return;

    if( modManager.is_scene_GameCore == true ){
        modManager.is_GamePaused = false; // Game is never paused when saber is inited

        // Still color the hands even if mod is not enabled.
        if(saber->get_saberType() == GlobalNamespace::SaberType::SaberB){
            modManager.ChangeRightSkeletonRendererColor(self->_colorManager->ColorForSaberType(GlobalNamespace::SaberType::SaberB));
            modManager.local_player_saber_r = saber;
        }
        if(saber->get_saberType() == GlobalNamespace::SaberType::SaberA){
            modManager.ChangeLeftSkeletonRendererColor(self->_colorManager->ColorForSaberType(GlobalNamespace::SaberType::SaberA));
            modManager.local_player_saber_l = saber;
        }

        if (getModConfig().ModEnabled.GetValue() == true)
        {
            if(getModConfig().HandMode.GetValue() == false){
                float sScale = 1.0f/6.5f;
                saber->get_transform()->set_localScale(UnityEngine::Vector3{sScale,sScale,sScale});
            }

            float pScale = 6.5f;

            handInitCount += 1;
            // Figured this is safer way to know when both sabers exist, as opposed to assuming which saber is last to get initialized.
            // (I assume its not always left or right saber that gets initialized last.)
            if (handInitCount%2 == 0){
                auto VRGameCore = UnityEngine::GameObject::Find(("Origin/VRGameCore"));

                if (!VRGameCore) {
                    INFO("VRGameCore not found");
                    return;
                }

                if(VRGameCore){
                    modManager.handTrackingObjectsParent->get_transform()->set_position(VRGameCore->get_transform()->get_position());

                    auto PauseController_go = UnityEngine::GameObject::Find(("PauseController"));
                    if(PauseController_go) modManager.pauseController = PauseController_go->GetComponent<GlobalNamespace::PauseController*>();

                    if(getModConfig().HandMode.GetValue() == false){
                        UnityEngine::Vector3 scaler{pScale,pScale,pScale};

                        VRGameCore->get_transform()->set_localScale(scaler);
                        modManager.handTrackingObjectsParent->get_transform()->set_localScale(scaler);

                        auto mainCamera = VRGameCore->Find(("MainCamera"));
                        float headLevel = mainCamera->get_transform()->get_position().y;
                        float platformLevel = headLevel + pScale * getModConfig().PlatformHeightOffsetMeters.GetValue();
                        float platformZOffset = pScale * getModConfig().PlatformDistanceOffsetMeters.GetValue();

                        auto posBody = VRGameCore->get_transform()->get_position();
                        posBody = posBody + UnityEngine::Vector3{0,-platformLevel, platformZOffset};
                        VRGameCore->get_transform()->set_position(posBody);

                        auto posHands = modManager.handTrackingObjectsParent->get_transform()->get_position();
                        posHands = posHands + UnityEngine::Vector3{0,-platformLevel, platformZOffset};
                        modManager.handTrackingObjectsParent->get_transform()->set_position(posHands);
                    }
                    // Set handtracking under player so that it is moved properly
                    // when using NoodleExtension.
                    modManager.handTrackingObjectsParent->get_transform()->set_parent(VRGameCore->get_transform());
                }
            }
        }
    }

}

void FingerSaber::_Hook_SaberModelController_Init(){

    INSTALL_HOOK(Logger, SaberModelController_Init);
}