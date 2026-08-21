#include <Tachyon/TinyIR.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Debug.hpp>

using namespace TinyIR;

void IRGenerator::PrintIR(void) {
    for(auto Opcode : this->BlockStack) {
        DebugInfo("IR Opcode: %d, total inputs: %d\n", Opcode.GetType(), Opcode.GetNumInputs());
    }
}

void IRGenerator::DescendInputs(IROpcode & Opcode) {
    const size_t TotalInputs = Opcode.Block->GetNumInputs();
    auto InputVector = Opcode.Block->GetAllInputs();
    for(size_t i = 0; i < TotalInputs; i++) {
        ScratchInput & Input = InputVector.at(i);
        if (Input.IsReporter()) {
            /* result could likely be a constant value, but we'll get to this later */
            continue;
        }
        /* likely a constant value */
        if (Input.IsType(InputType::ValueInput)) {
            Input_Value InputValue = std::get<Input_Value>(Input.Input);
            /* constant value */
            if (auto Value = std::get_if<BoxedValue>(&InputValue.Value)) {
                IRValue IRValue(*Value);

                IRValue.FirstUse = this->BlockCounter;
                IRValue.LastUse = IRValue.FirstUse;

                DebugInfo("Found constant value: 0x%016zx\n", *Value);
                Opcode.PushValue(i, IRValue);
            }
        }
    }
}

IROpcode IRGenerator::GenerateOpcode(ScratchBlock * Block) {
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
        DebugInfo("Found call to procedure: %s\n", Mutation.ProcCode.c_str());
        return Opcode;
    }
    if (Block->IsProcedureDef()) {
        return IROpcode {Block, IROpcodeType::ENTER};
    }
    const std::string & BlockOpcode = Block->GetOpcode();
    if (BlockOpcode == "event_whenflagclicked") {
        return IROpcode {Block, IROpcodeType::START};
    }
    DebugInfo("Unknown block opcode: %s\n", BlockOpcode.c_str());
    return IROpcode {Block, IROpcodeType::NOP};
}

const IRStack & IRGenerator::GenerateIR(ScratchBlock & Hat) {
    ScratchBlock * BlockPtr = &Hat;
    while(BlockPtr) {
        this->BlockStack.push_back(this->GenerateOpcode(BlockPtr));
        this->BlockCounter++;
        BlockPtr = BlockPtr->NextBlock_Pointer;
    }
    this->BlockStack.push_back(IROpcode {nullptr, IROpcodeType::END});
    this->PrintIR();
    return this->BlockStack;
}