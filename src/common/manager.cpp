#include <common/manager.hpp>
#include <cpu/ee/ee.hpp>
#include <cpu/iop/iop.hpp>

int cycles_executed = 0;

ComponentManager::ComponentManager()
{
    /* Allocate the entire 32MB of RAM */
    ram = new uint8_t[ee::RAM.length];
    iop_ram = new uint8_t[iop::RAM.length];
    bios = new uint8_t[ee::BIOS.length];
    std::memset(ram, 0, ee::RAM.length);
    std::memset(iop_ram, 0, iop::RAM.length);

    console.open("console.txt", std::ios::out);

    /* Load the BIOS in our memory */
    /* NOTE: Must make a GUI for this someday */
    this->read_bios();

    /* Finally construct our components */
    ee = std::make_unique<ee::EmotionEngine>(this);
    iop = std::make_unique<iop::IOProcessor>(this);
}

ComponentManager::~ComponentManager()
{
    delete[] ram;
    delete[] iop_ram;
    delete[] bios;
}

void ComponentManager::read_bios()
{
    /* Yes it's hardcoded for now, don't bite me, I'll change it eventually */
    std::ifstream reader;
    reader.open("SCPH-10000.BIN", std::ios::in | std::ios::binary);

    if (!reader.is_open())
        std::abort();
    
    reader.seekg(0);
    reader.read((char*)bios, ee::BIOS.length);
    reader.close();
}

void ComponentManager::tick()
{
    ee->tick();
    cycles_executed++;

    if (cycles_executed % 8 == 0) 
        iop->tick();
}