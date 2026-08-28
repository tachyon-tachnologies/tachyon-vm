#include <Tachyon/TinyIR.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Debug.hpp>

using namespace TinyIR;

void IRGenerator::BindProcedureParameters(ScratchBlock & ProcedureDefinition) {
    const ScratchInput & PrototypeInput = ProcedureDefinition.GetInput(0);

    ScratchSprite & Owner = ProcedureDefinition.GetOwnerSprite();
    const std::string & PrototypeId = std::get<std::string>(PrototypeInput.Input);
    ScratchBlock * Prototype = Owner.GetBlockFromId(PrototypeId);

    TachyonAssert(Prototype != nullptr);

    const ScratchMutation & Mutation = Prototype->GetMutation();
    TachyonAssert(Mutation.ParametersKeys.size() == Mutation.ParametersNames.size());

    for (size_t i = 0; i < Mutation.ParametersKeys.size(); i++) {
        const size_t VariableNum = this->AssignParameter();
        this->ParameterBinds.emplace(Mutation.ParametersKeys[i], VariableNum);
        this->ParameterBinds.emplace(Mutation.ParametersNames[i], VariableNum);
    }
}

void IRGenerator::DescendInputs(IROpcode & Opcode) {
    auto & InputVector = Opcode.Block->GetAllInputs();
    for(auto & Input : InputVector) {
        if (Input.IsReporter()) {
            /* result could likely be a constant value */
            if (ScratchBlock * Reporter = Input.ReporterBlock) {
                IROpcode ReporterOpcode = this->GenerateOpcode(Reporter);
                if (ReporterOpcode.GetType() == IROpcodeType::LPARAM && ReporterOpcode.GetNumInputs() != 0) {
                    Opcode.PushValue(ReporterOpcode.GetInputs()[0]);
                }
                continue;
            }
            DebugWarn("Reporter was not cached??\n");
            continue;
        }
        /* likely a constant value */
        switch(Input.GetType()) {
            case InputType::ValueInput: {
                Input_Value InputValue = std::get<Input_Value>(Input.Input);
                /* constant value */
                if (auto Value = std::get_if<BoxedValue>(&InputValue.Value)) {
                    Opcode.PushValue(this->AssignValue(*Value));
                } else if (auto VarValue = std::get_if<ScratchVariable *>(&InputValue.Value)) {
                    ScratchVariable * Variable = *VarValue;
                    auto Match = this->DataBinds.find(Variable);

                    if (Match == this->DataBinds.end()) {
                        // not found
                        DebugWarn("TODO: handle non-constant values!!\n");
                        break;
                    }

                    IRValueLocator & Locator = Match->second;
                    Opcode.PushValue(Locator.first);
                }
                break;
            }
            default: {
                DebugWarn("Unknown/unhandled input type: %d!\n", Input.GetType());
                break;
            }
        }
    }
}

