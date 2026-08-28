#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <zip.h>

#include <Tachyon/Debug.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Motion.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Scripts.hpp>
#include <Scratch/Data.hpp>
#include <simdjson.h>

using namespace NanBox;
using namespace simdjson;

namespace Scratch {

    class ScratchSprite;

    /**
     * Only works for block keys.
     * @param The block key
     * @return A 64-bit ID
     */
    inline uint64_t __hot IdToU64(std::string_view Key) {
        uint64_t IdU64 = 0;
        memcpy(&IdU64, Key.data(), std::min(Key.size(), sizeof(uint64_t)));
        return IdU64;
    }

    class ScratchAsset {
        public:
            constexpr std::string_view GetName(void) const {
                return this->Name;
            }
            constexpr std::string_view GetFilename(void) const {
                return this->Filename;
            }
        protected:
            void GetAssetInformation(ondemand::object & ObjectData) {
                ObjectData.reset();
                /*
                    name
                */
                simdjson::simdjson_result Result = ObjectData.find_field("name");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);
                /*
                    md5ext
                */
                Result = ObjectData.find_field("md5ext");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Filename) == error_code::SUCCESS);

                /*
                    dataFormat
                */
                Result = ObjectData.find_field_unordered("dataFormat");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                std::string FormatString;
                TachyonAssert(Result.get_string().get(FormatString) == error_code::SUCCESS);

                this->Format = DataFormat::UnknownFormat;
                if (FormatString == "svg") this->Format = DataFormat::SVGFormat;
                if (FormatString == "png") this->Format = DataFormat::PNGFormat;
                if (FormatString == "wav") this->Format = DataFormat::WAVFormat;

                ObjectData.reset();
            }
            std::string Name;
            std::string Filename;
            enum class DataFormat : uint8_t { UnknownFormat, SVGFormat, PNGFormat, WAVFormat } Format;
    };

    class ScratchSound : ScratchAsset {
        public:
            ScratchSound(ondemand::object ObjectData) {
                this->GetAssetInformation(ObjectData);

                TachyonAssert(this->Format == DataFormat::WAVFormat);

                /*
                    rate
                */
                simdjson::simdjson_result Result = ObjectData.find_field("rate");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_uint64().get(this->SampleRate) == error_code::SUCCESS);
                /*
                    sampleCount
                */
                Result = ObjectData.find_field("sampleCount");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_uint64().get(this->TotalSamples) == error_code::SUCCESS);
            }
        private:
            uint64_t SampleRate;
            uint64_t TotalSamples;
    };

    class ScratchCostume : ScratchAsset {
        public:
            ScratchCostume(ondemand::object ObjectData) {
                this->GetAssetInformation(ObjectData);

                TachyonAssert(this->Format != DataFormat::UnknownFormat);

                /*
                    rotationCenterX and rotationCenterY
                */
                simdjson::simdjson_result Result = ObjectData.find_field("rotationCenterX");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_double().get(this->RotationCenter.first) == error_code::SUCCESS);

                Result = ObjectData.find_field("rotationCenterY");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_double().get(this->RotationCenter.second) == error_code::SUCCESS);
            }
        private:
            ScratchPosition RotationCenter;
    };

    /**
     * Contains a scratch sprite's information.
     */
    class ScratchSprite {
        public:
            /**
             * Gets the Scratch sprite's name.
             * @return The sprite's name.
             */
            constexpr std::string_view GetName(void) {
                return this->Name;
            }
            /**
             * If true, the sprite is publicly accessible. Otherwise, it is local to the sprite.
             * @return A boolean that tells whether a variable is public or not.
             */
            constexpr bool IsStage(void) {
                return this->StageSprite;
            }
            /**
             * Gets a block from its ID.
             * @param Id The block's ID.
             * @return A pointer to the block's data.
             */
            inline ScratchBlock * __hot GetBlockFromId(const std::string & Id) {
                // it's a dud
                if (unlikely(Id.empty() == true)) {
                    return nullptr;
                }
                uint64_t IdU64 = IdToU64(Id);
                auto Item = this->Blocks.find(IdU64);
                /* regular blocks */
                if (unlikely(Item != this->Blocks.end())) {
                    return Item->second.get();
                }
                /* procedure definitions */
                Item = this->ProcedureDefinitions.find(IdU64);
                if (unlikely(Item != this->ProcedureDefinitions.end())) {
                    return Item->second.get();
                }
                /* green flag */
                auto GreenFlagItem = this->GreenFlags.find(IdU64);
                if (unlikely(GreenFlagItem != this->GreenFlags.end())) {
                    return GreenFlagItem->second.get();
                }
                DebugWarn("Block ID \"%s\" was not found\n", Id.c_str());
                return nullptr;
            }

            constexpr bool IsVisible(void) {
                return this->Visibile;
            }

            /**
             * ScratchSprite constructor.
             * @param The sprite's JSON data.
             */
            ScratchSprite(ondemand::object SpriteData) {
                /*
                    isStage
                */
                auto Result = SpriteData.find_field_unordered("isStage");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_bool().get(this->StageSprite) == error_code::SUCCESS);

                /*
                    name
                */
                Result = SpriteData.find_field_unordered("name");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);

                /*
                    visible
                */
                if (this->StageSprite == false) {
                    Result = SpriteData.find_field_unordered("visible");
                    TachyonAssert(Result.error() == error_code::SUCCESS);
                    TachyonAssert(Result.get_bool().get(this->Visibile) == error_code::SUCCESS);
                }

                /*
                    X and Y (if they're present)
                */
                if (this->StageSprite == false) {
                    auto ResultX = SpriteData.find_field_unordered("x");
                    auto ResultY = SpriteData.find_field_unordered("y");
                    TachyonAssert(ResultX.error() == error_code::SUCCESS && ResultY.error() == error_code::SUCCESS);
                    
                    TachyonAssert(ResultX.get_double().get(this->Position.first) == error_code::SUCCESS);
                    TachyonAssert(ResultY.get_double().get(this->Position.second) == error_code::SUCCESS);
                }
                /*
                    variables
                */
                Result = SpriteData.find_field_unordered("variables");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object VariableData;
                TachyonAssert(Result.get_object().get(VariableData) == error_code::SUCCESS);

                for (auto VariableField : VariableData) {
                    std::string VariableKey;
                    TachyonAssert(VariableField.escaped_key().get(VariableKey) == error_code::SUCCESS);
                    
                    ondemand::array VariableArray;
                    TachyonAssert(VariableField.value().get_array().get(VariableArray) == error_code::SUCCESS);

                    ScratchVariable Variable = ScratchVariable(VariableArray, this->StageSprite);

                    this->VariableKeyLUT.emplace(Variable.GetName(), VariableKey);
                    this->Variables.emplace(
                        VariableKey,
                        std::move(Variable)
                    );
                }
                /*
                    lists
                */
                Result = SpriteData.find_field_unordered("lists");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object ListData;
                TachyonAssert(Result.get_object().get(ListData) == error_code::SUCCESS);

                for (auto ListField : ListData) {
                    std::string ListKey;
                    TachyonAssert(ListField.escaped_key().get(ListKey) == error_code::SUCCESS);

                    ondemand::array ListArray;
                    TachyonAssert(ListField.value().get_array().get(ListArray) == error_code::SUCCESS);

                    ScratchList List = ScratchList(ListArray, this->StageSprite);

                    this->ListKeyLUT.emplace(List.GetName(), ListKey);
                    this->Lists.emplace(
                        ListKey,
                        std::move(List)
                    );
                }
                /*
                    sounds
                */
                Result = SpriteData.find_field_unordered("sounds");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::array SoundArray;
                TachyonAssert(Result.get_array().get(SoundArray) == error_code::SUCCESS);

                for (auto SoundField : SoundArray) {
                    ondemand::object SoundObject;
                    TachyonAssert(SoundField.get_object().get(SoundObject) == error_code::SUCCESS);

                    this->Sounds.emplace_back(
                        std::move(ScratchSound(SoundObject))
                    );
                }
                /*
                    costumes
                */
                Result = SpriteData.find_field_unordered("costumes");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::array CostumeArray;
                TachyonAssert(Result.get_array().get(CostumeArray) == error_code::SUCCESS);

                for (auto CostumeField : CostumeArray) {
                    ondemand::object CostumeObject;
                    TachyonAssert(CostumeField.get_object().get(CostumeObject) == error_code::SUCCESS);

                    this->Costumes.emplace_back(
                        std::move(ScratchCostume(CostumeObject))
                    );
                }
                /* 
                    blocks 
                */
                Result = SpriteData.find_field_unordered("blocks");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object BlockData;
                TachyonAssert(Result.get_object().get(BlockData) == error_code::SUCCESS);

                for (auto BlockField : BlockData) {
                    std::string BlockKey;
                    TachyonAssert(BlockField.escaped_key().get(BlockKey) == error_code::SUCCESS);

                    uint64_t BlockIdU64 = IdToU64(BlockKey);

                    ondemand::object BlockObject;
                    TachyonAssert(BlockField.value().get_object().get(BlockObject) == error_code::SUCCESS);

                    std::unique_ptr<ScratchBlock> Block = std::make_unique<ScratchBlock>(BlockKey, BlockObject, *this);

                    if (Block->IsGreenFlag()) {
                        this->GreenFlags.insert({ BlockIdU64, std::move(Block) });
                    } else if (Block->GetOpcode() == "event_whenbroadcastreceived") {
                        this->BroadcastReceivers.insert({ BlockIdU64, std::move(Block) });
                    } else if (Block->IsProcedureDef()) {
                        this->ProcedureDefinitions.insert({ BlockIdU64, std::move(Block) });
                    } else {
                        this->Blocks.insert({ BlockIdU64, std::move(Block) });
                    }
                }
                /* all blocks loaded. no missing dependencies to worry about */
                this->ResolveProcedureDefinitions();
                this->CachePointers();
                this->CreateScripts();
            }

            void CreateScript(ScratchBlock & Block);

            ScratchVariable * __hot GetVariableFromKey(const std::string & VarKey);
            ScratchList * __hot GetListFromKey(const std::string & ListKey);

            ScratchVariable * __hot GetVariable(const std::string & VarName);
            ScratchList * __hot GetList(const std::string & ListName); 

            std::map<uint64_t, std::unique_ptr<ScratchBlock>> GreenFlags;
            std::unordered_map<uint64_t, std::unique_ptr<ScratchBlock>> BroadcastReceivers;
            std::unordered_map<uint64_t, std::unique_ptr<ScratchBlock>> Blocks;
            std::unordered_map<uint64_t, std::unique_ptr<ScratchBlock>> ProcedureDefinitions;

            std::unordered_map<std::string, ScratchVariable> Variables;
            std::unordered_map<std::string, ScratchList> Lists;
            std::unordered_map<std::string, std::string> VariableKeyLUT;
            std::unordered_map<std::string, std::string> ListKeyLUT;

            std::vector<ScratchSound> Sounds;
            std::vector<ScratchCostume> Costumes;
            std::vector<ScratchScript> Scripts;
            std::unordered_map<std::string, ScratchProcedure> Procedures;
            ScratchPosition Position;

        private:
            void CreateScripts(void);
            void CachePointers(void);
            void ResolveProcedureDefinitions(void);

            std::string Name;
            bool Visibile = true;
            bool StageSprite;
    };

    /**
     * Contains a scratch project's information.
     */
    class ScratchProject {
        public:
            /**
             * ScratchProject constructor.
             * @param ZipPath The scratch project's SB3 file path.
             */
            explicit ScratchProject(std::string ZipPath) {
                this->ProjectZip = zip_open(ZipPath.c_str(), 0, nullptr);
                if (this->ProjectZip == nullptr) {
                    /* file doesn't exist */
                    return;
                }
                this->ProjectZip_Path = ZipPath;
            }

            ScratchProject() {
                this->ProjectZip = nullptr;
                this->ProjectZip_Path = {};
            }

            ~ScratchProject() {
                this->Close();
            }

            /**
             * De-initializes and closes the project and it's file.
             */
            void Close(void) {
                if (this->ProjectZip != nullptr) {
                    if (zip_close(this->ProjectZip) < 0) {
                        zip_discard(this->ProjectZip);
                    }
                }
                this->Sprites.clear();
                this->ProjectZip_Path.clear();
            }

            /**
             * Checks whether the project has been loaded.
             * @return Returns true if it has been loaded, otherwise false.
             */
            constexpr bool IsLoaded(void) const {
                return ProjectZip_Path.empty() == false;
            }

            /**
             * Parses the Scratch project into objects and prepares essential data.
             */
            int ParseContents(void);
            std::vector<std::unique_ptr<ScratchSprite>> Sprites;

        private:
            std::string ProjectZip_Path;
            zip_t * ProjectZip = nullptr;
            
            /**
             * If true, the project has been modified. Otherwise, it is false.
             */
            bool IsDirty = false;
    };
};
