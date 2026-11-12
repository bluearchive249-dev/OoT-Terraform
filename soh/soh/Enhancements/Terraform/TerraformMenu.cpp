// Copyright (c) 2026 Grimsfrost. All rights reserved. No copying, editing or additions, by people or AI tools, without the author's written consent. See AI-POLICY.md.
// Terraform (for Ship of Harkinian): its menu. Hold L and press R to open it: the world stops, the camera rises to look
// down from overhead, and the player chooses what to build or sculpt (see Terraform.cpp).
//   Stick or D-pad up/down: choose   A: pick   B (or L+R again): close   (the mouse works too)
// Off with the console variable gEnhancements.Terraform.Enabled set to 0.

#include <algorithm>
#include <string>

#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/GuiWindow.h>
#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#include "Terraform.h"

namespace {

const char* sExtra[4] = { "Sculpt the ground (A raise, Z lower)", "Remove the nearest piece",
                          "Clear everything built in this area", "Put this area's ground back as it was" };

bool sOpen = false;
int sSel = 0;
bool sScrollToSel = false;
int sStickHeld = 0;
int sStickDir = 0;
std::string sLast;
f32 sCamBlend = 0.0f; // 0 the game's camera .. 1 overhead
Vec3f sCamEye;        // where the game's camera stood as the menu opened
Vec3f sCamAt;

int EntryCount() {
    return Terraform_KindCount() + 4;
}

const char* EntryName(int i) {
    return (i < Terraform_KindCount()) ? Terraform_KindName(i) : sExtra[i - Terraform_KindCount()];
}

void Sfx(u16 id) {
    Audio_PlaySfxGeneral(id, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                         &gSfxDefaultReverb);
}

bool Allowed() {
    return (gPlayState != nullptr) && CVarGetInteger(CVAR_ENHANCEMENT("Terraform.Enabled"), 1);
}

void SetOpen(bool open) {
    PlayState* play = gPlayState;

    if (!open) {
        Terraform_Cancel(); // (a piece still being placed goes with the menu)
    }
    sOpen = open;
    if (play != nullptr) {
        // The world holds still while the menu is up (the game's own frame-advance pause)
        play->frameAdvCtx.enabled = open;
        play->frameAdvCtx.timer = 0;
        if (open) {
            sCamEye = play->view.eye;
            sCamAt = play->view.lookAt;
        }
    }
    sCamBlend = 0.0f;
    sScrollToSel = true;
    Sfx(open ? NA_SE_SY_FSEL_DECIDE_L : NA_SE_SY_FSEL_CLOSE);
}

void Pick(int i) {
    int kinds = Terraform_KindCount();

    if (i < kinds) {
        if (!Terraform_Begin(i)) {
            sLast = "No more pieces fit in this area (" + std::to_string(Terraform_MaxHere()) + ")";
        }
    } else if (i == kinds) {
        Terraform_BeginSculpt();
    } else if (i == kinds + 1) {
        sLast = Terraform_RemoveNearest() ? "Removed the nearest piece" : "Nothing built here";
    } else if (i == kinds + 2) {
        sLast = "Cleared " + std::to_string(Terraform_ClearHere()) + " pieces";
    } else {
        sLast = Terraform_RestoreGroundHere() ? "The ground is as it was" : "Nothing sculpted here";
    }
}

// The menu's camera: high above the player (or the piece being placed), looking down, eased in from where the game's
// camera stood. The world is still, so nothing else moves the view meanwhile.
void OverheadCamera(PlayState* play) {
    Player* player = GET_PLAYER(play);
    s16 yaw = player->actor.shape.rot.y;
    Vec3f focus = player->actor.world.pos;
    Vec3f at;
    Vec3f eye;
    Vec3f up;
    f32 t;

    if (Terraform_Focus(&focus)) {
        at = focus;
    } else {
        at = focus;
        at.x += Math_SinS(yaw) * 70.0f;
        at.z += Math_CosS(yaw) * 70.0f;
    }
    eye.x = focus.x - Math_SinS(yaw) * 160.0f;
    eye.y = focus.y + 620.0f;
    eye.z = focus.z - Math_CosS(yaw) * 160.0f;

    Math_ApproachF(&sCamBlend, 1.0f, 1.0f, 0.07f);
    t = sCamBlend * sCamBlend * (3.0f - 2.0f * sCamBlend);
    eye.x = sCamEye.x + (eye.x - sCamEye.x) * t;
    eye.y = sCamEye.y + (eye.y - sCamEye.y) * t;
    eye.z = sCamEye.z + (eye.z - sCamEye.z) * t;
    at.x = sCamAt.x + (at.x - sCamAt.x) * t;
    at.y = sCamAt.y + (at.y - sCamAt.y) * t;
    at.z = sCamAt.z + (at.z - sCamAt.z) * t;
    // Up is the player's facing, tipped off the line of sight
    up = { Math_SinS(yaw), 0.0f, Math_CosS(yaw) };
    Vec3f d = { at.x - eye.x, at.y - eye.y, at.z - eye.z };
    f32 len = sqrtf(SQ(d.x) + SQ(d.y) + SQ(d.z));
    if (len > 1.0f) {
        f32 dot = (up.x * d.x + up.z * d.z) / len;
        up.x -= d.x / len * dot;
        up.y -= d.y / len * dot;
        up.z -= d.z / len * dot;
    }
    if (SQ(up.x) + SQ(up.y) + SQ(up.z) < 0.0001f) {
        up = { 0.0f, 1.0f, 0.0f };
    }
    func_800AA358(&play->view, &eye, &at, &up);
    if (play->skyboxId && (play->skyboxId != SKYBOX_UNSET_1D) && !play->envCtx.skyboxDisabled) {
        SkyboxDraw_UpdateMatrix(&play->skyboxCtx, eye.x, eye.y, eye.z);
    }
}

void OnFrame() {
    PlayState* play = gPlayState;

    if (play == nullptr) {
        sOpen = false;
        return;
    }
    Input* input = &play->state.input[0];

    if (!sOpen) {
        if (CHECK_BTN_ALL(input->cur.button, BTN_L) && CHECK_BTN_ALL(input->press.button, BTN_R) && Allowed() &&
            (play->pauseCtx.state == 0) && (play->msgCtx.msgMode == MSGMODE_NONE) &&
            (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->csCtx.state == CS_STATE_IDLE)) {
            SetOpen(true);
        }
        return;
    }
    if (!Allowed() || (play->transitionTrigger != TRANS_TRIGGER_OFF)) {
        SetOpen(false);
        return;
    }
    play->frameAdvCtx.enabled = true;
    if (Terraform_IsPlacing()) {
        // Setting a piece down or sculpting: the controls are its until it's done
        if (!Terraform_PlacingFrame(input)) {
            sLast = "Built: " + std::to_string(Terraform_CountHere()) + " of " + std::to_string(Terraform_MaxHere()) +
                    " pieces in this area";
        }
        OverheadCamera(play);
        return;
    }
    OverheadCamera(play);

    if (CHECK_BTN_ALL(input->press.button, BTN_B) ||
        (CHECK_BTN_ALL(input->cur.button, BTN_L) && CHECK_BTN_ALL(input->press.button, BTN_R))) {
        SetOpen(false);
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
        Pick(sSel);
        return;
    }
    // Stick (with repeat while held) and D-pad
    int dir = 0;
    s8 sy = input->rel.stick_y;
    bool step = false;

    if (ABS(sy) > 40) {
        dir = (sy > 0) ? 1 : 2;
        if (dir != sStickDir) {
            sStickHeld = 0;
            step = true;
        } else if ((++sStickHeld > 14) && ((sStickHeld % 3) == 0)) {
            step = true;
        }
    }
    sStickDir = dir;
    if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
        dir = 1, step = true;
    } else if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN)) {
        dir = 2, step = true;
    }
    if (step) {
        sSel = (sSel + ((dir == 1) ? -1 : 1) + EntryCount()) % EntryCount();
        sScrollToSel = true;
        Sfx(NA_SE_SY_CURSOR);
    }
}

class TerraformWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void InitElement() override {
    }
    void DrawElement() override {
    }
    void UpdateElement() override {
    }
    void Draw() override;
};

void TerraformWindow::Draw() {
    if (!sOpen || (gPlayState == nullptr)) {
        return;
    }
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float scale = std::max(vp->Size.y / 720.0f, 1.0f);

    if (Terraform_IsPlacing()) {
        // The menu out of the way, the controls along the bottom
        ImDrawList* list = ImGui::GetForegroundDrawList(vp);
        const char* texName = Terraform_PlacingTexName();
        std::string piece = std::string(texName != nullptr ? std::string("Texture: ") + texName + "  (L: next)    " : "") +
                            "Stick: move    C-Left/Right: turn    C-Up/Down: raise/lower (R held: taller/shorter)    "
                            "D-pad: longer/shorter, wider/narrower    Z: onto the floor    A: set down    B: cancel";
        std::string sculpt = std::string(Terraform_SculptInfo()) +
                             "    Stick: move the brush    A (hold): raise    Z (hold): lower    D-pad: brush size    B: done";
        const char* help = Terraform_IsSculpting()
                               ? sculpt.c_str()
                               : piece.c_str();
        float fontSize = ImGui::GetFontSize() * scale;
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fontSize, vp->Size.x * 0.9f, vp->Size.x * 0.9f, help);
        ImVec2 p(vp->Pos.x + (vp->Size.x - ts.x) * 0.5f, vp->Pos.y + vp->Size.y - ts.y - 24.0f);

        list->AddRectFilled(ImVec2(p.x - 10.0f, p.y - 6.0f), ImVec2(p.x + ts.x + 10.0f, p.y + ts.y + 6.0f),
                            IM_COL32(12, 22, 16, 215), 6.0f);
        list->AddText(ImGui::GetFont(), fontSize, p, IM_COL32(200, 240, 170, 255), help, nullptr, vp->Size.x * 0.9f);
        return;
    }

    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400.0f * scale, 400.0f * scale), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.09f, 0.06f, 0.88f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.55f, 0.8f, 0.4f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.45f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.38f, 0.22f, 0.7f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::Begin("Terraform##TerraformMenu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
    ImGui::SetWindowFontScale(scale);

    ImGui::TextColored(ImVec4(0.75f, 0.95f, 0.6f, 1.0f), "Terraform");
    ImGui::SameLine();
    ImGui::TextDisabled("  A pick   B close");
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.65f, 1.0f), "Built here: %d of %d. Kept with your save.",
                       Terraform_CountHere(), Terraform_MaxHere());

    float bottom = ImGui::GetTextLineHeightWithSpacing() * 1.4f;
    ImGui::BeginChild("##list", ImVec2(0, -bottom), true);
    for (int i = 0; i < EntryCount(); i++) {
        bool sel = (i == sSel);

        if (i == Terraform_KindCount()) {
            ImGui::Separator();
        }
        ImGui::PushID(i);
        if (ImGui::Selectable(EntryName(i), sel)) {
            sSel = i;
            Pick(i);
        }
        if (sel && sScrollToSel) {
            ImGui::SetScrollHereY(0.5f);
        }
        ImGui::PopID();
    }
    sScrollToSel = false;
    ImGui::EndChild();
    ImGui::TextDisabled("%s", sLast.c_str());

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

void RegisterTerraformMenu() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(OnFrame);
}

static RegisterShipInitFunc initFunc(RegisterTerraformMenu, {});

} // namespace

std::shared_ptr<Ship::GuiWindow> Terraform_CreateWindow() {
    return std::make_shared<TerraformWindow>(CVAR_WINDOW("Terraform"), "Terraform");
}
