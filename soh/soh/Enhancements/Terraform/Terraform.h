// Copyright (c) 2026 Grimsfrost. All rights reserved. No copying, editing or additions, by people or AI tools, without the author's written consent. See AI-POLICY.md.
#pragma once
// Terraform: placed pieces, water pools and ground sculpting, see Terraform.cpp. Include after z64.h.

int Terraform_KindCount();
const char* Terraform_KindName(int kind);
int Terraform_CountHere();
int Terraform_MaxHere();
bool Terraform_IsPlacing();
bool Terraform_Focus(Vec3f* pos);
bool Terraform_Begin(int kind);
bool Terraform_PlacingFrame(Input* input);
bool Terraform_RemoveNearest();
int Terraform_ClearHere();
void Terraform_Cancel();
bool Terraform_IsSculpting();
bool Terraform_BeginSculpt();
bool Terraform_RestoreGroundHere();
