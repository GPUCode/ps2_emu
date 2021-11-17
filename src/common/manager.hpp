#pragma once
#include <memory>
#include <vector>
#include <cpu/ee.hpp>

/* This class act as the "motherboard" of sorts */
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager();

    void tick();

    /* Memory operations */
    template <typename T>
    T read(uint32_t addr);

    template <typename T>
    void write(uint32_t addr, T data);

protected:
    void read_bios();

public:
    /* Components */
    std::unique_ptr<EmotionEngine> ee;
    
    /* Memory - Registers */
    uint8_t* ram, * bios;

    uint32_t MCH_RICM = 0, MCH_DRD = 0;
    uint8_t rdram_sdevid = 0;
};