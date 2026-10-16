/**
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
#ifndef __HPDCACHE_TEST_WBUF_THRESHOLD_SEQ_H__
#define __HPDCACHE_TEST_WBUF_THRESHOLD_SEQ_H__

#include "hpdcache_test_sequence.h"

class hpdcache_test_wbuf_threshold_seq : public hpdcache_test_sequence
{
public:
    sc_core::sc_in<bool> wbuf_empty_i;
    sc_core::sc_in<bool> mem_req_write_valid_i;
    sc_core::sc_in<bool> mem_req_write_ready_i;
    sc_core::sc_in<bool> mem_resp_write_valid_i;
    sc_core::sc_in<bool> mem_resp_write_ready_i;

    hpdcache_test_wbuf_threshold_seq(sc_core::sc_module_name nm,
                                     sc_core::sc_signal<sc_bv<3>>& threshold)
      : hpdcache_test_sequence(nm, "wbuf_threshold_seq")
      , threshold(threshold)
    {
        SC_THREAD(run);
        sensitive << clk_i.pos();
        SC_METHOD(monitor);
        sensitive << clk_i.pos();
    }

private:
    using T = hpdcache_test_transaction_req;
    sc_core::sc_signal<sc_bv<3>>& threshold;
    unsigned int write_requests = 0;
    unsigned int write_acks = 0;

#if SC_VERSION_MAJOR < 3
    SC_HAS_PROCESS(hpdcache_test_wbuf_threshold_seq);
#endif

    void monitor()
    {
        if (!rst_ni.read()) return;
        if (mem_req_write_valid_i.read() && mem_req_write_ready_i.read()) write_requests++;
        if (mem_resp_write_valid_i.read() && mem_resp_write_ready_i.read()) write_acks++;
    }

    void send(T::hpdcache_req_op_e op)
    {
        auto t = acquire_transaction<T>();
        t->req_op = op;
        t->req_addr = 0x100;
        t->req_size = 3;
        t->req_wdata = 0x1122334455667788ULL;
        t->req_be = 0xff;
        t->req_tid = allocate_id();
        t->req_phys_indexed = true;
        t->req_need_rsp = true;
        t->req_wr_policy_hint = T::HPDCACHE_WR_POLICY_WT;
        send_transaction(t, 0);
        while (ids_size()) wait();
        wait();
    }

    void check_timeout(unsigned int initial_threshold, unsigned int new_threshold)
    {
        unsigned int requests_before = write_requests;
        unsigned int acks_before = write_acks;
        threshold.write(initial_threshold);
        wait();
        send(T::HPDCACHE_REQ_STORE);
        if (initial_threshold && (wbuf_empty_i.read() || write_requests != requests_before)) {
            SC_REPORT_FATAL("wbuf_threshold", "initial write must still be OPEN");
        }

        threshold.write(new_threshold);
        for (unsigned int i = 0; i < 1000; i++) wait();
        bool drained = wbuf_empty_i.read() && write_requests == requests_before + 1
                       && write_acks == acks_before + 1;
        std::cout << (drained ? "wbuf_threshold: " : "SEQ_ERROR: wbuf_threshold: ")
                  << initial_threshold << " -> " << new_threshold
                  << " empty=" << wbuf_empty_i.read()
                  << " requests=" << write_requests - requests_before
                  << " acks=" << write_acks - acks_before << std::endl;

        // Recover on failure so that the scoreboard can check the data as well.
        send(T::HPDCACHE_REQ_CMO_FENCE);
        send(T::HPDCACHE_REQ_LOAD);
    }

    void run()
    {
        while (rst_ni == 0) wait();
        scv_bag<std::pair<int, int>> no_delay;
        no_delay.push(std::pair<int, int>(0, 0), 1);
        auto mem = get_mem_resp_model();
        mem->set_wa_ready_delay_distribution(no_delay);
        mem->set_wd_ready_delay_distribution(no_delay);
        mem->set_wb_valid_delay_distribution(no_delay);

        check_timeout(7, 0);
        check_timeout(7, 7);
        check_timeout(7, 1);
        check_timeout(0, 0);
        transaction_fifo_o->write(nullptr);
    }
};

#endif // __HPDCACHE_TEST_WBUF_THRESHOLD_SEQ_H__
