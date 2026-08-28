#include <Tachyon/LRA.hpp>

void LinearRegAllocator::Allocate(std::vector<TinyIR::IRValue> & Values) {
    size_t StackId = 0;
    RegisterState * LastState = nullptr;
    for(size_t i = 0; i < Values.size(); i++) {
        auto & Value = Values.at(i);
        bool Allocated = false;

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

                // DebugInfo("v%d: assigned reg id %d\n", i, State.RegId);
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