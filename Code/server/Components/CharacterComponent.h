#pragma once

#ifndef TP_INTERNAL_COMPONENTS_GUARD
#error Include Components.h instead
#endif

#include <Structs/Tints.h>
#include <Structs/Factions.h>

struct CharacterComponent
{
    enum
    {
        kIsDirtyFactions = 1 << 0,
        kIsDead = 1 << 1,
        kIsPlayer = 1 << 2,
        kIsWeaponDrawn = 1 << 3,
        kIsDragon = 1 << 4,
    };

    [[nodiscard]] bool IsDirtyFactions() const
    {
        return Flags & kIsDirtyFactions;
    }
    [[nodiscard]] bool IsDead() const
    {
        return Flags & kIsDead;
    }
    [[nodiscard]] bool IsPlayer() const
    {
        return Flags & kIsPlayer;
    }
    [[nodiscard]] bool IsWeaponDrawn() const
    {
        return Flags & kIsWeaponDrawn;
    }
    [[nodiscard]] bool IsDragon() const
    {
        return Flags & kIsDragon;
    }

    void SetDirtyFactions(bool aSet)
    {
        if (aSet)
            Flags |= kIsDirtyFactions;
        else
            Flags &= ~kIsDirtyFactions;
    }
    void SetDead(bool aSet)
    {
        if (aSet)
            Flags |= kIsDead;
        else
            Flags &= ~kIsDead;
    }
    void SetPlayer(bool aSet)
    {
        if (aSet)
            Flags |= kIsPlayer;
        else
            Flags &= ~kIsPlayer;
    }
    void SetWeaponDrawn(bool aSet)
    {
        if (aSet)
            Flags |= kIsWeaponDrawn;
        else
            Flags &= ~kIsWeaponDrawn;
    }
    void SetDragon(bool aSet)
    {
        if (aSet)
            Flags |= kIsDragon;
        else
            Flags &= ~kIsDragon;
    }

    uint32_t ChangeFlags{ 0 };
    String SaveBuffer{};
    FormIdComponent BaseId{};
    Tints FaceTints{};
    Factions FactionsContent{};
    uint16_t Flags{};
};
