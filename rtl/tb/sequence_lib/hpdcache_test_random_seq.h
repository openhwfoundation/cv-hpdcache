/**
 *  Copyright 2023,2024 Commissariat a l'Energie Atomique et aux Energies Alternatives (CEA)
 *  Copyright 2025 Inria, Universite Grenoble-Alpes, TIMA
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
/**
 *  Author     : Cesar Fuguet
 *  Date       : October, 2024
 *  Description: Class definition of the HPDCACHE test random sequence
 */
#ifndef __HPDCACHE_TEST_RANDOM_SEQ_H__
#define __HPDCACHE_TEST_RANDOM_SEQ_H__

#include "hpdcache_test_defs.h"
#include "hpdcache_test_sequence.h"
#include "hpdcache_test_enum_distribution.h"
#include <systemc>
#include <random>

#define HPDCACHE_TEST_SEQUENCE_ENABLE_ERROR_SEGMENTS 1
#define HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT true

class hpdcache_test_random_seq : public hpdcache_test_sequence
{
public:

    hpdcache_test_random_seq(sc_core::sc_module_name nm)
      : hpdcache_test_sequence(nm, "random_seq"),
        delay_dist(2),
        amo_sc_do_dist(.75),
        need_rsp_dist(.95),
        size_dist(0, LOG2_REQ_DATA_BYTES),
        fault_inj_dist(.01),
        fault_inj_way_dist(0, HPDCACHE_WAYS - 1),
        fault_inj_double_dist(.05),
        wr_policy_dist(),
        seg_dist(),
        op_dist()
    {
        SC_THREAD(run);
        sensitive << clk_i.pos();

        init_wr_policy();
        init_mem_segs(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        init_op_dist(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        init_fault_inj();
    }

    void init_fault_inj()
    {
#if CONF_HPDCACHE_TEST_FAULT_INJ_DIR
        fault_inj_domain_dist.add_value_weight(hpdcache_fault_injection::domain_e::CACHE_DIR, 20),
#endif
        fault_inj_domain_dist.add_value_weight(hpdcache_fault_injection::domain_e::CACHE_DAT, 80);
        fault_inj_domain_dist.init(rg);
    }

    void init_wr_policy()
    {
        wr_policy_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_AUTO, 6);
        wr_policy_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_WB, 2);
        wr_policy_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_WT, 2);
        wr_policy_dist.init(rg);
    }

    void init_mem_segs(bool amo_supported)
    {
        seg[0].set_base(0x00000000ULL);
        seg[0].set_length(0x00004000ULL);
        seg[0].set_uncached(false);
        seg[0].set_amo_supported(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        seg[0].set_wr_policy_hint(hpdcache_test_memory_segment::WR_POLICY_RANDOM);

        seg[1].set_base(0x40004000ULL);
        seg[1].set_length(0x00004000ULL);
        seg[1].set_uncached(false);
        seg[1].set_amo_supported(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        seg[1].set_wr_policy_hint(hpdcache_test_memory_segment::WR_POLICY_WB);

        seg[2].set_base(0x80008000ULL);
        seg[2].set_length(0x00004000ULL);
        seg[2].set_uncached(false);
        seg[2].set_amo_supported(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        seg[2].set_wr_policy_hint(hpdcache_test_memory_segment::WR_POLICY_WT);

        seg[3].set_base(0xC000C000ULL);
        seg[3].set_length(0x00004000ULL);
        seg[3].set_uncached(true);
        seg[3].set_amo_supported(HPDCACHE_TEST_SEQUENCE_AMO_SUPPORT);
        seg[3].set_wr_policy_hint(hpdcache_test_memory_segment::WR_POLICY_AUTO);

        seg_dist.add_value_weight(0, 33);
        seg_dist.add_value_weight(1, 33);
        seg_dist.add_value_weight(2, 33);
        seg_dist.add_value_weight(3, 1);
        seg_dist.init(rg);
    }

    void init_op_dist(bool amo_supported)
    {
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_LOAD, 800);

#if (CONF_HPDCACHE_WT_ENABLE || CONF_HPDCACHE_WB_ENABLE)
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_STORE, 600);
        if (amo_supported) {
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_LR, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_SC, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_SWAP, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_ADD, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_AND, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_OR, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_XOR, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_MAX, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_MAXU, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_MIN, 4);
            op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_MINU, 4);
        }
#endif

        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_FENCE, 10);
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_PREFETCH, 10);

