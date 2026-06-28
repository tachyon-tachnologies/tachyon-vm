#include <Scratch/Scratch.hpp>
#include <Scratch/ControlFlow.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/Looks.hpp>
#include <Scratch/Motion.hpp>
#include <Scratch/Operator.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Reporters.hpp>
#include <Scratch/Sensing.hpp>
#include <Tachyon/Events.hpp>
#include <Tachyon/Tachyon.hpp>

void Scratch::RegisterAllOpcodes(void) {
    Scratch::Motion::RegisterAll();
    Scratch::Operator::RegisterAll();
    Scratch::Procedures::RegisterAll();
    Scratch::Data::RegisterAll();
    Scratch::Looks::RegisterAll();
    Scratch::ControlFlow::RegisterAll();
    Scratch::Sensing::RegisterAll();
    Scratch::Reporters::RegisterAll();
    Scratch::Events::RegisterAll();
    Tachyon::Pseudo::RegisterAll();
}