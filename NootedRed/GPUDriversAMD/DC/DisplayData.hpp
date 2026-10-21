// Display protocol validation and conversions. No panel identity or driver ABI assumptions.
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
#include <stddef.h>
#include <stdint.h>

namespace DisplayData
{

    constexpr size_t   EDIDBlockSize = 128;
    constexpr uint32_t ReceiverCaps = 0x000, ExtendedReceiverCaps = 0x2200;
    constexpr uint32_t SupportedLinkRates = 0x010, EDPRevision = 0x700;
    constexpr uint32_t BacklightCaps = 0x701, BacklightControl = 0x721, BacklightLevel = 0x722;
    constexpr size_t   LinkRateTableSize = 16;
    constexpr uint16_t HBR3TableRate     = 8100000 / 200;    // kHz / 200 kHz, eDP table units.
    constexpr uint8_t  HBR3LegacyRate    = 0x1E;

    inline uint8_t checksum(const uint8_t* bytes, size_t length)
    {
        uint8_t sum = 0;
        for (size_t i = 0; i < length; ++i) { sum = static_cast<uint8_t>(sum + bytes[i]); }
        return sum;
    }

    inline bool validEDIDContainer(const uint8_t* edid, size_t length)
    {
        constexpr uint8_t header[] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
        if (!edid || length < EDIDBlockSize || length % EDIDBlockSize
            || length / EDIDBlockSize != size_t(edid[126]) + 1)
        {
            return false;
        }
        for (size_t i = 0; i < sizeof(header); ++i) {
            if (edid[i] != header[i]) { return false; }
        }
        return checksum(edid, EDIDBlockSize) == 0;
    }

    // Explicit recovery only: a full-length DisplayID 1.3 section containing one Type-I timing
    // block followed by a zero block header. The caller must opt in: damaged padding cannot be
    // distinguished from lost data automatically. Never synthesize a timing or remove CTA data.
    inline bool repairDisplayID(uint8_t* edid, size_t length)
    {
        if (!validEDIDContainer(edid, length)) { return false; }
        constexpr size_t  sectionHeader = 5, blockHeader = 3, timingSize = 20;
        constexpr uint8_t maxPayload = EDIDBlockSize - sectionHeader - 2;
        size_t            candidate = 0, payloadEnd = 0;
        for (size_t offset = EDIDBlockSize; offset < length; offset += EDIDBlockSize) {
            const auto*  block     = edid + offset;
            const bool   displayID = block[0] == 0x70;
            const size_t end       = sectionHeader + block[2];
            const bool   valid     = checksum(block, EDIDBlockSize) == 0
                                     && (!displayID || (end < EDIDBlockSize - 1 && checksum(block + 1, end) == 0));
            if (valid) { continue; }
            if (candidate || !displayID || block[1] != 0x13 || block[2] != maxPayload || block[4] != 0
                || block[5] != 0x03 || block[6] != 0x01 || !block[7] || block[7] % timingSize)
            {
                return false;
            }
            payloadEnd = sectionHeader + blockHeader + block[7];
            if (payloadEnd + blockHeader >= EDIDBlockSize - 2) { return false; }
            if (block[payloadEnd] || block[payloadEnd + 1] || block[payloadEnd + 2]) { return false; }
            candidate = offset;
        }
        if (!candidate) { return false; }
        auto* block = edid + candidate;
        block[2]    = static_cast<uint8_t>(payloadEnd - sectionHeader);
        for (size_t i = payloadEnd; i < EDIDBlockSize; ++i) { block[i] = 0; }
        block[payloadEnd]        = static_cast<uint8_t>(0 - checksum(block + 1, payloadEnd - 1));
        block[EDIDBlockSize - 1] = static_cast<uint8_t>(0 - checksum(block, EDIDBlockSize - 1));
        return true;
    }

    inline bool advertisesHBR3(uint8_t revision, const uint8_t* table, size_t length)
    {
        if (revision < 3 || !table || length != LinkRateTableSize) { return false; }
        bool     found = false, terminated = false;
        uint16_t previous = 0;
        for (size_t i = 0; i < length; i += 2) {
            const auto rate = static_cast<uint16_t>(uint16_t(table[i]) | uint16_t(table[i + 1]) << 8);
            if (!rate) {
                terminated = true;
                continue;
            }
            if (terminated || rate <= previous) { return false; }
            previous  = rate;
            found    |= rate == HBR3TableRate;
        }
        return found;
    }

    inline bool normalizeReceiverRate(uint8_t* caps, size_t length, bool sourceHBR3, bool sinkHBR3)
    {
        if (!caps || length < 3 || !sourceHBR3 || !sinkHBR3 || caps[0] < 0x13 || caps[1]) { return false; }
        const auto lanes = caps[2] & 0x1F;
        if (lanes != 1 && lanes != 2 && lanes != 4) { return false; }
        caps[1] = HBR3LegacyRate;
        return true;
    }

    enum class BrightnessResult
    {
        Unsupported,
        Failed,
        Success
    };

    // Read/write use the caller's live link. No cached AUX service, capability, brightness range or port.
    template<typename Read, typename Write>
    BrightnessResult setSDRBacklight(Read read, Write write, uint32_t value, uint32_t maximum)
    {
        if (!maximum) { return BrightnessResult::Unsupported; }
        uint8_t caps[2]{}, control[4]{};
        if (!read(BacklightCaps, caps, sizeof(caps)) || !read(BacklightControl, control, sizeof(control))) {
            return BrightnessResult::Unsupported;
        }
        constexpr uint8_t tconAdjustment = 1, auxSetting = 2, twoByteSetting = 4, modeMask = 3, dpcdMode = 2;
        constexpr uint8_t luminanceControl = 1U << 7;
        if (!(caps[0] & tconAdjustment) || !(caps[1] & auxSetting) || (control[0] & modeMask) != dpcdMode
            || (control[0] & luminanceControl))
        {
            return BrightnessResult::Unsupported;
        }
        const bool twoBytes = (caps[1] & twoByteSetting) != 0;
        const auto bits     = control[3] & 0x1F;
        if (!bits || bits > (twoBytes ? 16 : 8)) { return BrightnessResult::Unsupported; }
        const uint64_t bounded   = value > maximum ? maximum : value;
        const auto     level     = static_cast<uint32_t>(bounded * ((1U << bits) - 1) / maximum);
        uint8_t        payload[] = {static_cast<uint8_t>(level >> 8), static_cast<uint8_t>(level)};
        if (!twoBytes) { payload[0] = static_cast<uint8_t>(level); }
        const size_t size = twoBytes ? 2 : 1;
        if (!write(BacklightLevel, payload, size)) { return BrightnessResult::Failed; }
        uint8_t actual[2]{};
        if (!read(BacklightLevel, actual, size)) { return BrightnessResult::Failed; }
        for (size_t i = 0; i < size; ++i) {
            if (actual[i] != payload[i]) { return BrightnessResult::Failed; }
        }
        return BrightnessResult::Success;
    }

}    // namespace DisplayData
