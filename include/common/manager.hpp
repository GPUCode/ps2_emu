#pragma once
#include <array>
#include <memory>
#include <cpu/ee.hpp>

/* This class act as the "motherboard" of sorts */
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager() = default;

    void tick();

protected:
    void read_bios();

public:
    /* Components */
    std::unique_ptr<EmotionEngine> ee;
    
    /* Memory */
    std::array<uint8_t, 4 * 1024 * 1024> bios;
};