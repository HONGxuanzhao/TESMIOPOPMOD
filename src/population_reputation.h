#ifndef TESMIO_POPULATION_REPUTATION_H
#define TESMIO_POPULATION_REPUTATION_H
static Rep21Binding gBirthRep = {};
static float gConfiguredBirthBase;
static bool gBirthCompatActive;
static int gOldRepBirthResearch, gOldRepBirthField;
typedef void (*PopulationRenderFn)(void*,bool,void*,void*,int,int);
static PopulationRenderFn gPopulationRenderOriginal;

static bool UpdatePopulationBirthCell(double multiplier) {
    double scaled = (double)gConfiguredBirthBase * multiplier;
    if (!_finite(scaled) || scaled < 0.0 || scaled > 1000000.0 || !gRuntimeCells)
        return false;
    float value = (float)scaled;
    if (gRuntimeCells->baseFactor == value) return true;
    DWORD old=0, ignored=0;
    if (!VirtualProtect(&gRuntimeCells->baseFactor,4,PAGE_READWRITE,&old)) return false;
    // One naturally aligned private float; never rewrite the other factors.
    LONG bits=0; memcpy(&bits,&value,4);
    InterlockedExchange((volatile LONG*)&gRuntimeCells->baseFactor,bits);
    if (!VirtualProtect(&gRuntimeCells->baseFactor,4,old,&ignored)) return false;
    Logf("TESMIOPOPMOD Reputation birth multiplier=%.6g configured_base=%.6g effective_base=%.6g",multiplier,(double)gConfiguredBirthBase,(double)value);
    return true;
}
static void UpdatePopulationReputation() {
    if (!gBirthCompatActive) return;
    Rep21Enter(gBirthRep);
    __try {
        double m=Rep21BirthMultiplier(gBirthRep);
        if (!UpdatePopulationBirthCell(m)) {
            UpdatePopulationBirthCell(1.0);
            *(int*)(gBirthRep.base+0xC3014)=gOldRepBirthResearch;
            *(int*)(gBirthRep.base+0xC355C)=gOldRepBirthField;
            gBirthCompatActive=false;
            Logf("TESMIOPOPMOD Reputation adapter stopped: private cell update failed; Reputation birth writers restored");
        }
    } __finally { Rep21Leave(gBirthRep); }
}
static void PopulationRenderDetour(void* self,bool a,void* c1,void* c2,int b,int c) {
    // Invoke the existing chain first: Reputation updates its actual event state
    // there. No duplicated event schedules or second daily accumulator write.
    gPopulationRenderOriginal(self,a,c1,c2,b,c);
    UpdatePopulationReputation();
}
static int StartPopulationReputation() {
    if (!gRuntimeCells || !H->configInt(kIniName,kSection,"reputation_compat_enabled",1)) return 0;
    int found=Rep21Bind(&gBirthRep);
    if (!found) { Logf("TESMIOPOPMOD no Reputation provider: standalone population factors active"); return 0; }
    if (found<0) { Logf("TESMIOPOPMOD Reputation adapter refused: supported binary is 2.1 timestamp 6AB18B03; base factors remain active"); return 0; }
    static const BYTE gate[]={0x83,0x3D,0xCB,0x11,0x0B,0x00,0x00};
    // This exact cmp guards the verified resident birth-field writer.
    static const BYTE researchGate[]={0x83,0x3D,0x8F,0xEB,0x0B,0x00,0x00};
    if (!Rep21Matches(gBirthRep.base,0x1238A,gate,sizeof(gate)) ||
        !Rep21Matches(gBirthRep.base,0x447E,researchGate,sizeof(researchGate)) ||
        !H->readablePtr(gBirthRep.base+0xC3014,4) ||
        !H->readablePtr(gBirthRep.base+0xC355C,4) ||
        !H->readablePtr(gBirthRep.base+0xC4DA8,sizeof(CRITICAL_SECTION)) ||
        !*(int*)(gBirthRep.base+0xD72D4) || !H->patchIat) {
        Logf("TESMIOPOPMOD Reputation birth adapter refused: writer/clock layout changed"); return 0;
    }
    if (!H->patchIat(H->exeModule,"C3DDLL64.dll",
        "?Render@C3D_TERRAIN@@QEAAX_NPEAVC3D_CAMERA@@0HH@Z",(void*)PopulationRenderDetour,
        (void**)&gPopulationRenderOriginal,"POPMOD Reputation birth overlay")) {
        Logf("TESMIOPOPMOD Reputation birth adapter refused: no frame clock; native Reputation writers retained"); return 0;
    }
    Rep21Enter(gBirthRep);
    gOldRepBirthResearch=*(int*)(gBirthRep.base+0xC3014);
    gOldRepBirthField=*(int*)(gBirthRep.base+0xC355C);
    *(int*)(gBirthRep.base+0xC3014)=0;
    *(int*)(gBirthRep.base+0xC355C)=0;
    gBirthCompatActive=true;
    Rep21Leave(gBirthRep);
    Logf("TESMIOPOPMOD Reputation 2.1 adapter active: one exact aggregate multiplier on private birth base; event research/accumulator double application suppressed; native research and death factor retained");
    return 0;
}
#endif
