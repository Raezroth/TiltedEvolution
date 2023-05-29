#include <BSAnimationGraphManager.h>
#include <Games/ActorExtension.h>
#include <ModCompat/BehaviorVarSig.h>
#include <Structs/AnimationGraphDescriptorManager.h>

#include <algorithm>
#include <fstream>
#include <iostream>

// debug
#define BEHAVIOR_DEBUG 1

#ifdef BEHAVIOR_DEBUG == 1
#define D(x) spdlog::info(x)
#else
#define D(x)
#endif

BehaviorVarSig* BehaviorVarSig::single = nullptr;

BehaviorVarSig* BehaviorVarSig::Get()
{
    if (!BehaviorVarSig::single)
        BehaviorVarSig::single = new BehaviorVarSig();
    return BehaviorVarSig::single;
}

void removeWhiteSpace(std::string& aString)
{
    // TODO: FIX THIS GODDAMN THING
    // aString.erase(std::remove(aString.begin(), aString.end(), std::isspace), aString.end());
}

bool isDirExist(std::string aPath)
{
    return std::filesystem::is_directory(aPath);
}

void BehaviorVarSig::initialize()
{
    const std::string PATH = TiltedPhoques::GetPath().string() + "/behaviors";
    if (!isDirExist(PATH))
        return;
    std::vector<std::string> dirs = loadDirs(PATH);
    for (auto item : dirs)
    {
        Sig* sig = loadSigFromDir(item);
        if (sig)
        {
            sigPool.push_back(*sig);
        }
        else
        {
            Add* add = loadAddFromDir(item);
            if (!add)
                continue;
            addPool.push_back(*add);
        }
    }
}

