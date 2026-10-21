// Framebuffer display protocol adapter. ABI-specific access stays in this module.
//
// Copyright (C) 2022-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <GPUDriversAMD/DC/DisplayData.hpp>
#include <GPUDriversAMD/FB/DisplayABI.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/graphics/IOGraphicsTypes.h>

class DisplayLink
{
    using GetLink      = void*        (*)(void*, UInt32);
    using DPCDTransfer = UInt32  (*)(void*, UInt32, UInt8*, UInt32);
    using CurrentMode  = IOReturn (*)(void*, IODisplayModeID*, IOIndex*);
    using Timing       = IOReturn      (*)(void*, IODisplayModeID, IOTimingInformation*);

    mach_vm_address_t orgEDIDQuery{0}, orgAUXTransfer{0};
    GetLink           getLink{nullptr};
    DPCDTransfer      readDPCD{nullptr}, writeDPCD{nullptr};
    CurrentMode       currentMode{nullptr};
    Timing            timing{nullptr};

    static UInt32 wrapEDIDQuery(void* ddc);
    static UInt32 wrapAUXTransfer(void* self, DisplayABI::AUXRequest* request, size_t length);

public:
    static DisplayLink& singleton();
    void                processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);
    bool queryTiming(void* framebuffer, IODisplayModeID& mode, IOIndex& depth, IOTimingInformation& info) const;
    DisplayData::BrightnessResult setSDRBacklight(void* link, UInt32 value, UInt32 maximum) const;
};
