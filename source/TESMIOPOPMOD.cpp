// TESMIOPOPMOD - version-locked, resident population control for Workers &
// Resources: Soviet Republic through TesmioLoader.
//
// The executable contains a resident birth accumulator and a type-specific
// resident death-candidate branch.  This plugin redirects only four exact
// RIP-relative constant reads to private plugin-owned storage.  It deliberately
// does not hook the global random generator or the generic resident-removal
// routine.

#include "tesmio_plugin.h"

#include <float.h>
#include <stdint.h>
#include "reputation_21.h"

#if !defined(_WIN64)
#error TESMIOPOPMOD must be built as an x64 DLL.
#endif

static const char kIniName[] = "plugins\\TESMIOPOPMOD.ini";
static const char kSection[] = "TESMIOPOPMOD";

// These RVAs are for the locally installed WRSR v1.1.1.9 x64 image.  The
// exact instruction bytes and their original RIP targets are checked before
// any write.  A different executable layout is refused.
// TsmioLoader exposes PE SizeOfImage (memory image size), not the on-disk
// file length.  The latter is 10,308,608 bytes; the loaded image is 0xA9D000.
static const unsigned kExpectedImageMemorySize = 11128832u;
static const unsigned kBirthFunctionRva = 0x00834A20u;
static const unsigned kResidentUpdateFunctionRva = 0x00834DE0u;

static const unsigned kBirthResearchIncreaseSiteRva = 0x00834C6Eu;
static const unsigned kBirthResearchDecreaseSiteRva = 0x00834C7Fu;
static const unsigned kBirthBaseSiteRva             = 0x00834C87u;

static const unsigned kNativeResearchIncreaseRva = 0x00909FDCu; // 1.22
static const unsigned kNativeResearchDecreaseRva = 0x00909E54u; // 0.70
static const unsigned kNativeBirthBaseRva        = 0x00909E34u; // 2 / 3
static const unsigned kNativeDeathCandidateBaseRva = 0x00909ED4u; // 0.85

static const unsigned kDeathCandidateBaseSiteRva = 0x00835B51u;

struct PopulationSettings
{
    float baseFactor;
    float researchIncreaseFactor;
    float researchDecreaseFactor;
    float deathCandidateBaseFactor;
};

// The cells are private to this DLL/process.  No original .rdata cell is
// made writable, so other game functions continue to read their native data.
struct RuntimeCells
{
    float researchIncreaseFactor;
    float researchDecreaseFactor;
    float baseFactor;
    float deathCandidateBaseFactor;
};

static RuntimeCells gFallbackRuntimeCells = {};
static RuntimeCells* gRuntimeCells = NULL;

struct RipPatchSpec
{
    unsigned siteRva;
    unsigned displacementOffset;
    const unsigned char* expectedBytes;
    size_t expectedLength;
    unsigned expectedTargetRva;
    const char* label;
};

struct ActiveRipPatch
{
    RipPatchSpec spec;
    const void* replacementTarget;
};

static const unsigned char kBirthFunctionPrologue[] = {
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x48
};

static const unsigned char kBirthResearchIncreaseBytes[] = {
    0xF3, 0x0F, 0x59, 0x35, 0x66, 0x53, 0x0D, 0x00
};
static const unsigned char kBirthResearchDecreaseBytes[] = {
    0xF3, 0x0F, 0x59, 0x35, 0xCD, 0x51, 0x0D, 0x00
};
static const unsigned char kBirthBaseBytes[] = {
    0xF3, 0x0F, 0x59, 0x35, 0xA5, 0x51, 0x0D, 0x00
};
static const unsigned char kDeathUpdateFunctionPrologue[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08, 0x57, 0x41, 0x54,
    0x48, 0x81, 0xEC, 0xC8, 0x00, 0x00, 0x00, 0x48
};
static const unsigned char kDeathCandidateBaseBytes[] = {
    0xF3, 0x0F, 0x58, 0x05, 0x7B, 0x43, 0x0D, 0x00
};

static const RipPatchSpec kPopulationPatchSpecs[] = {
    { kBirthResearchIncreaseSiteRva, 4, kBirthResearchIncreaseBytes,
      sizeof(kBirthResearchIncreaseBytes), kNativeResearchIncreaseRva,
      "birth research-increase factor" },
    { kBirthResearchDecreaseSiteRva, 4, kBirthResearchDecreaseBytes,
      sizeof(kBirthResearchDecreaseBytes), kNativeResearchDecreaseRva,
      "birth research-decrease factor" },
    { kBirthBaseSiteRva, 4, kBirthBaseBytes, sizeof(kBirthBaseBytes),
      kNativeBirthBaseRva, "birth base factor" },
    { kDeathCandidateBaseSiteRva, 4, kDeathCandidateBaseBytes,
      sizeof(kDeathCandidateBaseBytes), kNativeDeathCandidateBaseRva,
      "death-candidate base factor" }
};

