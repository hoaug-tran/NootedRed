// Invariant high-entropy context signatures for dynamic VDA user-space driver patching.
// Target binary: /System/Library/Extensions/AMDRadeonVADriver2.bundle/Contents/MacOS/AMDRadeonVADriver2

#ifndef VDASignatures_inc
#define VDASignatures_inc

namespace
{
    // 1. IdentifyForceFamily5: VAAcceleratorInfo::identify -> classify device 0x1638 as VA family 5 (Navi10/VCN2)
    static const UInt8 kVDACtx_Identify[] = {
        0x3D, 0x60, 0x73, 0x00, 0x00, 0x75, 0x0A, 0xBB, 0x06, 0x00, 0x00, 0x00,
        0xE9, 0x00, 0x00, 0x00, 0x00, // Wildcard jump offset
        0xBB, 0x0B, 0x00, 0x00, 0x00,
        0xE9, 0x00, 0x00, 0x00, 0x00  // Wildcard jump offset
    };
    static const UInt8 kVDAMask_Identify[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x00, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAFind_Identify[]    = {0xBB, 0x0B, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_Identify[] = {0xBB, 0x05, 0x00, 0x00, 0x00};

    // 2. AddrLibInitVegaToRV: VAAddrLibInterface::init -> map family 0x8E (RV/Renoir) to addrlib family 0xD
    static const UInt8 kVDACtx_AddrLibInit[] = {
        0x41, 0x81, 0xFC, 0x82, 0x00, 0x00, 0x00, 0x74, 0x0E,
        0x41, 0x81, 0xFC, 0x8D, 0x00, 0x00, 0x00, 0x75, 0x09, 0xB8, 0x0D, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAMask_AddrLibInit[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_AddrLibInit[]    = {0x41, 0x81, 0xFC, 0x8D, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_AddrLibInit[] = {0x41, 0x81, 0xFC, 0x8E, 0x00, 0x00, 0x00};

    // 3. AddrLibCreateVegaToRV: Addr::Lib::Create -> accept family 0x8E and take Addr::Gfx9HwlInit
    static const UInt8 kVDACtx_AddrLibCreate[] = {
        0x74, 0x21, 0x3D, 0x8D, 0x00, 0x00, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAMask_AddrLibCreate[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAFind_AddrLibCreate[]    = {0x3D, 0x8D, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_AddrLibCreate[] = {0x3D, 0x8E, 0x00, 0x00, 0x00};

    // 4. Gfx9ConvertChipFamilyVegaToRV: Addr::V2::Gfx9Lib::HwlConvertChipFamily -> configure 0x8E via Gfx9
    static const UInt8 kVDACtx_ConvertFamily[] = {
        0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAMask_ConvertFamily[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAFind_ConvertFamily[]    = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_ConvertFamily[] = {0x81, 0xFE, 0x8E, 0x00, 0x00, 0x00};

    // 5. ForceGfx9EngineAndBlt: VAFactory::createGraphicsEngine/createImageBlt -> force GFX9 engine
    static const UInt8 kVDACtx_EngineAndBlt[] = {
        0x48, 0x8B, 0x86, 0x60, 0x04, 0x00, 0x00, 0x8B, 0x40, 0x0C, 0x8D, 0x48, 0xFF, 0x83
    };
    static const UInt8 kVDAMask_EngineAndBlt[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_EngineAndBlt[]    = {0x48, 0x8B, 0x86, 0x60, 0x04, 0x00, 0x00, 0x8B, 0x40, 0x0C, 0x8D, 0x48, 0xFF, 0x83};
    static const UInt8 kVDAReplace_EngineAndBlt[] = {0xB8, 0x02, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x8D, 0x48, 0xFF, 0x83};

    // 6. ForceGfx9VideoProcessors: VAFactory color format/dithering/deblocker -> Gfx9 branch
    static const UInt8 kVDACtx_VideoProcessors[] = {
        0x8D, 0x46, 0xFF, 0x83, 0xF8, 0x02, 0x72, 0x1F
    };
    static const UInt8 kVDAMask_VideoProcessors[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_VideoProcessors[]    = {0x8D, 0x46, 0xFF, 0x83, 0xF8, 0x02, 0x72, 0x1F};
    static const UInt8 kVDAReplace_VideoProcessors[] = {0x8D, 0x46, 0xFF, 0x83, 0xF8, 0x02, 0xEB, 0x1F};

    // 7. Vcn2EncCommandFeedbackStride: Vcn2EncCommand::getFeedbackSize -> 48-byte (0x30) stride
    static const UInt8 kVDACtx_EncFeedbackStride[] = {
        0x55, 0x48, 0x89, 0xE5, 0xB8, 0xA4, 0x00, 0x00, 0x00, 0x5D, 0xC3
    };
    static const UInt8 kVDAMask_EncFeedbackStride[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_EncFeedbackStride[]    = {0xB8, 0xA4, 0x00, 0x00, 0x00, 0x5D, 0xC3};
    static const UInt8 kVDAReplace_EncFeedbackStride[] = {0xB8, 0x30, 0x00, 0x00, 0x00, 0x5D, 0xC3};

    // 8a. ProcessEncodeStatusStride (Ventura): AVDVcn2(Avc/Hevc)EncDataProcessor::ProcessEncodeStatus
    static const UInt8 kVDACtx_StatusStride_Ventura[] = {
        0x4C, 0x69, 0xE8, 0xA4, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAMask_StatusStride_Ventura[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_StatusStride_Ventura[]    = {0x4C, 0x69, 0xE8, 0xA4, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_StatusStride_Ventura[] = {0x4C, 0x69, 0xE8, 0x30, 0x00, 0x00, 0x00};

    // 8b. ProcessEncodeStatusStride (Sonoma, Sequoia, Tahoe): imul rax, rax, 0x30
    static const UInt8 kVDACtx_StatusStride_Modern[] = {
        0x48, 0x69, 0xC0, 0xA4, 0x00, 0x00, 0x00
    };
    static const UInt8 kVDAMask_StatusStride_Modern[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_StatusStride_Modern[]    = {0x48, 0x69, 0xC0, 0xA4, 0x00, 0x00, 0x00};
    static const UInt8 kVDAReplace_StatusStride_Modern[] = {0x48, 0x69, 0xC0, 0x30, 0x00, 0x00, 0x00};

    // 9. Vcn2EncoderSourcePitch: Vcn2Encoder::setEncodeParams -> true source texture pitch read
    static const UInt8 kVDACtx_SourcePitch[] = {
        0x8B, 0x8A, 0x84, 0x00, 0x00, 0x00, 0x8B, 0x97, 0x24, 0x04, 0x00, 0x00, 0x0F, 0xAF, 0xD1, 0x0F,
        0xB6, 0xFA, 0xB8, 0x00, 0x01, 0x00, 0x00, 0x29, 0xF8, 0x85, 0xFF, 0x0F, 0x44, 0xC7, 0x01, 0xD0
    };
    static const UInt8 kVDAMask_SourcePitch[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const UInt8 kVDAFind_SourcePitch[] = {
        0x8B, 0x8A, 0x84, 0x00, 0x00, 0x00, 0x8B, 0x97, 0x24, 0x04, 0x00, 0x00, 0x0F, 0xAF, 0xD1, 0x0F,
        0xB6, 0xFA, 0xB8, 0x00, 0x01, 0x00, 0x00, 0x29, 0xF8, 0x85, 0xFF, 0x0F, 0x44, 0xC7, 0x01, 0xD0
    };
    static const UInt8 kVDAReplace_SourcePitch[] = {
        0x8B, 0x82, 0x80, 0x00, 0x00, 0x00, 0x8B, 0x8A, 0x84, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90,
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
    };

    // Resolver table for macOS 13 (Ventura)
    static const UserPatches::SignaturePatch kVDAResolverSignatures_Ventura[] = {
        {kVDACtx_Identify, kVDAMask_Identify, sizeof(kVDACtx_Identify), 17,
         kVDAReplace_Identify, sizeof(kVDAReplace_Identify), 0, 1, ~0ULL,
         kVDAFind_Identify, sizeof(kVDAFind_Identify)},
        {kVDACtx_AddrLibInit, kVDAMask_AddrLibInit, sizeof(kVDACtx_AddrLibInit), 9,
         kVDAReplace_AddrLibInit, sizeof(kVDAReplace_AddrLibInit), 0, 1, ~0ULL,
         kVDAFind_AddrLibInit, sizeof(kVDAFind_AddrLibInit)},
        {kVDACtx_AddrLibCreate, kVDAMask_AddrLibCreate, sizeof(kVDACtx_AddrLibCreate), 2,
         kVDAReplace_AddrLibCreate, sizeof(kVDAReplace_AddrLibCreate), 0, 1, ~0ULL,
         kVDAFind_AddrLibCreate, sizeof(kVDAFind_AddrLibCreate)},
        {kVDACtx_ConvertFamily, kVDAMask_ConvertFamily, sizeof(kVDACtx_ConvertFamily), 0,
         kVDAReplace_ConvertFamily, sizeof(kVDAReplace_ConvertFamily), 0, 1, ~0ULL,
         kVDAFind_ConvertFamily, sizeof(kVDAFind_ConvertFamily)},
        {kVDACtx_EngineAndBlt, kVDAMask_EngineAndBlt, sizeof(kVDACtx_EngineAndBlt), 0,
         kVDAReplace_EngineAndBlt, sizeof(kVDAReplace_EngineAndBlt), 0, 2, ~0ULL,
         kVDAFind_EngineAndBlt, sizeof(kVDAFind_EngineAndBlt)},
        {kVDACtx_VideoProcessors, kVDAMask_VideoProcessors, sizeof(kVDACtx_VideoProcessors), 0,
         kVDAReplace_VideoProcessors, sizeof(kVDAReplace_VideoProcessors), 0, 3, ~0ULL,
         kVDAFind_VideoProcessors, sizeof(kVDAFind_VideoProcessors)},
        {kVDACtx_EncFeedbackStride, kVDAMask_EncFeedbackStride, sizeof(kVDACtx_EncFeedbackStride), 4,
         kVDAReplace_EncFeedbackStride, sizeof(kVDAReplace_EncFeedbackStride), 0, 1, ~0ULL,
         kVDAFind_EncFeedbackStride, sizeof(kVDAFind_EncFeedbackStride)},
        {kVDACtx_StatusStride_Ventura, kVDAMask_StatusStride_Ventura, sizeof(kVDACtx_StatusStride_Ventura), 0,
         kVDAReplace_StatusStride_Ventura, sizeof(kVDAReplace_StatusStride_Ventura), 0, 2, ~0ULL,
         kVDAFind_StatusStride_Ventura, sizeof(kVDAFind_StatusStride_Ventura)},
        {kVDACtx_SourcePitch, kVDAMask_SourcePitch, sizeof(kVDACtx_SourcePitch), 0,
         kVDAReplace_SourcePitch, sizeof(kVDAReplace_SourcePitch), 0, 1, ~0ULL,
         kVDAFind_SourcePitch, sizeof(kVDAFind_SourcePitch)},
    };
    constexpr size_t kVDAResolverSignatureCount_Ventura = arrsize(kVDAResolverSignatures_Ventura);

    // Resolver table for modern macOS (14 Sonoma, 15 Sequoia, 26 Tahoe)
    static const UserPatches::SignaturePatch kVDAResolverSignatures_Modern[] = {
        {kVDACtx_Identify, kVDAMask_Identify, sizeof(kVDACtx_Identify), 17,
         kVDAReplace_Identify, sizeof(kVDAReplace_Identify), 0, 1, ~0ULL,
         kVDAFind_Identify, sizeof(kVDAFind_Identify)},
        {kVDACtx_AddrLibInit, kVDAMask_AddrLibInit, sizeof(kVDACtx_AddrLibInit), 9,
         kVDAReplace_AddrLibInit, sizeof(kVDAReplace_AddrLibInit), 0, 1, ~0ULL,
         kVDAFind_AddrLibInit, sizeof(kVDAFind_AddrLibInit)},
        {kVDACtx_AddrLibCreate, kVDAMask_AddrLibCreate, sizeof(kVDACtx_AddrLibCreate), 2,
         kVDAReplace_AddrLibCreate, sizeof(kVDAReplace_AddrLibCreate), 0, 1, ~0ULL,
         kVDAFind_AddrLibCreate, sizeof(kVDAFind_AddrLibCreate)},
        {kVDACtx_ConvertFamily, kVDAMask_ConvertFamily, sizeof(kVDACtx_ConvertFamily), 0,
         kVDAReplace_ConvertFamily, sizeof(kVDAReplace_ConvertFamily), 0, 1, ~0ULL,
         kVDAFind_ConvertFamily, sizeof(kVDAFind_ConvertFamily)},
        {kVDACtx_EngineAndBlt, kVDAMask_EngineAndBlt, sizeof(kVDACtx_EngineAndBlt), 0,
         kVDAReplace_EngineAndBlt, sizeof(kVDAReplace_EngineAndBlt), 0, 2, ~0ULL,
         kVDAFind_EngineAndBlt, sizeof(kVDAFind_EngineAndBlt)},
        {kVDACtx_VideoProcessors, kVDAMask_VideoProcessors, sizeof(kVDACtx_VideoProcessors), 0,
         kVDAReplace_VideoProcessors, sizeof(kVDAReplace_VideoProcessors), 0, 3, ~0ULL,
         kVDAFind_VideoProcessors, sizeof(kVDAFind_VideoProcessors)},
        {kVDACtx_EncFeedbackStride, kVDAMask_EncFeedbackStride, sizeof(kVDACtx_EncFeedbackStride), 4,
         kVDAReplace_EncFeedbackStride, sizeof(kVDAReplace_EncFeedbackStride), 0, 1, ~0ULL,
         kVDAFind_EncFeedbackStride, sizeof(kVDAFind_EncFeedbackStride)},
        {kVDACtx_StatusStride_Modern, kVDAMask_StatusStride_Modern, sizeof(kVDACtx_StatusStride_Modern), 0,
         kVDAReplace_StatusStride_Modern, sizeof(kVDAReplace_StatusStride_Modern), 0, 2, ~0ULL,
         kVDAFind_StatusStride_Modern, sizeof(kVDAFind_StatusStride_Modern)},
        {kVDACtx_SourcePitch, kVDAMask_SourcePitch, sizeof(kVDACtx_SourcePitch), 0,
         kVDAReplace_SourcePitch, sizeof(kVDAReplace_SourcePitch), 0, 1, ~0ULL,
         kVDAFind_SourcePitch, sizeof(kVDAFind_SourcePitch)},
    };
    constexpr size_t kVDAResolverSignatureCount_Modern = arrsize(kVDAResolverSignatures_Modern);
}

#endif // VDASignatures_inc