#if !defined(CONF_HPDCACHE_WB_ENABLE) || (CONF_HPDCACHE_WB_ENABLE == 0)
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_INVAL_NLINE, 10);
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_INVAL_ALL, 1);
#endif

        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_FLUSH_NLINE, 10);
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_FLUSH_ALL, 1);
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_FLUSH_INVAL_NLINE, 10);
        op_dist.add_value_weight(hpdcache_test_transaction_req::HPDCACHE_REQ_CMO_FLUSH_INVAL_ALL, 1);
        op_dist.init(rg);
    }

private:
    using wr_policy_e = hpdcache_test_transaction_req::hpdcache_wr_policy_hint_e;
    using op_e = hpdcache_test_transaction_req::hpdcache_req_op_e;
    using fault_domain_e = hpdcache_fault_injection::domain_e;

    std::default_random_engine rg;
    std::poisson_distribution<int> delay_dist;
    std::bernoulli_distribution amo_sc_do_dist;
    std::bernoulli_distribution need_rsp_dist;
    hpdcache_test_enum_distribution<wr_policy_e> wr_policy_dist;
    hpdcache_test_enum_distribution<int> seg_dist;
    hpdcache_test_enum_distribution<op_e> op_dist;

    hpdcache_test_sequence::hpdcache_test_memory_segment seg[4];
    uniform_int_distribution<uint32_t> data_dist;
    uniform_int_distribution<int> size_dist;

    std::bernoulli_distribution fault_inj_dist;
    uniform_int_distribution<int> fault_inj_way_dist;
    hpdcache_test_enum_distribution<fault_domain_e> fault_inj_domain_dist;
    std::bernoulli_distribution fault_inj_double_dist;

    static constexpr unsigned int REQ_DATA_BYTES = HPDCACHE_REQ_DATA_WIDTH / 8;
    static constexpr unsigned int LOG2_REQ_DATA_BYTES = HPDCACHE_TEST_DEFS_LOG2(REQ_DATA_BYTES);

#if SC_VERSION_MAJOR < 3
    SC_HAS_PROCESS(hpdcache_test_random_seq);