void BehaviorVarSig::patch(BSAnimationGraphManager* apManager, Actor* apActor)
{ /////////////////////////////////////////////////////////////////////////
    // check with animation graph holder
    ////////////////////////////////////////////////////////////////////////
    uint32_t hexFormID = apActor->formID;
    auto pExtendedActor = apActor->GetExtension();
    const AnimationGraphDescriptor* pGraph =
        AnimationGraphDescriptorManager::Get().GetDescriptor(pExtendedActor->GraphDescriptorHash);

    ////////////////////////////////////////////////////////////////////////
    // Already created
    ////////////////////////////////////////////////////////////////////////
    if (pGraph || failedSig.find(pExtendedActor->GraphDescriptorHash) != failedSig.end())
    {
        return;
    }

    D("actor with formID {:x} with hash of {} has modded behavior", hexFormID, pExtendedActor->GraphDescriptorHash);

    ////////////////////////////////////////////////////////////////////////
    // getVar
    ////////////////////////////////////////////////////////////////////////
    auto dumpVar = apManager->DumpAnimationVariables(false);
    std::unordered_map<std::string, uint32_t> reverseMap;

    ////////////////////////////////////////////////////////////////////////
    // reverse the map
    ////////////////////////////////////////////////////////////////////////
    for (auto item : dumpVar)
    {
        reverseMap.insert({(std::string)item.second, item.first});
    }

    D("known behavior variables");
    for (auto pair : dumpVar)
    {
        D("{}:{}", pair.first, pair.second);
    }

    ////////////////////////////////////////////////////////////////////////
    // do the sig
    ////////////////////////////////////////////////////////////////////////
    for (auto sig : sigPool)
    {

        D("sig {}", sig.sigName);

        bool isSig = true;
        for (std::string sigVar : sig.sigStrings)
        {
            if (reverseMap.find(sigVar) != reverseMap.end())
            {
                continue;

                D("{} found", sig.sigName);
            }
            else
            {
                isSig = false;
                break;
            }
        }
        for (std::string negSigVar : sig.negSigStrings)
        {
            if (reverseMap.find(negSigVar) != reverseMap.end())
            {
                isSig = false;
                break;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // sig not found found
        ////////////////////////////////////////////////////////////////////////
        if (!isSig)
            continue;

        D("sig found as {}", sig.sigName);

        ////////////////////////////////////////////////////////////////////////
        // calculate hash
        ////////////////////////////////////////////////////////////////////////
        uint64_t mHash = apManager->GetDescriptorKey();

        D("sig {} has a animGraph hash of {}", sig.sigName, mHash);

        ////////////////////////////////////////////////////////////////////////
        // prepare the synced var
        ////////////////////////////////////////////////////////////////////////
        TiltedPhoques::Vector<uint32_t> boolVar;
        TiltedPhoques::Vector<uint32_t> floatVar;
        TiltedPhoques::Vector<uint32_t> intVar;

        ////////////////////////////////////////////////////////////////////////
        // fill the vector
        ////////////////////////////////////////////////////////////////////////

        D("prepraring var to sync");

        D("boolean variable");

        for (std::string var : sig.syncBooleanVar)
        {
            if (reverseMap.find(var) != reverseMap.end())
            {

                D("{}:{}", reverseMap[var], var);

                boolVar.push_back(reverseMap[var]);
            }
        }

        D("float variable");

        for (std::string var : sig.syncFloatVar)
        {
            if (reverseMap.find(var) != reverseMap.end())
            {

                D("{}:{}", reverseMap[var], var);

                floatVar.push_back(reverseMap[var]);
            }
        }

        D("integer variable");

        for (std::string var : sig.syncIntegerVar)
        {
            if (reverseMap.find(var) != reverseMap.end())
            {

                D("{}:{}", reverseMap[var], var);

                intVar.push_back(reverseMap[var]);
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // Very hacky and shouldnt be allowed
        // This is a breach in the dev code and will not be merged
        ////////////////////////////////////////////////////////////////////////

        D("building animgraph var for {0:x}", hexFormID);

        auto animGrapDescriptor = new AnimationGraphDescriptor({0}, {0}, {0});
        animGrapDescriptor->BooleanLookUpTable = boolVar;
        animGrapDescriptor->FloatLookupTable = floatVar;
        animGrapDescriptor->IntegerLookupTable = intVar;

        ////////////////////////////////////////////////////////////////////////
        // add the new graph to the var graph
        ////////////////////////////////////////////////////////////////////////
        new AnimationGraphDescriptorManager::Builder(AnimationGraphDescriptorManager::Get(), mHash,
                                                     *animGrapDescriptor);

        ////////////////////////////////////////////////////////////////////////
        // change the actor hash? is this even necessary?
        ////////////////////////////////////////////////////////////////////////
        pExtendedActor->GraphDescriptorHash = mHash;

        ////////////////////////////////////////////////////////////////////////
        // handle hard coded case
        ////////////////////////////////////////////////////////////////////////
        /*if (sig.sigName == "master")
        {
            humanoidSig = {mHash, sig};
            new AnimationGraphDescriptorManager::Builder(AnimationGraphDescriptorManager::Get(), 17585368238253125375,
                                                         *animGrapDescriptor);
        }
         else if (sig.sigName == "werewolf")
            werewolfSig = {mHash, sig};
        else if (sig.sigName == "vampire_lord")
            vampireSig = {mHash, sig};
        */
        ////////////////////////////////////////////////////////////////////////
        // take a break buddy
        ////////////////////////////////////////////////////////////////////////
        return;
    }

    // sig failed

    D("sig for actor {:x} failed with hash {}", hexFormID, pExtendedActor->GraphDescriptorHash);

    failedSig[pExtendedActor->GraphDescriptorHash] = true;
}

std::vector<std::string> BehaviorVarSig::loadDirs(const std::string& acPATH)
{
    std::vector<std::string> result;
    for (auto& p : std::filesystem::directory_iterator(acPATH))
        if (p.is_directory())
            result.push_back(p.path().string());
    return result;
}

BehaviorVarSig::Sig* BehaviorVarSig::loadSigFromDir(std::string aDir)
{

    D("creating sig");

    std::string nameVarFileDir;
    std::string sigFileDir;
    std::vector<std::string> floatVarFileDir;
    std::vector<std::string> intVarFileDir;
    std::vector<std::string> boolVarFileDir;
    bool isHash = false;

    ////////////////////////////////////////////////////////////////////////
    // Enumerate all files in this directory
    ////////////////////////////////////////////////////////////////////////

    for (auto& p : std::filesystem::directory_iterator(aDir))
    {
        std::string path = p.path().string();
        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);

        D("base_path: {}", base_filename);

        if (base_filename.find("__name.txt") != std::string::npos)
        {
            nameVarFileDir = path;

            D("name file: {}", nameVarFileDir);
        }
        else if (base_filename.find("__sig.txt") != std::string::npos)
        {
            sigFileDir = path;

            D("sig file: {}", path);
        }
        else if (base_filename.find("__float.txt") != std::string::npos)
        {
            floatVarFileDir.push_back(path);

            D("float file: {}", path);
        }
        else if (base_filename.find("__int.txt") != std::string::npos)
        {
            intVarFileDir.push_back(path);

            D("int file: {}", path);
        }
        else if (base_filename.find("__bool.txt") != std::string::npos)
        {
            boolVarFileDir.push_back(path);

            D("bool file: {}", path);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // sanity check
    ////////////////////////////////////////////////////////////////////////
    if (nameVarFileDir == "" || sigFileDir == "")
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////
    // read the files
    ////////////////////////////////////////////////////////////////////////
    std::string name = "";
    std::vector<std::string> sig;
    std::vector<std::string> negSig;
    std::set<std::string> floatVar;
    std::set<std::string> intVar;
    std::set<std::string> boolVar;

    // read name var
    std::string tempString;
    std::ifstream file(nameVarFileDir);
    getline(file, tempString);
    name = tempString;
    removeWhiteSpace(name);
    file.close();
    if (name == "")
        return nullptr;

    // read sig var

    D("creating sig for {}", name);

    std::ifstream file1(sigFileDir);
    while (std::getline(file1, tempString))
    {
        removeWhiteSpace(tempString);
        if (tempString.find("~") != std::string::npos)
        {
            negSig.push_back(tempString.substr(tempString.find("~") + 1));

            D("~{}:{}", name, tempString.substr(tempString.find("~") + 1));
        }
        else
        {
            sig.push_back(tempString);

            D("{}:{}", name, tempString);
        }
    }
    file1.close();
    if (sig.size() < 1)
    {
        return nullptr;
    }

    D("reading float var", name, tempString);

    // read float var
    for (auto item : floatVarFileDir)
    {
        std::ifstream file2(item);
        while (std::getline(file2, tempString))
        {
            removeWhiteSpace(tempString);
            floatVar.insert(tempString);

            D(tempString);
        }
        file2.close();
    }

    D("reading int var", name, tempString);

    // read int var
    for (auto item : intVarFileDir)
    {
        std::ifstream file3(item);
        while (std::getline(file3, tempString))
        {
            removeWhiteSpace(tempString);
            intVar.insert(tempString);

            D(tempString);
        }
        file3.close();
    }

    D("reading bool var", name, tempString);

    // read bool var
    for (auto item : boolVarFileDir)
    {
        std::ifstream file4(item);
        while (std::getline(file4, tempString))
        {
            removeWhiteSpace(tempString);
            boolVar.insert(tempString);

            D(tempString);
        }
        file4.close();
    }

    // convert set to vector
    std::vector<std::string> floatVector;
    std::vector<std::string> intVector;
    std::vector<std::string> boolVector;

    for (auto item : floatVar)
    {
        floatVector.push_back(item);
    }

    for (auto item : intVar)
    {
        intVector.push_back(item);
    }

    for (auto item : boolVar)
    {
        boolVector.push_back(item);
    }

    // create the sig
    Sig* result = new Sig();

    result->sigName = name;
    result->sigStrings = sig;
    result->negSigStrings = negSig;
    result->syncBooleanVar = boolVector;
    result->syncFloatVar = floatVector;
    result->syncIntegerVar = intVector;

    return result;
}

BehaviorVarSig::Add* BehaviorVarSig::loadAddFromDir(std::string aDir)
{

    D("creating hash patch");

    std::string mHashFileDir;
    std::vector<std::string> floatVarFileDir;
    std::vector<std::string> intVarFileDir;
    std::vector<std::string> boolVarFileDir;
    bool isHash = false;

    ////////////////////////////////////////////////////////////////////////
    // Enumerate all files in this directory
    ////////////////////////////////////////////////////////////////////////

    for (auto& p : std::filesystem::directory_iterator(aDir))
    {
        std::string path = p.path().string();
        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);

        D("base_path: {}", base_filename);

        if (base_filename.find("__hash.txt") != std::string::npos)
        {
            mHashFileDir = path;

            D("hash file: {}", mHashFileDir);
        }
        else if (base_filename.find("__float.txt") != std::string::npos)
        {
            floatVarFileDir.push_back(path);

            D("float file: {}", path);
        }
        else if (base_filename.find("__int.txt") != std::string::npos)
        {
            intVarFileDir.push_back(path);

            D("int file: {}", path);
        }
        else if (base_filename.find("__bool.txt") != std::string::npos)
        {
            boolVarFileDir.push_back(path);

            D("bool file: {}", path);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // sanity check
    ///////////////////////////////////////////////////////////////////////

    if (mHashFileDir == "")
        return nullptr;

    ////////////////////////////////////////////////////////////////////////
    // try getting the hash
    ////////////////////////////////////////////////////////////////////////
    std::uint64_t hash;
    std::set<uint32_t> floatVar;
    std::set<uint32_t> intVar;
    std::set<uint32_t> boolVar;

    // read name var
    std::string tempString;
    std::ifstream file(mHashFileDir);
    getline(file, tempString);
    removeWhiteSpace(tempString);
    file.close();
    if (tempString == "")
        return nullptr;

    try
    {
        std::stringstream ss(tempString);
        if ((ss >> hash).fail() || !(ss >> std::ws).eof())
        {
            throw std::bad_cast();
        }

        D("hash found: {}", hash);
    }
    catch (const std::bad_cast& e)
    {

        D("bad hash inputed: {}", tempString);

        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////
    // read the files
    ////////////////////////////////////////////////////////////////////////

    D("reading float var");

    // read float var
    for (auto item : floatVarFileDir)
    {
        std::ifstream file2(item);
        while (std::getline(file2, tempString))
        {
            removeWhiteSpace(tempString);
            try
            {
                uint32_t temp;
                std::stringstream ss(tempString);
                if ((ss >> temp).fail() || !(ss >> std::ws).eof())
                {
                    throw std::bad_cast();
                }
                floatVar.insert(temp);

                D(tempString);
            }
            catch (const std::bad_cast& e)
            {
                continue;
            }
        }
        file2.close();
    }

    D("reading int var");

    // read int var
    for (auto item : intVarFileDir)
    {
        std::ifstream file3(item);
        while (std::getline(file3, tempString))
        {
            removeWhiteSpace(tempString);
            try
            {
                uint32_t temp;
                std::stringstream ss(tempString);
                if ((ss >> temp).fail() || !(ss >> std::ws).eof())
                {
                    throw std::bad_cast();
                }
                intVar.insert(temp);

                D(tempString);
            }
            catch (const std::bad_cast& e)
            {
                continue;
            }
        }
        file3.close();
    }

    D("reading bool var");

    // read bool var
    for (auto item : boolVarFileDir)
    {
        std::ifstream file4(item);
        while (std::getline(file4, tempString))
        {
            removeWhiteSpace(tempString);
            try
            {
                uint32_t temp;
                std::stringstream ss(tempString);
                if ((ss >> temp).fail() || !(ss >> std::ws).eof())
                {
                    throw std::bad_cast();
                }
                boolVar.insert(temp);

                D(tempString);
            }
            catch (const std::bad_cast& e)
            {
                continue;
            }
        }
        file4.close();
    }

    // create the add
    Add* result = new Add;
    result->mHash = hash;
    result->syncBooleanVar.assign(boolVar.begin(), boolVar.end());
    result->syncFloatVar.assign(floatVar.begin(), floatVar.end());
    result->syncIntegerVar.assign(intVar.begin(), intVar.end());

    return result;
}

void BehaviorVarSig::patchAdd(BehaviorVarSig::Add& aAdd)
{

    D("patching hash of {}", aAdd.mHash);

    const AnimationGraphDescriptor* pGraph = AnimationGraphDescriptorManager::Get().GetDescriptor(aAdd.mHash);
    if (!pGraph)
    {

        D("patching hash of {} not found", aAdd.mHash);

        return;
    }

    std::map<uint32_t, bool> boolVar;
    std::map<uint32_t, bool> floatVar;
    std::map<uint32_t, bool> intVar;

    for (auto item : pGraph->BooleanLookUpTable)
    {
        boolVar.insert({item, true});
    }
    for (auto item : pGraph->FloatLookupTable)
    {
        floatVar.insert({item, true});
    }
    for (auto item : pGraph->IntegerLookupTable)
    {
        intVar.insert({item, true});
    }

    D("boolean var", aAdd.mHash);

    for (auto item : aAdd.syncBooleanVar)
    {

        D("{}", item);

        boolVar.insert({item, true});
    }

    D("float var", aAdd.mHash);

    for (auto item : aAdd.syncFloatVar)
    {
        floatVar.insert({item, true});

        D("{}", item);
    }

    D("int var", aAdd.mHash);

    for (auto item : aAdd.syncIntegerVar)
    {

        D("{}", item);

        intVar.insert({item, true});
    }

    TiltedPhoques::Vector<uint32_t> bVar;
    TiltedPhoques::Vector<uint32_t> fVar;
    TiltedPhoques::Vector<uint32_t> iVar;

    for (auto item : boolVar)
    {
        bVar.push_back(item.first);
    }
    for (auto item : floatVar)
    {
        fVar.push_back(item.first);
    }
    for (auto item : intVar)
    {
        iVar.push_back(item.first);
    }

    auto animGrapDescriptor = new AnimationGraphDescriptor({0}, {0}, {0});
    animGrapDescriptor->BooleanLookUpTable = bVar;
    animGrapDescriptor->FloatLookupTable = fVar;
    animGrapDescriptor->IntegerLookupTable = iVar;

    AnimationGraphDescriptorManager::Get().ReRegister(aAdd.mHash, *animGrapDescriptor);
    // new AnimationGraphDescriptorManager::Builder(AnimationGraphDescriptorManager::Get(), aAdd.mHash,
    // *animGrapDescriptor);
}
