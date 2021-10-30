#pragma once
#include <cstdint>

class ComponentManager;

/* A class implemeting the MIPS R5900 CPU. */
class EmotionEngine {
public:
    EmotionEngine(ComponentManager* parent);
    ~EmotionEngine() = default;

    /* CPU functionality. */
    void tick();
    void reset_state();
    void fetch_instruction();

protected:
    ComponentManager* manager;

    /* Registers. */
    uint32_t pc;
};