
/**
 *  Copyright 2026 Inria, Universite Grenoble-Alpes, TIMA
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
/**
 *  Author     : Cesar Fuguet
 *  Date       : November, 2026
 *  Description: Class definition of the HPDCACHE test enumerated random distribution
 */
#ifndef __HPDCACHE_TEST_ENUM_DISTRIBUTION_H__
#define __HPDCACHE_TEST_ENUM_DISTRIBUTION_H__

#include <utility>
#include <random>

template <typename enum_type, class rand_gen = std::default_random_engine>
class hpdcache_test_enum_distribution
{
private:

    std::vector<enum_type> values;
    std::vector<int> weights;
    std::discrete_distribution<int> *dist;
    rand_gen rg;

public:

    hpdcache_test_enum_distribution() : dist(nullptr)
    {
    }

    ~hpdcache_test_enum_distribution()
    {
        if (dist) delete dist;
        dist = nullptr;
    }

    void add_value_weight(enum_type value, int weight)
    {
        values.push_back(value);
        weights.push_back(weight);
    }

    void init(rand_gen rg)
    {
        dist = new std::discrete_distribution<int>(weights.begin(), weights.end());
        this->rg = rg;
    }

    enum_type rand()
    {
        if (dist) return values[(*dist)(rg)];
        return static_cast<enum_type>(0);
    }
};

#endif // __HPDCACHE_TEST_ENUM_DISTRIBUTION_H__
// vim: ts=4 : sts=4 : sw=4 : et : tw=100 : spell : spelllang=en : fdm=marker