IROpcode IRGenerator::GenerateOpcode(ScratchBlock * Block) {
    TachyonAssert(Block != nullptr);
    if (Block->IsProcedureCall()) {
        ScratchSprite & Owner = Block->GetOwnerSprite();
        const ScratchMutation & Mutation = Block->GetMutation();
        // TODO: check if pseudo-block
        auto SearchResult = Owner.Procedures.find(Mutation.ProcCode);
        TachyonAssertMsg(SearchResult != Owner.Procedures.end(), "Invalid procedure given when generating IR!!");

        IROpcode Opcode(Block, IROpcodeType::CALL, &SearchResult->second);
        ScratchBlock * ProcBlock = Owner.GetBlockFromId(SearchResult->second.DefinitionKey);
        Opcode.BlockLinks[0] = ProcBlock;

        this->DescendInputs(Opcode);
        DebugInfo("call \"%s\"", Mutation.ProcCode.c_str());
        for(auto Assignment : Opcode.GetInputs()) {
            /* update liveness tracker */
            IRValue & Value = this->GetVariable(Assignment);
            Value.LastUsed = this->BlockCounter;

            std::cout << ", ";
            std::cout << (Value.ParameterNum.has_value() ? "v" : "p") << Assignment;
        }
        std::cout << std::endl;
        return Opcode;
    }
    if (Block->IsProcedureDef()) {
        DebugInfo("enter\n");
        IROpcode Opcode(Block, IROpcodeType::ENTER);
        /* we are in a procedure context */
        this->ProcedureContext = true;
        this->BindProcedureParameters(*Block);
        
        return Opcode;
    }
    if (Block->IsGreenFlag()) {
        DebugInfo("start\n");
        return IROpcode(Block, IROpcodeType::START);
    }
    const std::string & BlockOpcode = Block->GetOpcode();

    if (BlockOpcode == "argument_reporter_string_number" || BlockOpcode == "argument_reporter_boolean") {
        IROpcode Opcode(Block, IROpcodeType::LPARAM);
        const ScratchField & Field = Block->GetField(0);
        const std::string & ParameterName = std::get<std::string>(Field.Value);

        auto Match = this->ParameterBinds.find(ParameterName);
        TachyonAssertMsg(Match != this->ParameterBinds.end(), "Unknown procedure parameter: %s\n", ParameterName.c_str());

        Opcode.PushValue(Match->second);
        return Opcode;
    } else if (BlockOpcode == "data_setvariableto") {
        IROpcode Opcode(Block, IROpcodeType::SET);
        this->DescendInputs(Opcode);
        auto & Inputs = Opcode.GetInputs();

        if (Inputs.empty()) {
            DebugWarn("Inputs are EMPTY!\n");
            return Opcode;
        }

        const ScratchField & Field = Block->GetField(0);

        if (auto FieldValue = std::get_if<ScratchVariable *>(&Field.Value)) {
            ScratchVariable * Variable = *FieldValue;

            auto Match = this->DataBinds.find(Variable);

            if (Match == this->DataBinds.end()) {
                size_t VariableNum = this->AssignReference(Inputs[0]);

                /* update liveness tracker */
                IRValue & Value = this->GetVariable(VariableNum);
                Value.LastUsed = this->BlockCounter;

                this->BindData(Variable, VariableNum);
                Opcode.PushValue(VariableNum);
            } else {
                auto Locator = Match->second;
                /* update liveness tracker */
                IRValue & Value = *Locator.second;
                Value.LastUsed = this->BlockCounter;

                Value.Write(this->GetVariableData(Inputs[0]));
                Opcode.PushValue(Locator.first);
            }
        }
        return Opcode;
    } else if (BlockOpcode == "looks_say") {
        
        IROpcode Opcode(Block, IROpcodeType::RCALL, IRRCallType::LOOKS_SAY);
        this->DescendInputs(Opcode);

        auto & Inputs = Opcode.GetInputs();
        DebugInfo("rcall looks_say");

        for(auto Assignment : Opcode.GetInputs()) {
            /* update liveness tracker */
            IRValue & Value = this->GetVariable(Assignment);
            Value.LastUsed = this->BlockCounter;

            std::cout << ", ";
            std::cout << (Value.ParameterNum.has_value() ? "p" : "v") << Assignment;
        }
        std::cout << std::endl;
        return Opcode;
    }
    DebugWarn("Unknown block opcode: %s\n", BlockOpcode.c_str());
    return IROpcode(Block, IROpcodeType::NOP);
}

const IRStack & IRGenerator::GenerateIR(ScratchBlock & Hat) {
    ScratchBlock * BlockPtr = &Hat;
    while(BlockPtr) {
        this->BlockStack.push_back(this->GenerateOpcode(BlockPtr));
        this->BlockCounter++;
        BlockPtr = BlockPtr->NextBlock_Pointer;
    }
    if (Hat.IsGreenFlag()) {
        DebugInfo("end\n");
        this->BlockStack.push_back(IROpcode(nullptr, IROpcodeType::END));
    } else {
        DebugInfo("leave\n");
        this->BlockStack.push_back(IROpcode(nullptr, IROpcodeType::LEAVE));
    }
    return this->BlockStack;
}