#endif

    inline sc_bv<HPDCACHE_REQ_DATA_WIDTH> create_random_data()
    {
        sc_uint<HPDCACHE_REQ_DATA_WIDTH> ret;
        for (int i = 0; i < HPDCACHE_REQ_DATA_WIDTH/32; i++) {
            ret.range((i+1)*32-1, i*32) = data_dist(rg);
        }
        return sc_bv<HPDCACHE_REQ_DATA_WIDTH>(ret);
    }

    inline uint32_t create_random_size(bool is_amo)
    {
        uint32_t ret;
        ret = size_dist(rg);
        if (is_amo) {
            return (ret >= 3) ? 3 : 2;
        }
        return ret;
    }

    std::shared_ptr<hpdcache_test_transaction_req> create_random_transaction(uint64_t addr)
    {
        std::shared_ptr<hpdcache_test_transaction_req> t;

        bool seg_wr_policy_random;
        hpdcache_test_memory_segment::wr_policy_e seg_wr_policy;
        int segn;

        while (!is_available_id()) wait();

        segn = seg_dist.rand();

        seg_wr_policy = seg[segn].get_wr_policy_hint();
        seg_wr_policy_random = (seg_wr_policy == hpdcache_test_memory_segment::WR_POLICY_RANDOM);

        t = acquire_transaction<hpdcache_test_transaction_req>();

        //  Select operation
        t->req_op = op_dist.rand();
        t->req_wdata = create_random_data();
        t->req_sid = 0;
        t->req_tid = allocate_id();
        t->req_abort = false;
        t->req_phys_indexed = false;

        //  Select write policy
        if (seg_wr_policy_random) {
            t->req_wr_policy_hint = wr_policy_dist.rand();
        } else if (seg_wr_policy == hpdcache_test_memory_segment::WR_POLICY_WB) {
            t->req_wr_policy_hint = hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_WB;
        } else if (seg_wr_policy == hpdcache_test_memory_segment::WR_POLICY_WT) {
            t->req_wr_policy_hint = hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_WT;
        } else {
            t->req_wr_policy_hint = hpdcache_test_transaction_req::HPDCACHE_WR_POLICY_AUTO;
        }

        //  Select address and size
        bool req_is_amo = t->is_amo() || t->is_amo_sc() || t->is_amo_lr();
        uint32_t sz = create_random_size(req_is_amo);
        uint64_t base = seg[segn].get_base();
        uint64_t length = seg[segn].get_length();
        uint32_t bytes = 1 << sz;
        uint64_t address = ((base + (addr % length)) / bytes) * bytes;

        t->req_addr = address;
        if (t->is_cmo()) {
            t->req_be = 0;
            t->req_size = 0;
            t->req_uncacheable = false;
            t->req_need_rsp = need_rsp_dist(rg);
        } else {
            uint32_t offset = address % REQ_DATA_BYTES;
            t->req_be = ((1UL << bytes) - 1) << offset;
            t->req_size = sz;
            t->req_uncacheable = seg[seg_dist.rand()].is_uncached() ? 1 : 0;
            if (req_is_amo) {
                t->req_need_rsp = true;
            } else {
                t->req_need_rsp = need_rsp_dist(rg);
            }
        }

#if CONF_HPDCACHE_TEST_FAULT_INJ
        if (fault_inj_dist(rg)) {
            t->req_fault.valid = true;
            t->req_fault.set = t->get_cache_set();
            t->req_fault.way = fault_inj_way_dist(rg);
            t->req_fault.word = t->get_cache_word();
            t->req_fault.domain = fault_inj_domain_dist.rand();

#if CONF_HPDCACHE_TEST_DOUBLE_FAULT_INJ
            t->req_fault.fault_mask = fault_inj_double_dist(rg) ? 0xA0 : 0x20;
#else
            t->req_fault.fault_mask = 0xA0;
#endif
        }
#endif

        return t;
    }

    std::shared_ptr<hpdcache_test_transaction_req> create_sc_transaction(uint64_t addr,
                                                                         bool uncacheable)
    {
        std::shared_ptr<hpdcache_test_transaction_req> t;

        while (!is_available_id()) wait();

        uint32_t sz = create_random_size(true);
        uint32_t bytes = 1 << sz;
        uint64_t address = (addr / bytes) * bytes;
        uint32_t offset = address % REQ_DATA_BYTES;

        t = acquire_transaction<hpdcache_test_transaction_req>();
        t->req_op = hpdcache_test_transaction_req::HPDCACHE_REQ_AMO_SC;
        t->req_wdata = create_random_data();
        t->req_sid = 0;
        t->req_tid = allocate_id();
        t->req_addr = address;
        t->req_be = ((1UL << bytes) - 1) << offset;
        t->req_size = sz;
        t->req_uncacheable = uncacheable;
        t->req_need_rsp = true;

        return t;
    }

    void run()
    {

#if HPDCACHE_TEST_SEQUENCE_ENABLE_ERROR_SEGMENTS
        if (hpdcache_test_sequence::mem_resp_model) {
            hpdcache_test_sequence::mem_resp_model->add_error_segment(
                hpdcache_test_mem_resp_model_base::segment_t(0x00000000ULL, 0x00000200ULL, true));
            hpdcache_test_sequence::mem_resp_model->add_error_segment(
                hpdcache_test_mem_resp_model_base::segment_t(0x40004000ULL, 0x40004200ULL, true));
            hpdcache_test_sequence::mem_resp_model->add_error_segment(
                hpdcache_test_mem_resp_model_base::segment_t(0xC000C000ULL, 0xC000C200ULL, true));
        }
#endif

        while (rst_ni == 0) wait();

        wait();

        uniform_int_distribution<uint64_t> addr;
        uniform_int_distribution<int> lrsc_inbetween_instrs_dist(0, 10);

        for (size_t n = 0; n < this->max_transactions; n++) {
            std::shared_ptr<hpdcache_test_transaction_req> t;
            t = create_random_transaction(addr(rg));

            if (t->is_amo_lr()) {
                hpdcache_test_transaction_req prev_lr = *t;
                send_transaction(t, delay_dist(rg));

                if (amo_sc_do_dist(rg)) {
                    for (int i = 0; i < lrsc_inbetween_instrs_dist(rg); i++) {
                        t = create_random_transaction(addr(rg));
                        send_transaction(t, delay_dist(rg));
                    }

                    t = create_sc_transaction(prev_lr.req_addr.to_uint64(),
                                              prev_lr.req_uncacheable);
                    send_transaction(t, delay_dist(rg));
                }

                continue;
            }
            send_transaction(t, delay_dist(rg));
        }

        //  ask the driver to stop
        transaction_fifo_o->write(nullptr);
    }
};

#endif // __HPDCACHE_TEST_RANDOM_SEQ_H__
// vim: ts=4 : sts=4 : sw=4 : et : tw=100 : spell : spelllang=en : fdm=marker
