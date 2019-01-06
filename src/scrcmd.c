#include "global.h"
#include "gba/isagbprint.h"
#include "palette.h"
#include "script.h"
#include "mystery_event_script.h"
#include "event_data.h"
#include "random.h"
#include "item.h"
#include "overworld.h"
#include "field_screen_effect.h"
#include "quest_log.h"
#include "map_preview_screen.h"
#include "field_weather.h"

extern u16 (*const gSpecials[])(void);
extern u16 (*const gSpecialsEnd[])(void);
extern const u8 *const gStdScripts[];
extern const u8 *const gStdScriptsEnd[];

EWRAM_DATA ptrdiff_t gVScriptOffset = 0;
EWRAM_DATA u8 gUnknown_20370AC = 0;
EWRAM_DATA u16 gUnknown_20370AE = 0;
EWRAM_DATA u16 gUnknown_20370B0 = 0;
EWRAM_DATA u16 gUnknown_20370B2 = 0;
EWRAM_DATA u16 gUnknown_20370B4 = 0;
EWRAM_DATA u16 gUnknown_20370B6 = 0;

// This is defined in here so the optimizer can't see its value when compiling
// script.c.
void * const gNullScriptPtr = NULL;

const u8 sScriptConditionTable[6][3] =
    {
//  <  =  >
        1, 0, 0, // <
        0, 1, 0, // =
        0, 0, 1, // >
        1, 1, 0, // <=
        0, 1, 1, // >=
        1, 0, 1, // !=
    };



#define SCRCMD_DEF(name) bool8 name(struct ScriptContext *ctx)

SCRCMD_DEF(ScrCmd_nop)
{
    return FALSE;
}

SCRCMD_DEF(ScrCmd_nop1)
{
    return FALSE;
}

