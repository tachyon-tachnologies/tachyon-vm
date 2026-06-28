#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Motion.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static ScratchStatus Motion_GoToXY(ScratchBlock & Block) {
    BoxedValue X_Data = Block.GetInputData(0);
    BoxedValue Y_Data = Block.GetInputData(1);

    double X = UnboxAsDouble(X_Data);
    double Y = UnboxAsDouble(Y_Data);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    Owner.Position.first = (X > 255) ? 255 : X;
    Owner.Position.second = (Y > 255) ? 255 : Y;

    return ScratchStatus::SCRATCH_NEXT;
}

void Motion::RegisterAll(void) {
    Tachyon::RegisterOpHandler("motion_gotoxy", Motion_GoToXY);
}