static bool ReadFloatSetting(const char* key, float fallback,
                             float minimum, float maximum, float* result)
{
    if (!key || !result || !H ||
        !HostHasField(offsetof(TsmHost, configString),
                      sizeof(H->configString)) ||
        !H->configString)
    {
        return false;
    }

    char text[96] = {};
    int present = H->configString(kIniName, kSection, key, text,
                                  (int)sizeof(text), "");
    if (!present || !text[0])
    {
        *result = fallback;
        return true;
    }

    char* end = NULL;
    double value = strtod(text, &end);
    while (end && (*end == ' ' || *end == '\t' || *end == '\r' ||
                   *end == '\n'))
    {
        ++end;
    }

    if (end == text || !end || *end != '\0' || !_finite(value) ||
        value < (double)minimum || value > (double)maximum)
    {
        Logf("TESMIOPOPMOD rejected %s=%s; allowed range is %.6g..%.6g",
             key, text, (double)minimum, (double)maximum);
        return false;
    }

    *result = (float)value;
    if (!_finite(*result) || *result < minimum || *result > maximum)
    {
        Logf("TESMIOPOPMOD rejected %s after float conversion", key);
        return false;
    }
    return true;
}

static bool ReadSettings(PopulationSettings* settings)
{
    if (!settings)
        return false;

    // These are the exact native factors in the v1.1.1.9 population routines.
    // They are absolute factors/terms, not percentages.
    if (!ReadFloatSetting("birth_base_factor", 0.6666667f, 0.0f, 4.0f,
                          &settings->baseFactor))
    {
        return false;
    }
    if (!ReadFloatSetting("birth_research_increase_factor", 1.22f,
                          0.0f, 4.0f,
                          &settings->researchIncreaseFactor))
    {
        return false;
    }
    if (!ReadFloatSetting("birth_research_decrease_factor", 0.70f,
                          0.0f, 4.0f,
                          &settings->researchDecreaseFactor))
    {
        return false;
    }
    if (!ReadFloatSetting("death_candidate_base_factor", 0.85f,
                          0.0f, 2.0f,
                          &settings->deathCandidateBaseFactor))
    {
        return false;
    }
    return true;
}

static bool RangeReadable(unsigned rva, size_t length)
{
    if (!g_exeBase || !g_exeSize || !H ||
        !HostHasField(offsetof(TsmHost, readablePtr),
                      sizeof(H->readablePtr)) ||
        !H->readablePtr)
    {
        return false;
    }
    if ((size_t)rva > (size_t)g_exeSize ||
        length > (size_t)g_exeSize - (size_t)rva)
    {
        return false;
    }
    return H->readablePtr(g_exeBase + rva, length) != 0;
}

static bool BytesMatch(unsigned rva, const unsigned char* expected,
                       size_t length)
{
    return expected && RangeReadable(rva, length) &&
           memcmp(g_exeBase + rva, expected, length) == 0;
}

static bool VerifyRipTarget(unsigned instructionRva, unsigned displacementOffset,
                            unsigned expectedTargetRva)
{
    if (!RangeReadable(instructionRva + displacementOffset, sizeof(int32_t)))
        return false;

    int32_t displacement = 0;
    memcpy(&displacement, g_exeBase + instructionRva + displacementOffset,
           sizeof(displacement));
    int64_t actualTarget = (int64_t)instructionRva + displacementOffset +
                           (int64_t)sizeof(int32_t) + displacement;
    return actualTarget == (int64_t)expectedTargetRva;
}

static bool VerifyGameLayout()
{
    if (!g_exeBase || g_exeSize != kExpectedImageMemorySize)
    {
        Logf("TESMIOPOPMOD refused: unsupported SOVIET64 image size (got %llu, expected %u)",
             (unsigned long long)g_exeSize, kExpectedImageMemorySize);
        return false;
    }

    bool ok = BytesMatch(kBirthFunctionRva, kBirthFunctionPrologue,
                         sizeof(kBirthFunctionPrologue));
    ok = ok && BytesMatch(kResidentUpdateFunctionRva,
                          kDeathUpdateFunctionPrologue,
                          sizeof(kDeathUpdateFunctionPrologue));
    for (unsigned i = 0;
         i < sizeof(kPopulationPatchSpecs) /
                 sizeof(kPopulationPatchSpecs[0]); ++i)
    {
        const RipPatchSpec& spec = kPopulationPatchSpecs[i];
        ok = ok && BytesMatch(spec.siteRva, spec.expectedBytes,
                              spec.expectedLength);
        ok = ok && VerifyRipTarget(spec.siteRva, spec.displacementOffset,
                                   spec.expectedTargetRva);
    }

    if (!ok)
    {
        Logf("TESMIOPOPMOD refused: population-function bytes or RIP targets do not match v1.1.1.9; no memory was changed");
        return false;
    }
    return true;
}

