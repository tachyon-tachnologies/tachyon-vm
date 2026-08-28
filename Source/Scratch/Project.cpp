#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Scheduler.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <simdjson.h>
#include <zip.h>

using namespace simdjson;
using namespace Scratch;

void ScratchSprite::CachePointers(void) {
    auto Cache = [this](auto & Map) {   
        for(auto & Item : Map) {
            ScratchBlock * GreenFlagBlock = Item.second.get();
            TachyonAssert(GreenFlagBlock != nullptr);

            ScratchBlock * Block = GreenFlagBlock;
            ScratchBlock * LastBlock = nullptr;

            while(Block) {
                if (LastBlock) {
                    LastBlock->NextBlock_Pointer = Block;
                }
                
                auto & Inputs = Block->GetAllInputs();
                for(ScratchInput & Input : Inputs) {
                    if (Input.IsReporter() == false) {
                        continue;
                    }
                    const std::string & ReporterId = std::get<std::string>(Input.Input);
                    Input.ReporterBlock = this->GetBlockFromId(ReporterId);
                    break;
                }

                LastBlock = Block;
                Block = this->GetBlockFromId(Block->GetNextKey());
            }
        }
    };
    Cache(this->GreenFlags);
    Cache(this->ProcedureDefinitions);
    Cache(this->BroadcastReceivers);
}

void __hot ScratchSprite::CreateScript(ScratchBlock & Block) {
    ScratchScript Script(Block.GetKey(), this);
    this->Scripts.emplace_back(Script);
    Tachyon::ScriptAddReadyQueue(this->Scripts.back());
}

/**
 * Only use when initializing the scheduler.
 */
void ScratchSprite::CreateScripts(void) {
    for(auto & Item : this->GreenFlags) {
        ScratchBlock & Block = *Item.second;
        ScratchScript Script(Block.GetKey(), this);
        DebugInfo("Created script for constume '%s'\n", this->GetName().data());
        this->Scripts.emplace_back(std::move(Script));
    }
}

ScratchList * __hot ScratchSprite::GetListFromKey(const std::string & ListKey) {           
    /* check if it's local */
    auto LocalItem = this->Lists.find(ListKey);
    if (LocalItem != this->Lists.end()) {
        return &LocalItem->second;
    }

    if (this->IsStage() == true) {
        return nullptr;
    }

    /* if not, maybe it's global */
    
    ScratchSprite * Stage = Tachyon::GetStage();
    TachyonAssert(Stage != nullptr);
    auto GlobalItem = Stage->Lists.find(ListKey);
    if (unlikely(GlobalItem == Stage->Lists.end())) {
        return nullptr;
    }
    return &GlobalItem->second;
}

ScratchVariable * __hot ScratchSprite::GetVariableFromKey(const std::string & VarKey) {           
    /* check if it's local */
    auto LocalItem = this->Variables.find(VarKey.c_str());
    if (LocalItem != this->Variables.end()) {
        return &LocalItem->second;
    }

    if (unlikely(this->IsStage() == true)) {
        return nullptr;
    }

    /* global or non-existent */

    ScratchSprite * Stage = Tachyon::GetStage();
    TachyonAssert(Stage != nullptr);
    
    auto GlobalItem = Stage->Variables.find(VarKey.c_str());

    if (unlikely(GlobalItem == Stage->Variables.end())) {
        return nullptr;
    }

    return &GlobalItem->second;
}

ScratchList * __hot ScratchSprite::GetList(const std::string & ListName) {           
    auto LocalItem = this->ListKeyLUT.find(ListName.c_str());
    if (LocalItem != this->ListKeyLUT.end()) {
        return &this->Lists.at(LocalItem->second);
    }
    
    if (this->IsStage() == true) {
        return nullptr;
    }

    /* global or non-existent */

    ScratchSprite * Stage = Tachyon::GetStage();
    TachyonAssert(Stage != nullptr);

    auto GlobalItem = Stage->ListKeyLUT.find(ListName.c_str());

    if (unlikely(GlobalItem == Stage->ListKeyLUT.end())) {
        return nullptr;
    }

    return &Stage->Lists.at(GlobalItem->second);
}

ScratchVariable * __hot ScratchSprite::GetVariable(const std::string & VarName) {           
    auto LocalItem = this->VariableKeyLUT.find(VarName.c_str());
    if (LocalItem != this->VariableKeyLUT.end()) {
        return &this->Variables.at(LocalItem->second);
    }
    
    if (this->IsStage() == true) {
        return nullptr;
    }

    /* global or non-existent */

    ScratchSprite * Stage = Tachyon::GetStage();
    TachyonAssert(Stage != nullptr);

    auto GlobalItem = Stage->VariableKeyLUT.find(VarName.c_str());

    if (unlikely(GlobalItem == Stage->VariableKeyLUT.end())) {
        return nullptr;
    }

    return &Stage->Variables.at(GlobalItem->second);
}

int ScratchProject::ParseContents(void) {
    /* get file size */
    struct zip_stat ProjectStat;
    zip_stat(this->ProjectZip, "project.json", ZIP_STAT_SIZE, &ProjectStat);
    zip_file_t * ProjectJsonFile = zip_fopen(this->ProjectZip, "project.json", 0);
    if (ProjectJsonFile == nullptr) {
        this->Close();
        return -1;
    }
    char * ProjectDataPointer = (char *)malloc(ProjectStat.size);
    if (unlikely(ProjectDataPointer == nullptr)) {
        this->Close();
        return -1;
    }
    if (zip_fread(ProjectJsonFile, ProjectDataPointer, ProjectStat.size) < 0) {
        free(ProjectDataPointer);
        zip_fclose(ProjectJsonFile);
        this->Close();
        return -1;
    }
    /* convert to json */
    ondemand::parser parser;
    padded_string data(ProjectDataPointer, ProjectStat.size);
    simdjson::simdjson_result ProjectJson = parser.iterate(data);
    if (unlikely(ProjectJson.error() != error_code::SUCCESS)) {
        DebugError("Failed to load Scratch project. Exiting...\n");
        return -1;
    }
    /* begin the load chain */
    for (auto SpriteField: ProjectJson["targets"]) {
        ondemand::object SpriteObject;
        TachyonAssert(SpriteField.get_object().get(SpriteObject) == error_code::SUCCESS);
        std::unique_ptr<ScratchSprite> Sprite = std::make_unique<ScratchSprite>(SpriteObject);
        if (unlikely(Sprite->IsStage() == true)) {
            Tachyon::SetStage(Sprite.get());
        }
        this->Sprites.push_back(std::move(Sprite));
    }
    /* we are done */
    free(ProjectDataPointer);
    zip_fclose(ProjectJsonFile);
    return 0;
}
