#include <cpu/ee.hpp>
#include <common/manager.hpp>

EmotionEngine::EmotionEngine(ComponentManager* parent)
{
    this->manager = parent;

    /* Reset CPU state. */
    this->reset_state();
}

void EmotionEngine::tick()
{
    /* Fetch next instruction. */
    this->fetch_instruction();
}

void EmotionEngine::reset_state()
{
    /* Reset PC. */
    pc = 0xbfc00000;
}

void EmotionEngine::fetch_instruction()
{
    
}