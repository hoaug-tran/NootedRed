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

    constexpr uint8_t  LinkRateRBR  = 0x06;
    constexpr uint8_t  LinkRateHBR  = 0x0A;
    constexpr uint8_t  LinkRateHBR2 = 0x14;
    constexpr uint8_t  LinkRateHBR3 = 0x1E;

    constexpr uint16_t TableRateRBR  = 1620000 / 200;
    constexpr uint16_t TableRateHBR  = 2700000 / 200;
    constexpr uint16_t TableRateHBR2 = 5400000 / 200;
    constexpr uint16_t TableRateHBR3 = 8100000 / 200;

    constexpr uint16_t HBR3TableRate  = TableRateHBR3;
    constexpr uint8_t  HBR3LegacyRate = LinkRateHBR3;

    inline uint8_t checksum(const uint8_t* bytes, size_t length)
    {
        uint8_t sum = 0;
        for (size_t i = 0; i < length; ++i) { sum = static_cast<uint8_t>(sum + bytes[i]); }
        return sum;
    }

    inline bool validEDIDContainer(const uint8_t* edid, size_t length)
    {
        constexpr uint8_t header[] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
        if (!edid || length < EDIDBlockSize || length % EDIDBlockSize) {
            return false;
        }
        for (size_t i = 0; i < sizeof(header); ++i) {
            if (edid[i] != header[i]) { return false; }
        }
        return checksum(edid, EDIDBlockSize) == 0;
    }

    inline bool sanitizeAndRepairEDID(uint8_t* edid, size_t length)
    {
        constexpr uint8_t header[] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
        if (!edid || length < EDIDBlockSize) { return false; }

        for (size_t i = 0; i < sizeof(header); ++i) {
            if (edid[i] != header[i]) { return false; }
        }

        bool modified = false;

        const uint8_t baseSum = checksum(edid, EDIDBlockSize);
        if (baseSum != 0) {
            edid[EDIDBlockSize - 1] = static_cast<uint8_t>(edid[EDIDBlockSize - 1] - baseSum);
            modified = true;
        }

        const size_t totalBlocks = length / EDIDBlockSize;
        const size_t maxAllowedExts = totalBlocks > 0 ? totalBlocks - 1 : 0;
        if (edid[126] > maxAllowedExts) {
            edid[126] = static_cast<uint8_t>(maxAllowedExts);
            edid[EDIDBlockSize - 1] = static_cast<uint8_t>(edid[EDIDBlockSize - 1] - checksum(edid, EDIDBlockSize));
            modified = true;
        }

        const size_t blockCount = static_cast<size_t>(edid[126]) + 1;

        for (size_t b = 1; b < blockCount && b < totalBlocks; ++b) {
            uint8_t* block = edid + (b * EDIDBlockSize);
            const uint8_t tag = block[0];

            if (tag == 0x70) {
                const uint8_t payloadBytes = block[2];
                constexpr size_t sectionHeader = 5;

                if (payloadBytes <= (EDIDBlockSize - sectionHeader - 2)) {
                    const size_t sectionEnd = sectionHeader + payloadBytes;

                    if (block[5] == 0x03 && block[6] == 0x01 && block[7] > 0) {
                        const size_t actualPayloadEnd = sectionHeader + 3 + block[7];
                        if (actualPayloadEnd < EDIDBlockSize - 1 && actualPayloadEnd < sectionEnd) {
                            for (size_t p = actualPayloadEnd + 1; p < EDIDBlockSize - 1; ++p) {
                                if (block[p] != 0) {
                                    block[p] = 0;
                                    modified = true;
                                }
                            }
                        }
                    }

                    const uint8_t secSum = checksum(block + 1, sectionEnd);
                    if (secSum != 0) {
                        block[sectionEnd] = static_cast<uint8_t>(block[sectionEnd] - secSum);
                        modified = true;
                    }
                }
            }

            const uint8_t extSum = checksum(block, EDIDBlockSize);
            if (extSum != 0) {
                block[EDIDBlockSize - 1] = static_cast<uint8_t>(block[EDIDBlockSize - 1] - extSum);
                modified = true;
            }
        }

        return modified || (checksum(edid, EDIDBlockSize) == 0);
    }

    inline bool repairDisplayID(uint8_t* edid, size_t length)
    {
        return sanitizeAndRepairEDID(edid, length);
    }

    inline bool disableEDIDVRR(uint8_t* edid, size_t length)
    {
        if (!edid || length < EDIDBlockSize) { return false; }
        bool modified = false;

        constexpr size_t descriptorOffsets[] = {54, 72, 90, 108};
        for (const auto offset : descriptorOffsets) {
            uint8_t* desc = edid + offset;
            if (desc[0] == 0x00 && desc[1] == 0x00 && desc[2] == 0x00 && desc[3] == 0xFD) {
                if (desc[5] != desc[6] && desc[6] > 0) {
                    desc[5] = desc[6];
                    modified = true;
                }
            }
        }

        if (modified) {
            const uint8_t baseSum = checksum(edid, EDIDBlockSize);
            edid[EDIDBlockSize - 1] = static_cast<uint8_t>(edid[EDIDBlockSize - 1] - baseSum);
        }

        return modified;
    }

    struct RateEvaluation
    {
        uint8_t  recommendedRate{0};
        uint16_t highestTableRate{0};
        bool     isTableSupported{false};
        bool     hasHBR3{false};
        bool     hasHBR2{false};
        bool     hasHBR{false};
    };

    inline RateEvaluation evaluateLinkRateTable(const uint8_t* table, size_t length)
    {
        RateEvaluation eval{};
        if (!table || length != LinkRateTableSize) { return eval; }

        for (size_t i = 0; i < length; i += 2) {
            const auto rate = static_cast<uint16_t>(uint16_t(table[i]) | (uint16_t(table[i + 1]) << 8));
            if (!rate) { continue; }
            eval.isTableSupported = true;
            if (rate > eval.highestTableRate) { eval.highestTableRate = rate; }
            if (rate >= TableRateHBR3) { eval.hasHBR3 = true; }
            else if (rate >= TableRateHBR2) { eval.hasHBR2 = true; }
            else if (rate >= TableRateHBR) { eval.hasHBR = true; }
        }

        if (eval.hasHBR3) { eval.recommendedRate = LinkRateHBR3; }
        else if (eval.hasHBR2) { eval.recommendedRate = LinkRateHBR2; }
        else if (eval.hasHBR) { eval.recommendedRate = LinkRateHBR; }
        else if (eval.highestTableRate >= TableRateRBR) { eval.recommendedRate = LinkRateRBR; }
        else if (eval.isTableSupported) { eval.recommendedRate = LinkRateHBR2; }

        return eval;
    }

    inline bool advertisesHBR3(uint8_t revision, const uint8_t* table, size_t length)
    {
        if (revision < 3 || !table || length != LinkRateTableSize) { return false; }
        const auto eval = evaluateLinkRateTable(table, length);
        return eval.hasHBR3;
    }

    inline bool normalizeReceiverRate(uint8_t* caps, size_t length, bool sourceSupportsHBR3, const RateEvaluation& eval)
    {
        if (!caps || length < 3 || caps[0] < 0x11) { return false; }

        const uint8_t originalRate = caps[1];
        uint8_t targetRate = originalRate;

        if (eval.isTableSupported && eval.recommendedRate != 0) {
            targetRate = eval.recommendedRate;
        } else if (originalRate == 0) {
            targetRate = LinkRateHBR2;
        }

        if (targetRate == LinkRateHBR3 && !sourceSupportsHBR3) {
            targetRate = LinkRateHBR2;
        }

        const uint8_t lanes = caps[2] & 0x1F;
        if (lanes == 0) {
            caps[2] = (caps[2] & 0xE0) | 0x04;
        }

        if (targetRate != 0 && (originalRate == 0 || targetRate > originalRate || (originalRate == LinkRateHBR3 && !sourceSupportsHBR3))) {
            caps[1] = targetRate;
            return true;
        }

        return false;
    }

    inline bool normalizeReceiverRate(uint8_t* caps, size_t length, bool sourceHBR3, bool sinkHBR3)
    {
        RateEvaluation eval{};
        eval.isTableSupported = sinkHBR3;
        eval.hasHBR3 = sinkHBR3;
        eval.recommendedRate = sinkHBR3 ? LinkRateHBR3 : LinkRateHBR2;
        return normalizeReceiverRate(caps, length, sourceHBR3, eval);
    }

    enum class BrightnessResult
    {
        Unsupported,
        Failed,
        Success
    };

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
