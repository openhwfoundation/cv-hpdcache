/**
 *  Copyright 2023,2024 Commissariat a l'Energie Atomique et aux Energies Alternatives (CEA)
 *  Copyright 2025 Inria, Universite Grenoble-Alpes, TIMA
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
/**
 *  Author     : Cesar Fuguet
 *  Date       : October, 2024
 *  Description: Generic Memory model
 */
#ifndef __MEM_MODEL_H__
#define __MEM_MODEL_H__

#include <cstdint>
#include <map>

class mem_model
{
  public:
    enum mem_model_init_mode_e
    {
        MEM_MODEL_NO_INIT,
        MEM_MODEL_INIT_VALUE,
        MEM_MODEL_INIT_RANDOM
    };

    struct data_entry_t
    {
        uint64_t data; // data word
        uint8_t vmask; // byte validity mask
    };

    typedef std::map<uint64_t, data_entry_t> mem_array_t;
    typedef std::pair<uint64_t, data_entry_t> mem_array_pair_t;

    mem_model(const char *name, mem_model_init_mode_e init_mode = MEM_MODEL_INIT_RANDOM,
              uint64_t init_val = 0)
        : name_m(name), init_mode_m(init_mode), init_val_m(init_val)
    {
    }

    ~mem_model() {}

    mem_array_t *getMemPtr() { return &mem_array_m; }

    const char *getName() { return name_m.c_str(); }

    uint64_t readMemory(uint64_t word_addr, uint8_t *vmask = nullptr)
    {
        mem_array_t::const_iterator it;

        //  check if the target address was already accessed
        it = mem_array_m.find(word_addr);
        if (it != mem_array_m.end()) {
            const data_entry_t &x = it->second;
            if (vmask != nullptr)
                *vmask = x.vmask;
            return x.data;
        }

        //  addr not found
        uint64_t value;
        switch (init_mode_m) {
        case MEM_MODEL_NO_INIT:
            if (vmask != nullptr)
                *vmask = 0x00;
            return 0xdeadbeafbadcab1eull;
        case MEM_MODEL_INIT_RANDOM:
            value = ((uint64_t)rand() << 32) | (uint64_t)rand();
            break;
        case MEM_MODEL_INIT_VALUE:
            value = init_val_m;
            break;
        }
        //  if init mode selected, add the word to the memory
        data_entry_t x;
        x.data = value;
        x.vmask = 0xff;
        mem_array_m.insert(mem_array_pair_t(word_addr, x));
        if (vmask != nullptr)
            *vmask = x.vmask;
        return value;
    }

    void writeMemory(uint64_t word_addr, uint64_t data, uint8_t mask)
    {
        mem_array_t::iterator it;
        uint64_t bmask = beToMask(mask);

        //  check if the target address was already accessed
        it = mem_array_m.find(word_addr);
        if (it != mem_array_m.end()) {
            data_entry_t &x = it->second;
            x.data = (x.data & ~bmask) | (data & bmask);
            x.vmask |= mask;
            return;
        }

        //  address not found, thus add it to the memory and merge the
        //  written data with random data
        uint64_t value;
        switch (init_mode_m) {
        case MEM_MODEL_NO_INIT:
        case MEM_MODEL_INIT_RANDOM:
            value = ((uint64_t)rand() << 32) | (uint64_t)rand();
            break;
        case MEM_MODEL_INIT_VALUE:
            value = init_val_m;
            break;
        }
        data_entry_t x;
        x.data = (value & ~bmask) | (data & bmask);
        x.vmask = mask;
        mem_array_m.insert(mem_array_pair_t(word_addr, x));
    }

    uint8_t readBytemask(uint64_t word_addr)
    {
        mem_array_t::const_iterator it;

        //  check if the target address was already accessed
        it = mem_array_m.find(word_addr);
        if (it != mem_array_m.end()) {
            const data_entry_t &x = it->second;
            return x.vmask;
        }
        return 0x0;
    }

    static uint64_t beToMask(uint8_t be)
    {
        uint64_t mask;

        //  compute the write mask
        mask = 0;
        for (int j = 0; j < 8; j++) {
            if ((be >> j) & 1) {
                mask |= (0xffull) << (j * 8);
            }
        }

        return mask;
    }

  protected:
    std::string name_m;
    mem_array_t mem_array_m;
    mem_model_init_mode_e init_mode_m;
    uint64_t init_val_m;
};

#endif /* __MEM_MODEL_H__ */
// vim: ts=4 : sts=4 : sw=4 : et : tw=100 : spell : spelllang=en : fdm=marker
