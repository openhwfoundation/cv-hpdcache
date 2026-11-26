/**
 *  Copyright 2026 Capabilities Limited
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
/**
 *  Description: Directed sequence: a request replayed from the replay table (RTAB) that hits an
 *               uncorrectable error in the directory, in write-back mode.
 *
 *               1. A load misses, and a second load to the same cacheline waits in the RTAB for
 *                  the refill (MSHR hit).
 *               2. While the refill is pending, loads to another set inject double faults into
 *                  the directory entries of all the ways of the first set. The refill rewrites
 *                  the entry of its victim way; the other ways (invalid) keep their errors.
 *               3. The second load is replayed after the refill, and hits the uncorrectable
 *                  errors. In write-back mode it ends there (with an error, but it needs no
 *                  response).
 *               4. A fence must then complete: it waits for all pending transactions, including
 *                  any left in the RTAB. A final load checks that the cache still works.
 *
 *               It requires a write-back cache with ECC, and a testbench built with fault
 *               injection in the directory (CONF_HPDCACHE_TEST_FAULT_INJ=1
 *               CONF_HPDCACHE_TEST_FAULT_INJ_DIR=1). Otherwise, it does nothing.
 */
#ifndef __HPDCACHE_TEST_ECC_RTAB_SEQ_H__
#define __HPDCACHE_TEST_ECC_RTAB_SEQ_H__

#include <systemc>

#include "hpdcache_test_defs.h"
#include "hpdcache_test_sequence.h"

class hpdcache_test_ecc_rtab_seq : public hpdcache_test_sequence
{
  public:
    hpdcache_test_ecc_rtab_seq(sc_core::sc_module_name nm)
        : hpdcache_test_sequence(nm, "ecc_rtab_seq")
    {
        SC_THREAD(run);
        sensitive << clk_i.pos();
    }

  private:
    using T = hpdcache_test_transaction_req;

    //  Cycles that the memory takes to respond to a read: enough for the faults to be injected
    //  before the refill of the first load
    static constexpr int REFILL_DELAY = 100;

    //  Cycles to wait for the fence (the cache is idle well before)
    static constexpr int FENCE_TIMEOUT = 10000;

#if SC_VERSION_MAJOR < 3
    SC_HAS_PROCESS(hpdcache_test_ecc_rtab_seq);
#endif

    static uint64_t line_addr(uint64_t tag, uint64_t set)
    {
        return (tag << (HPDCACHE_CL_OFFSET_WIDTH + HPDCACHE_SET_WIDTH)) |
               (set << HPDCACHE_CL_OFFSET_WIDTH);
    }

    std::shared_ptr<T> create_load(uint64_t addr, bool need_rsp)
    {
        while (!is_available_id())
            wait();

        auto t = acquire_transaction<T>();
        t->req_op = T::HPDCACHE_REQ_LOAD;
        t->req_addr = addr;
        t->req_size = 3;
        t->req_be = 0xffULL << (addr % (HPDCACHE_REQ_DATA_WIDTH / 8));
        t->req_wdata = 0;
        t->req_sid = 0;
        t->req_tid = allocate_id();
        t->req_need_rsp = need_rsp;
        t->req_phys_indexed = true;
        t->req_uncacheable = false;
        t->req_abort = false;
        return t;
    }

    bool wait_responses(int cycles)
    {
        for (int i = 0; (i < cycles) && ids_size(); i++)
            wait();
        return ids_size() == 0;
    }

    void run()
    {
        while (rst_ni == 0)
            wait();

#if CONF_HPDCACHE_TEST_FAULT_INJ && CONF_HPDCACHE_TEST_FAULT_INJ_DIR &&                            \
    CONF_HPDCACHE_ECC_ENABLE && CONF_HPDCACHE_WB_ENABLE
        auto mem = get_mem_resp_model();
        mem->set_ra_ready_delay(0);
        mem->set_rd_valid_delay(REFILL_DELAY);

        wait();

        const uint64_t victim_set = 1;
        const uint64_t other_set = 2;

        //  1. A load miss, then a second load to the same cacheline, which waits in the RTAB
        send_transaction(create_load(line_addr(1, victim_set), true), 2);
        send_transaction(create_load(line_addr(1, victim_set), false), 2);

        //  2. Double faults in the directory entries of all the ways of the first set, carried
        //     by loads to another set
        for (int w = 0; w < HPDCACHE_WAYS; w++) {
            auto t = create_load(line_addr(1 + w, other_set), true);
            t->req_fault.valid = true;
            t->req_fault.set = victim_set;
            t->req_fault.way = w;
            t->req_fault.word = 0;
            t->req_fault.domain = hpdcache_fault_injection::domain_e::CACHE_DIR;
            t->req_fault.fault_mask = 0xA0;
            send_transaction(t, 2);
        }

        //  3. (the replay of the second load hits the uncorrectable errors)
        //  4. A fence, then a load
        auto fence = acquire_transaction<T>();
        fence->req_op = T::HPDCACHE_REQ_CMO_FENCE;
        fence->req_tid = allocate_id();
        fence->req_need_rsp = true;
        send_transaction(fence, 0);

        if (!wait_responses(FENCE_TIMEOUT)) {
            std::cout << "SEQ_ERROR: ecc_rtab: the fence did not complete within " << FENCE_TIMEOUT
                      << " cycles" << std::endl;
            Verilated::gotFinish(true);
            return;
        }

        send_transaction(create_load(line_addr(1, victim_set), true), 0);
        if (!wait_responses(FENCE_TIMEOUT)) {
            std::cout << "SEQ_ERROR: ecc_rtab: the final load did not complete within "
                      << FENCE_TIMEOUT << " cycles" << std::endl;
            Verilated::gotFinish(true);
            return;
        }
        std::cout << "ecc_rtab: the fence and the final load completed" << std::endl;
#else
        std::cout << "ecc_rtab: skipped (requires a write-back cache with ECC, and "
                  << "CONF_HPDCACHE_TEST_FAULT_INJ=1 CONF_HPDCACHE_TEST_FAULT_INJ_DIR=1)"
                  << std::endl;
#endif

        //  ask the driver to stop
        transaction_fifo_o->write(nullptr);
    }
};

#endif // __HPDCACHE_TEST_ECC_RTAB_SEQ_H__
