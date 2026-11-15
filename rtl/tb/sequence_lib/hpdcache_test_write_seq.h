/**
 *  Copyright 2023,2024 CEA*
 *  *Commissariat a l'Energie Atomique et aux Energies Alternatives (CEA)
 *  Copyright 2025 Inria, Universite Grenoble-Alpes, TIMA
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 *
 *  Licensed under the Solderpad Hardware License v 2.1 (the “License”); you
 *  may not use this file except in compliance with the License, or, at your
 *  option, the Apache License version 2.0. You may obtain a copy of the
 *  License at
 *
 *  https://solderpad.org/licenses/SHL-2.1/
 *
 *  Unless required by applicable law or agreed to in writing, any work
 *  distributed under the License is distributed on an “AS IS” BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 */
/**
 *  Author     : Cesar Fuguet
 *  Date       : October, 2024
 *  Description: Class definition of the HPDCACHE test random sequence
 */
#ifndef __HPDCACHE_TEST_WRITE_SEQ_H__
#define __HPDCACHE_TEST_WRITE_SEQ_H__

#include <random>
#include <systemc>

#include "hpdcache_test_defs.h"
#include "hpdcache_test_mem_resp_model_base.h"
#include "hpdcache_test_range_distribution.h"
#include "hpdcache_test_sequence.h"

class hpdcache_test_write_seq : public hpdcache_test_sequence
{
  public:
    hpdcache_test_write_seq(sc_core::sc_module_name nm) : hpdcache_test_sequence(nm, "write_seq")
    {
        SC_THREAD(run);
        sensitive << clk_i.pos();

        seg.set_base(0x00000000ULL);
        seg.set_length(0x00004000ULL);
        seg.set_uncached(false);
    }

  private:
    hpdcache_test_sequence::hpdcache_test_memory_segment seg;
    std::default_random_engine rg;
    std::uniform_int_distribution<uint32_t> data_dist;
    std::uniform_int_distribution<uint32_t> size_dist{0, 3};
    std::uniform_int_distribution<uint64_t> addr_dist;
    const unsigned int HPDCACHE_REQ_DATA_BYTES = HPDCACHE_REQ_DATA_WIDTH / 8;

#if SC_VERSION_MAJOR < 3
    SC_HAS_PROCESS(hpdcache_test_write_seq);
#endif

    inline sc_bv<HPDCACHE_REQ_DATA_WIDTH> create_random_data()
    {
        sc_uint<HPDCACHE_REQ_DATA_WIDTH> ret;
        for (int i = 0; i < HPDCACHE_REQ_DATA_WIDTH / 32; i++) {
            ret.range((i + 1) * 32 - 1, i * 32) = data_dist(rg);
        }
        return sc_bv<HPDCACHE_REQ_DATA_WIDTH>(ret);
    }

    inline uint32_t create_random_size(bool is_amo)
    {
        return is_amo ? std::uniform_int_distribution<uint32_t>(2, 3)(rg) : size_dist(rg);
    }

    void send_transaction(std::shared_ptr<hpdcache_test_transaction_req> t)
    {
        // send transaction to the driver
        transaction_fifo_o->write(t);

        // wait and consume driver acknowledgement (this is blocking)
        transaction_ret_i.read();

        // release the previously used transaction object
        release_transaction<hpdcache_test_transaction_req>(t);

        // add some random delay between two consecutive commands
    }

    std::shared_ptr<hpdcache_test_transaction_req> create_random_transaction()
    {
        std::shared_ptr<hpdcache_test_transaction_req> t;

        while (!is_available_id())
            wait();

        t = acquire_transaction<hpdcache_test_transaction_req>();
        t->req_op = hpdcache_test_transaction_req::HPDCACHE_REQ_STORE;
        t->req_wdata = create_random_data();
        t->req_sid = 0;
        t->req_tid = allocate_id();
        t->req_abort = false;
        t->req_phys_indexed = false;

        uint32_t sz = create_random_size(t->is_amo() || t->is_amo_sc() || t->is_amo_lr());

        uint64_t base = seg.get_base();
        uint64_t length = seg.get_length();
        uint32_t bytes = 1 << sz;
        uint64_t address = ((base + (addr_dist(rg) % length)) / bytes) * bytes;
        uint32_t offset = address % HPDCACHE_REQ_DATA_BYTES;

        t->req_addr = address;
        t->req_be = ((1UL << bytes) - 1) << offset;
        t->req_size = sz;
        t->req_uncacheable = seg.is_uncached() ? 1 : 0;
        t->req_need_rsp = true;

        return t;
    }

    void run()
    {
        while (rst_ni == 0)
            wait();

        std::shared_ptr<hpdcache_test_mem_resp_model_base> mem_resp_model =
            this->get_mem_resp_model();

        hpdcache_test_range_distribution<int> no_delay;
        no_delay.add_value_weight(0, 0, 1);

        mem_resp_model->set_ra_ready_delay(0);
        mem_resp_model->set_rd_valid_delay(16);
        mem_resp_model->set_wa_ready_delay_distribution(no_delay);
        mem_resp_model->set_wd_ready_delay_distribution(no_delay);
        mem_resp_model->set_wb_valid_delay_distribution(no_delay);

        wait();

        for (size_t n = 0; n < this->max_transactions; n++) {
            std::shared_ptr<hpdcache_test_transaction_req> t;
            t = create_random_transaction();
            send_transaction(t);
        }

        //  ask the driver to stop
        transaction_fifo_o->write(nullptr);
    }
};

#endif // __HPDCACHE_TEST_WRITE_SEQ_H__