static bool ComputeRipDisplacement(unsigned siteRva, unsigned displacementOffset,
                                   const void* target, int32_t* result)
{
    if (!g_exeBase || !target || !result)
        return false;

    const BYTE* nextInstruction = g_exeBase + siteRva + displacementOffset +
                                  sizeof(int32_t);
    intptr_t targetAddress = (intptr_t)reinterpret_cast<uintptr_t>(target);
    intptr_t nextAddress = (intptr_t)reinterpret_cast<uintptr_t>(
        nextInstruction);
    intptr_t delta = targetAddress - nextAddress;
    if (delta < (intptr_t)-2147483648LL ||
        delta > (intptr_t)2147483647LL)
    {
        return false;
    }

    *result = (int32_t)delta;
    return true;
}

static bool WriteRipDisplacement(unsigned siteRva, unsigned displacementOffset,
                                 int32_t displacement)
{
    BYTE* address = g_exeBase + siteRva + displacementOffset;
    DWORD oldProtect = 0;
    if (!VirtualProtect(address, sizeof(displacement), PAGE_EXECUTE_READWRITE,
                        &oldProtect))
    {
        return false;
    }

    memcpy(address, &displacement, sizeof(displacement));
    DWORD ignored = 0;
    BOOL restored = VirtualProtect(address, sizeof(displacement), oldProtect,
                                   &ignored);
    FlushInstructionCache(g_exe, address, sizeof(displacement));
    return restored != FALSE;
}

static bool StoreRuntimeCells(const RuntimeCells& values)
{
    if (!gRuntimeCells)
        return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(gRuntimeCells, sizeof(RuntimeCells), PAGE_READWRITE,
                        &oldProtect))
    {
        Logf("TESMIOPOPMOD refused: private runtime storage is not writable (%lu)",
             (unsigned long)GetLastError());
        return false;
    }

    memcpy(gRuntimeCells, &values, sizeof(values));
    DWORD ignored = 0;
    BOOL restored = VirtualProtect(gRuntimeCells, sizeof(RuntimeCells),
                                   oldProtect, &ignored);
    if (!restored)
    {
        Logf("TESMIOPOPMOD refused: private runtime storage protection restore failed (%lu)",
             (unsigned long)GetLastError());
        return false;
    }
    return true;
}

static bool PrepareRuntimeCells(const PopulationSettings& settings)
{
    RuntimeCells values = {};
    values.researchIncreaseFactor = settings.researchIncreaseFactor;
    values.researchDecreaseFactor = settings.researchDecreaseFactor;
    values.baseFactor = settings.baseFactor;
    values.deathCandidateBaseFactor = settings.deathCandidateBaseFactor;

    RuntimeCells* candidate = NULL;
    if (HostHasField(offsetof(TsmHost, allocNear), sizeof(H->allocNear)) &&
        H->allocNear)
    {
        candidate = reinterpret_cast<RuntimeCells*>(H->allocNear(
            g_exeBase + kBirthFunctionRva, 0x1000));
    }
    if (!candidate)
    {
        candidate = &gFallbackRuntimeCells;
        Logf("TESMIOPOPMOD warning: allocNear unavailable; using range-checked DLL storage");
    }

    gRuntimeCells = candidate;
    if (!StoreRuntimeCells(values))
    {
        gRuntimeCells = NULL;
        return false;
    }
    return true;
}

struct AppliedRipPatch
{
    const RipPatchSpec* spec;
    int32_t oldDisplacement;
};

