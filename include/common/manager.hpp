#pragma once
#include <memory>
#include <vector>
#include <cpu/ee.hpp>

/* This class act as the "motherboard" of sorts */
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager() = default;

    void tick_components();

    /* Memory operations */
    template <typename T>
    T read_memory(uint32_t addr);

    template <typename T>
    void write_memory(uint32_t addr, T data);

protected:
    void read_bios();

public:
    /* Components */
    std::unique_ptr<EmotionEngine> ee;
    
    /* Since we have more than enough RAM, let's allocate this all at once */
    std::vector<uint8_t> memory;
};