SCRCMD_DEF(ScrCmd_end)
{
    StopScript(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_gotonative)
{
    bool8 (*func)(void) = (bool8 (*)(void))ScriptReadWord(ctx);
    SetupNativeScript(ctx, func);
    return TRUE;
}

SCRCMD_DEF(ScrCmd_special)
{
    u16 (*const *specialPtr)(void) = gSpecials + ScriptReadHalfword(ctx);
    if (specialPtr < gSpecialsEnd)
        (*specialPtr)();
    else
         AGB_ASSERT_EX(0, "C:/WORK/POKeFRLG/src/pm_lgfr_ose/source/scrcmd.c", 241);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_specialvar)
{
    u16 * varPtr = GetVarPointer(ScriptReadHalfword(ctx));
    u16 (*const *specialPtr)(void) = gSpecials + ScriptReadHalfword(ctx);
    if (specialPtr < gSpecialsEnd)
        *varPtr = (*specialPtr)();
    else
         AGB_ASSERT_EX(0, "C:/WORK/POKeFRLG/src/pm_lgfr_ose/source/scrcmd.c", 263);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_callnative)
{
    void (*func )(void) = ((void (*)(void))ScriptReadWord(ctx));
    func();
    return FALSE;
}

SCRCMD_DEF(ScrCmd_waitstate)
{
    ScriptContext1_Stop();
    return TRUE;
}

SCRCMD_DEF(ScrCmd_goto)
{
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    ScriptJump(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_return)
{
    ScriptReturn(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_call)
{
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    ScriptCall(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_goto_if)
{
    u8 condition = ScriptReadByte(ctx);
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
        ScriptJump(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_call_if)
{
    u8 condition = ScriptReadByte(ctx);
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
        ScriptCall(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_setvaddress)
{
    u32 addr1 = (u32)ctx->scriptPtr - 1;
    u32 addr2 = ScriptReadWord(ctx);

    gVScriptOffset = addr2 - addr1;
    return FALSE;
}

SCRCMD_DEF(ScrCmd_vgoto)
{
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    ScriptJump(ctx, scrptr - gVScriptOffset);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_vcall)
{
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx);
    ScriptCall(ctx, scrptr - gVScriptOffset);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_vgoto_if)
{
    u8 condition = ScriptReadByte(ctx);
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx) - gVScriptOffset;
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
        ScriptJump(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_vcall_if)
{
    u8 condition = ScriptReadByte(ctx);
    const u8 * scrptr = (const u8 *)ScriptReadWord(ctx) - gVScriptOffset;
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
        ScriptCall(ctx, scrptr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_gotostd)
{
    u8 stdIdx = ScriptReadByte(ctx);
    const u8 *const * script = gStdScripts + stdIdx;
    if (script < gStdScriptsEnd)
        ScriptJump(ctx, *script);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_callstd)
{
    u8 stdIdx = ScriptReadByte(ctx);
    const u8 *const * script = gStdScripts + stdIdx;
    if (script < gStdScriptsEnd)
        ScriptCall(ctx, *script);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_gotostd_if)
{
    u8 condition = ScriptReadByte(ctx);
    u8 stdIdx = ScriptReadByte(ctx);
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
    {
        const u8 *const * script = gStdScripts + stdIdx;
        if (script < gStdScriptsEnd)
            ScriptJump(ctx, *script);
    }
    return FALSE;
}

SCRCMD_DEF(ScrCmd_callstd_if)
{
    u8 condition = ScriptReadByte(ctx);
    u8 stdIdx = ScriptReadByte(ctx);
    if (sScriptConditionTable[condition][ctx->comparisonResult] == 1)
    {
        const u8 *const * script = gStdScripts + stdIdx;
        if (script < gStdScriptsEnd)
            ScriptCall(ctx, *script);
    }
    return FALSE;
}

SCRCMD_DEF(ScrCmd_gotoram)
{
    ScriptJump(ctx, gRAMScriptPtr);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_killscript)
{
    ClearRamScript();
    StopScript(ctx);
    return TRUE;
}

SCRCMD_DEF(ScrCmd_setmysteryeventstatus)
{
    SetMysteryEventScriptStatus(ScriptReadByte(ctx));
    return FALSE;
}

SCRCMD_DEF(sub_806A28C)
{
    const u8 * script = sub_8069E48();
    if (script != NULL)
    {
        gRAMScriptPtr = ctx->scriptPtr;
        ScriptJump(ctx, script);
    }
    return FALSE;
}

SCRCMD_DEF(ScrCmd_loadword)
{
    u8 which = ScriptReadByte(ctx);
    ctx->data[which] = ScriptReadWord(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_loadbytefromaddr)
{
    u8 which = ScriptReadByte(ctx);
    ctx->data[which] = *(const u8 *)ScriptReadWord(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_writebytetoaddr)
{
    u8 value = ScriptReadByte(ctx);
    *(u8 *)ScriptReadWord(ctx) = value;
    return FALSE;
}

SCRCMD_DEF(ScrCmd_loadbyte)
{
    u8 which = ScriptReadByte(ctx);
    ctx->data[which] = ScriptReadByte(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_setptrbyte)
{
    u8 which = ScriptReadByte(ctx);
    *(u8 *)ScriptReadWord(ctx) = ctx->data[which];
    return FALSE;
}

SCRCMD_DEF(ScrCmd_copylocal)
{
    u8 whichDst = ScriptReadByte(ctx);
    u8 whichSrc = ScriptReadByte(ctx);
    ctx->data[whichDst] = ctx->data[whichSrc];
    return FALSE;
}

SCRCMD_DEF(ScrCmd_copybyte)
{
    u8 * dest = (u8 *)ScriptReadWord(ctx);
    *dest = *(const u8 *)ScriptReadWord(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_setvar)
{
    u16 * varPtr = GetVarPointer(ScriptReadHalfword(ctx));
    *varPtr = ScriptReadHalfword(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_copyvar)
{
    u16 * destPtr = GetVarPointer(ScriptReadHalfword(ctx));
    u16 * srcPtr = GetVarPointer(ScriptReadHalfword(ctx));
    *destPtr = *srcPtr;
    return FALSE;
}

SCRCMD_DEF(ScrCmd_setorcopyvar)
{
    u16 * destPtr = GetVarPointer(ScriptReadHalfword(ctx));
    *destPtr = VarGet(ScriptReadHalfword(ctx));
    return FALSE;
}

u8 * const sScriptStringVars[] =
{
    gStringVar1,
    gStringVar2,
    gStringVar3,
};

u8 compare_012(u16 left, u16 right)
{
    if (left < right)
        return 0;
    else if (left == right)
        return 1;
    else
        return 2;
}

// comparelocaltolocal
SCRCMD_DEF(ScrCmd_compare_local_to_local)
{
    const u8 value1 = ctx->data[ScriptReadByte(ctx)];
    const u8 value2 = ctx->data[ScriptReadByte(ctx)];

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

// comparelocaltoimm
SCRCMD_DEF(ScrCmd_compare_local_to_value)
{
    const u8 value1 = ctx->data[ScriptReadByte(ctx)];
    const u8 value2 = ScriptReadByte(ctx);

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_local_to_addr)
{
    const u8 value1 = ctx->data[ScriptReadByte(ctx)];
    const u8 value2 = *(const u8 *)ScriptReadWord(ctx);

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_addr_to_local)
{
    const u8 value1 = *(const u8 *)ScriptReadWord(ctx);
    const u8 value2 = ctx->data[ScriptReadByte(ctx)];

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_addr_to_value)
{
    const u8 value1 = *(const u8 *)ScriptReadWord(ctx);
    const u8 value2 = ScriptReadByte(ctx);

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_addr_to_addr)
{
    const u8 value1 = *(const u8 *)ScriptReadWord(ctx);
    const u8 value2 = *(const u8 *)ScriptReadWord(ctx);

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_var_to_value)
{
    const u16 value1 = *GetVarPointer(ScriptReadHalfword(ctx));
    const u16 value2 = ScriptReadHalfword(ctx);

    ctx->comparisonResult = compare_012(value1, value2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_compare_var_to_var)
{
    const u16 *ptr1 = GetVarPointer(ScriptReadHalfword(ctx));
    const u16 *ptr2 = GetVarPointer(ScriptReadHalfword(ctx));

    ctx->comparisonResult = compare_012(*ptr1, *ptr2);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_addvar)
{
    u16 *ptr = GetVarPointer(ScriptReadHalfword(ctx));
    *ptr += ScriptReadHalfword(ctx);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_subvar)
{
    u16 *ptr = GetVarPointer(ScriptReadHalfword(ctx));
    *ptr -= VarGet(ScriptReadHalfword(ctx));
    return FALSE;
}

SCRCMD_DEF(ScrCmd_random)
{
    u16 max = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = Random() % max;
    return FALSE;
}

SCRCMD_DEF(ScrCmd_giveitem)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u32 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = AddBagItem(itemId, (u8)quantity);
    sub_809A824(itemId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_takeitem)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u32 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = RemoveBagItem(itemId, (u8)quantity);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkitemspace)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u32 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = CheckBagHasSpace(itemId, (u8)quantity);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkitem)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u32 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = CheckBagHasItem(itemId, (u8)quantity);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkitemtype)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = GetPocketByItemId(itemId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_givepcitem)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u16 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = AddPCItem(itemId, quantity);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkpcitem)
{
    u16 itemId = VarGet(ScriptReadHalfword(ctx));
    u16 quantity = VarGet(ScriptReadHalfword(ctx));

    gSpecialVar_Result = CheckPCHasItem(itemId, quantity);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_givedecoration)
{
    u32 decorId = VarGet(ScriptReadHalfword(ctx));

//    gSpecialVar_Result = DecorationAdd(decorId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_takedecoration)
{
    u32 decorId = VarGet(ScriptReadHalfword(ctx));

//    gSpecialVar_Result = DecorationRemove(decorId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkdecorspace)
{
    u32 decorId = VarGet(ScriptReadHalfword(ctx));

//    gSpecialVar_Result = DecorationCheckSpace(decorId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkdecor)
{
    u32 decorId = VarGet(ScriptReadHalfword(ctx));

//    gSpecialVar_Result = CheckHasDecoration(decorId);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_setflag)
{
    FlagSet(ScriptReadHalfword(ctx));
    return FALSE;
}

SCRCMD_DEF(ScrCmd_clearflag)
{
    FlagClear(ScriptReadHalfword(ctx));
    return FALSE;
}

SCRCMD_DEF(ScrCmd_checkflag)
{
    ctx->comparisonResult = FlagGet(ScriptReadHalfword(ctx));
    return FALSE;
}

SCRCMD_DEF(ScrCmd_incrementgamestat)
{
    IncrementGameStat(ScriptReadByte(ctx));
    return FALSE;
}

SCRCMD_DEF(sub_806A888)
{
    u8 statIdx = ScriptReadByte(ctx);
    u32 value = ScriptReadWord(ctx);
    u32 statValue = GetGameStat(statIdx);

    if (statValue < value)
        ctx ->comparisonResult = 0;
    else if (statValue == value)
        ctx->comparisonResult = 1;
    else
        ctx->comparisonResult = 2;
    return FALSE;
}

SCRCMD_DEF(sub_806A8C0)
{
    u16 value = ScriptReadHalfword(ctx);
    sub_8115748(value);
    sub_80F85BC(value);
    return FALSE;
}

SCRCMD_DEF(ScrCmd_animateflash)
{
    sub_807F028(ScriptReadByte(ctx));
    ScriptContext1_Stop();
    return TRUE;
}

SCRCMD_DEF(ScrCmd_setflashradius)
{
    u16 flashLevel = VarGet(ScriptReadHalfword(ctx));

    Overworld_SetFlashLevel(flashLevel);
    return FALSE;
}

static bool8 IsPaletteNotActive(void)
{
    if (!gPaletteFade.active)
        return TRUE;
    else
        return FALSE;
}

SCRCMD_DEF(ScrCmd_fadescreen)
{
    fade_screen(ScriptReadByte(ctx), 0);
    SetupNativeScript(ctx, IsPaletteNotActive);
    return TRUE;
}

SCRCMD_DEF(ScrCmd_fadescreenspeed)
{
    u8 mode = ScriptReadByte(ctx);
    u8 speed = ScriptReadByte(ctx);

    fade_screen(mode, speed);
    SetupNativeScript(ctx, IsPaletteNotActive);
    return TRUE;
}