static bool ApplyRipPatches(const ActiveRipPatch* patches, size_t count)
{
    if (!patches || count == 0 || count > 8)
        return false;

    int32_t newDisplacements[8] = {};
    for (size_t i = 0; i < count; ++i)
    {
        const RipPatchSpec& spec = patches[i].spec;
        if (!BytesMatch(spec.siteRva, spec.expectedBytes,
                        spec.expectedLength) ||
            !VerifyRipTarget(spec.siteRva, spec.displacementOffset,
                             spec.expectedTargetRva))
        {
            Logf("TESMIOPOPMOD refused: %s site no longer matches the supported layout",
                 spec.label);
            return false;
        }
        if (!ComputeRipDisplacement(spec.siteRva, spec.displacementOffset,
                                    patches[i].replacementTarget,
                                    &newDisplacements[i]))
        {
            Logf("TESMIOPOPMOD refused: %s private target is outside x64 RIP range",
                 spec.label);
            return false;
        }
    }

    AppliedRipPatch applied[8] = {};
    size_t appliedCount = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const RipPatchSpec& spec = patches[i].spec;
        int32_t oldDisplacement = 0;
        memcpy(&oldDisplacement,
               g_exeBase + spec.siteRva + spec.displacementOffset,
               sizeof(oldDisplacement));
        applied[appliedCount].spec = &spec;
        applied[appliedCount].oldDisplacement = oldDisplacement;
        ++appliedCount;
        if (!WriteRipDisplacement(spec.siteRva, spec.displacementOffset,
                                   newDisplacements[i]))
        {
            Logf("TESMIOPOPMOD refused: could not patch %s (%lu)",
                 spec.label, (unsigned long)GetLastError());
            while (appliedCount != 0)
            {
                --appliedCount;
                const RipPatchSpec* rollback = applied[appliedCount].spec;
                if (!WriteRipDisplacement(rollback->siteRva,
                                          rollback->displacementOffset,
                                          applied[appliedCount].oldDisplacement))
                {
                    Logf("TESMIOPOPMOD ERROR: failed to roll back %s",
                         rollback->label);
                }
            }
            return false;
        }
    }

    Logf("TESMIOPOPMOD redirected %u resident-population factor reads to private storage",
         (unsigned)count);
    return true;
}

static bool ApplyPopulationSettings(const PopulationSettings& settings)
{
    if (!PrepareRuntimeCells(settings))
        return false;

    ActiveRipPatch patches[sizeof(kPopulationPatchSpecs) /
                           sizeof(kPopulationPatchSpecs[0])] = {};
    for (unsigned i = 0; i < sizeof(patches) / sizeof(patches[0]); ++i)
        patches[i].spec = kPopulationPatchSpecs[i];

    patches[0].replacementTarget = &gRuntimeCells->researchIncreaseFactor;
    patches[1].replacementTarget = &gRuntimeCells->researchDecreaseFactor;
    patches[2].replacementTarget = &gRuntimeCells->baseFactor;
    patches[3].replacementTarget = &gRuntimeCells->deathCandidateBaseFactor;

    if (!ApplyRipPatches(patches, sizeof(patches) / sizeof(patches[0])))
    {
        gRuntimeCells = NULL;
        return false;
    }

    Logf("TESMIOPOPMOD population patch active: birth RVA 0x%08X and death-candidate RVA 0x%08X; age/health/random/removal code is untouched",
         kBirthFunctionRva, kDeathCandidateBaseSiteRva);
    Logf("TESMIOPOPMOD birth_base_factor=%.9g birth_research_increase_factor=%.9g birth_research_decrease_factor=%.9g death_candidate_base_factor=%.9g",
         (double)settings.baseFactor,
         (double)settings.researchIncreaseFactor,
         (double)settings.researchDecreaseFactor,
         (double)settings.deathCandidateBaseFactor);
    return true;
}

#include "population_reputation.h"

extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void)
{
    return TSM_API_VERSION;
}

extern "C" __declspec(dllexport) int TsmPluginInit(const TsmHost* host,
                                                    TsmPluginInfo* info)
{
    if (!host || !info)
        return 1;

    TsmBind(host);
    info->name = "TESMIOPOPMOD";
    info->version = "0.4.0-reputation2.1";

    if (!H || !H->log)
        return 1;

    if (!HostHasField(offsetof(TsmHost, configInt), sizeof(H->configInt)) ||
        !H->configInt)
    {
        Logf("TESMIOPOPMOD refused: loader has no configInt API; no memory was changed");
        return 1;
    }

    if (!H->configInt(kIniName, kSection, "enabled", 0))
    {
        Logf("TESMIOPOPMOD disabled by plugins\\TESMIOPOPMOD.ini; no population memory was changed");
        return 0;
    }

    if (!VerifyGameLayout())
        return 1;

    PopulationSettings settings = {};
    if (!ReadSettings(&settings))
    {
        Logf("TESMIOPOPMOD refused: invalid population settings; no memory was changed");
        return 1;
    }

    if (!ApplyPopulationSettings(settings))
        return 1;
    gConfiguredBirthBase = settings.baseFactor;
    return 0;
}

extern "C" __declspec(dllexport) int TsmPluginStart(void)
{
    return StartPopulationReputation();
}
