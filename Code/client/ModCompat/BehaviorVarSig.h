#pragma once

struct BehaviorVarSig
{
    struct Sig
    {
        std::string sigName;
        std::vector<std::string> sigStrings;
        std::vector<std::string> negSigStrings;
        std::vector<std::string> syncBooleanVar;
        std::vector<std::string> syncFloatVar;
        std::vector<std::string> syncIntegerVar;
    };

    struct Add
    {
        std::uint64_t mHash;
        std::vector<std::uint32_t> syncBooleanVar;
        std::vector<std::uint32_t> syncFloatVar;
        std::vector<std::uint32_t> syncIntegerVar;
    };

    // hardcoded
    // std::pair<uint64_t, Sig> humanoidSig;
    // std::pair<uint64_t, Sig> werewolfSig;
    // std::pair<uint64_t, Sig> vampireSig;

    // Sig pool
    std::vector<Sig> sigPool;
    std::unordered_map<uint64_t, bool> failedSig;

    // Add pool
    std::vector<Add> addPool;
    bool isAddPatched;

    static BehaviorVarSig* Get();
    void initialize();
    void patch(BSAnimationGraphManager* apManager, Actor* apActor);
    void patchAdd(Add& aAdd);

  private:
    static BehaviorVarSig* single;
    Sig* loadSigFromDir(std::string aDir);
    Add* loadAddFromDir(std::string aDir);
    void tryAddtoHash(BehaviorVarSig::Add& aAdd);
    std::vector<std::string> loadDirs(const std::string& acPATH);
    void vanillaPatch();
};
