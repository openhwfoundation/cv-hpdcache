/**
 *  Copyright 2026 Inria, Universite Grenoble-Alpes, TIMA
 *
 *  SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
 */
#ifndef __HPDCACHE_TEST_RANGE_DISTRIBUTION_H__
#define __HPDCACHE_TEST_RANGE_DISTRIBUTION_H__

#include <random>
#include <utility>
#include <vector>

template <typename value_type, class rand_gen = std::default_random_engine>
class hpdcache_test_range_distribution
{
  private:
    std::vector<std::pair<value_type, value_type>> values;
    std::vector<int> weights;
    std::discrete_distribution<size_t> selection;
    rand_gen rg;

  public:
    void add_value_weight(value_type min, value_type max, int weight)
    {
        values.emplace_back(min, max);
        weights.push_back(weight);
        selection = std::discrete_distribution<size_t>(weights.begin(), weights.end());
    }

    value_type rand()
    {
        const auto &range = values[selection(rg)];
        return std::uniform_int_distribution<value_type>(range.first, range.second)(rg);
    }
};

#endif // __HPDCACHE_TEST_RANGE_DISTRIBUTION_H__
