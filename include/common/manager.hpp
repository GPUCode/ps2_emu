#pragma once
#include <memory>
#include <vector>
#include <cpu/ee.hpp>

constexpr uint32_t MEMORY_RANGE = 0x20000000;

/* This class act as the "motherboard" of sorts */
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager() = default;

    void tick_components();

protected:
    void read_bios();

public:
    /* Components */
    std::unique_ptr<EmotionEngine> ee;
    
    /* Since we have more than enough RAM, let's allocate this all at once */
    std::vector<uint8_t> memory;
};