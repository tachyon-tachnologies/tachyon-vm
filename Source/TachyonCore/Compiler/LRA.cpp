#include <Tachyon/LRA.hpp>

void LinearRegAllocator::Allocate(std::vector<TinyIR::IRValue> & Values) {
    size_t StackId = 0;
    RegisterState * LastState = nullptr;
    for(size_t i = 0; i < Values.size(); i++) {
        bool Allocated = false;

        auto & Value = Values.at(i);

        for(auto & State : this->Registers) {
            if (likely(LastState != nullptr)) {
                if (LastState->LastUsed < Value.FirstUsed) {
                    LastState->LastUsed = 0;
                }
            }
            if (State.LastUsed < Value.FirstUsed) {
                Value.AllocatorId = State.RegId;
                Value.StackSpilled = false;

                State.LastUsed = Value.LastUsed;
                Allocated = true;
                
                LastState = &State;
                break;
            }
            /* try the next register */
        }

        if (unlikely(Allocated == false)) {
            /* spill to the stack */
            Value.AllocatorId = StackId++;
            Value.StackSpilled = true;
        }
    }
}
