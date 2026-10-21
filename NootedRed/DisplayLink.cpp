// DisplayID recovery, eDP rate-table compatibility and standard AUX backlight transport.
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

#include <CompatTarget.hpp>
#include <DisplayLink.hpp>
#include <GPUDriversAMD/DC/SignalTypes.hpp>
#include <Headers/kern_util.hpp>
#include <Kexts.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>

namespace
{

    template<size_t N>
    mach_vm_address_t findUnique(const UInt8 (&pattern)[N], mach_vm_address_t slide, size_t size)
    {
        const auto* image  = reinterpret_cast<const UInt8*>(slide);
        size_t      offset = 0, duplicate = 0;
        if (!KernelPatcher::findPattern(pattern, nullptr, N, image, size, &offset)) { return 0; }
        if (KernelPatcher::findPattern(pattern, nullptr, N, image + offset + 1, size - offset - 1, &duplicate)) {
            return 0;
        }
        return slide + offset;
    }

}    // namespace

static DisplayLink moduleInstance;
DisplayLink&       DisplayLink::singleton() { return moduleInstance; }

void DisplayLink::processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size)
{
    if (kextRadeonX6000Framebuffer.loadIndex != id || checkKernelArgument("-NRedNoDisplayFix")) { return; }
    CompatTarget::Identity identity;
    if (!CompatTarget::parseMachOIdentity(identity, reinterpret_cast<const UInt8*>(slide), size)
        || memcmp(identity.uuid, DisplayABI::FramebufferUUID, sizeof(identity.uuid)))
    {
        SYSLOG("DisplayLink", "Unknown framebuffer ABI; keeping native display protocol handling");
        return;
    }

    // Resolve before routing. Helpers are independent: an unavailable EDID hook never disables AUX.
    this->currentMode = reinterpret_cast<CurrentMode>(findUnique(DisplayABI::CurrentMode, slide, size));
    this->timing      = reinterpret_cast<Timing>(findUnique(DisplayABI::Timing, slide, size));
    this->readDPCD    = reinterpret_cast<DPCDTransfer>(findUnique(DisplayABI::ReadDPCD, slide, size));
    this->writeDPCD   = reinterpret_cast<DPCDTransfer>(findUnique(DisplayABI::WriteDPCD, slide, size));
    if (checkKernelArgument("-NRedRepairDisplayID")) {
        const auto address = findUnique(DisplayABI::EDIDQuery, slide, size);
        if (address) {
            KernelPatcher::RouteRequest request{nullptr, wrapEDIDQuery, this->orgEDIDQuery};
            request.from = address;
            SYSLOG_COND(!patcher.routeMultiple(id, &request, 1, slide, size), "DisplayLink", "EDID hook unavailable");
            patcher.clearError();
        }
    }
    PenguinWizardry::PatternSolveRequest link{"__ZNK30AMDRadeonX6000_AmdAgdcServices7getLinkEj", this->getLink};
    if (!link.solve(patcher, id, slide, size)) {
        patcher.clearError();
        return;
    }
    PenguinWizardry::PatternRouteRequest aux{
        "__ZN30AMDRadeonX6000_AmdAgdcServices17auxStreamTransferEP21AGDCStreamAccessAUX_tm", wrapAUXTransfer,
        this->orgAUXTransfer};
    SYSLOG_COND(!aux.route(patcher, id, slide, size), "DisplayLink", "AUX hook unavailable");
    patcher.clearError();
}

UInt32 DisplayLink::wrapEDIDQuery(void* ddc)
{
    auto&      mod   = singleton();
    const auto bytes = FunctionCast(wrapEDIDQuery, mod.orgEDIDQuery)(ddc);
    if (ddc && bytes == getMember<UInt32>(ddc, DisplayABI::EDIDLength)
        && DisplayData::repairDisplayID(static_cast<UInt8*>(ddc) + DisplayABI::EDIDBuffer, bytes))
    {
        SYSLOG("DisplayLink", "Recovered malformed DisplayID section; original timings preserved");
    }
    return bytes;
}

UInt32 DisplayLink::wrapAUXTransfer(void* self, DisplayABI::AUXRequest* request, size_t length)
{
    auto&      mod    = singleton();
    const auto result = FunctionCast(wrapAUXTransfer, mod.orgAUXTransfer)(self, request, length);
    if (result || !self || !request || length != sizeof(*request) || !request->port || request->write || request->status
        || request->length < 3 || request->length > sizeof(request->data)
        || (request->address != DisplayData::ReceiverCaps && request->address != DisplayData::ExtendedReceiverCaps)
        || request->data[0] < 0x13 || request->data[1])
    {
        return result;
    }
    auto* link = mod.getLink(self, request->port);
    if (!link || getMember<UInt32>(link, DisplayABI::LinkSignal) != SIGNAL_TYPE_EDP
        || !getMember<UInt8>(link, DisplayABI::LinkActive)
        || !(getMember<UInt16>(link, DisplayABI::LinkCapabilities) & DisplayABI::SourceHBR3))
    {
        return result;
    }

    // Re-probe the requesting port, never a fixed port or a cached panel-wide flag. Original calls
    // avoid recursion and leave failure/retry semantics with the native AUX implementation.
    DisplayABI::AUXRequest version{}, table{};
    version.port = table.port = request->port;
    version.address           = DisplayData::EDPRevision;
    version.length            = 1;
    table.address             = DisplayData::SupportedLinkRates;
    table.length              = DisplayData::LinkRateTableSize;
    const auto vr             = FunctionCast(wrapAUXTransfer, mod.orgAUXTransfer)(self, &version, sizeof(version));
    const auto tr             = FunctionCast(wrapAUXTransfer, mod.orgAUXTransfer)(self, &table, sizeof(table));
    const bool verified       = !vr && !tr && !version.status && !table.status
                                && DisplayData::advertisesHBR3(version.data[0], table.data, table.length);
    if (DisplayData::normalizeReceiverRate(request->data, request->length, true, verified)) {
        DBGLOG("DisplayLink", "Normalized receiver rate from live eDP HBR3 table (port %u)", request->port);
    }
    return result;
}

bool DisplayLink::queryTiming(void* framebuffer, IODisplayModeID& mode, IOIndex& depth, IOTimingInformation& info) const
{
    return framebuffer && this->currentMode && this->timing
           && this->currentMode(framebuffer, &mode, &depth) == kIOReturnSuccess
           && this->timing(framebuffer, mode, &info) == kIOReturnSuccess && (info.flags & kIODetailedTimingValid);
}

DisplayData::BrightnessResult DisplayLink::setSDRBacklight(void* link, UInt32 value, UInt32 maximum) const
{
    if (!link || !this->readDPCD || !this->writeDPCD
        || getMember<UInt32>(link, DisplayABI::DCLinkSignal) != SIGNAL_TYPE_EDP
        || getMember<UInt8>(link, DisplayABI::DCLinkSkipAUX))
    {
        return DisplayData::BrightnessResult::Unsupported;
    }
    const auto read = [this, link](uint32_t address, uint8_t* data, size_t size)
    { return this->readDPCD(link, address, data, static_cast<UInt32>(size)) == DisplayABI::DPCDSuccess; };
    const auto write = [this, link](uint32_t address, uint8_t* data, size_t size)
    { return this->writeDPCD(link, address, data, static_cast<UInt32>(size)) == DisplayABI::DPCDSuccess; };
    return DisplayData::setSDRBacklight(read, write, value, maximum);
}
