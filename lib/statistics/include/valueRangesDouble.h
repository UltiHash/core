//
// Created by benjamin-elias on 4/25/22.
//

#include "valueRangesSingle.h"

#ifndef CMAKE_BUILD_DEBUG_ULTIHASH_VALUERANGESDOUBLE_H
#define CMAKE_BUILD_DEBUG_ULTIHASH_VALUERANGESDOUBLE_H

//or map<tuple<start_big_ref_a,end_big_ref_a>,map<tuple<start_big_ref_b,end_big_ref_b>,tuple<vector<tuple<goal,rest...>>,map<tuple<start_sub_ref,end_sub_ref>,vector<tuple<start_sort,end_sort>>>>>>
//if the benchTuple tuple has more than 3 values, we assume the second double map and an input with 5 values in benchTuple with the form:
//tuple<ref_a,ref_b,time,benchHardwareType_a,benchHardwareType_b>
//valueRangesDouble for 5 tuple data so binary functions and operation testing
template<UnsignedIntegral Ref0, UnsignedIntegral Ref1, UnsignedIntegral Sort, typename...Goal>
class valueRangesDouble
:virtual public std::vector<std::tuple<Ref0,valueRangesSingle<Ref1,Sort,Goal...>>>{
public:
    /*
     * structure <ref_a,ref_b,time,benchHardwareType_a,benchHardwareType_b>
     */
    template<template<class> class SequentialContainerType>
    explicit valueRangesDouble(SequentialContainerType<std::tuple<Ref0, Ref1, Sort, Goal...>> input)
            : std::vector<std::tuple<Ref0,valueRangesSingle<Ref1,Sort,Goal...>>>() {
        (void) init(input);
    }

public:
    template<template<typename> class SequentialContainerType>
    auto init(SequentialContainerType<std::tuple<Ref0,Ref1,Sort,Goal...>> input){
        this->clear();
        if (input.empty())[[unlikely]] {
            return *this;
        }
        boost::sort::pdqsort(input.begin(), input.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });
        auto beg = input.begin();
        SequentialContainerType<std::tuple<Ref1,Sort,Goal...>> buf;
        auto beg2 = beg;
        while(beg2 != input.end()){
            if(std::get<0>(*beg) == std::get<0>(*beg2))[[likely]]{
                buf.push_back(filterTupElement<0>(*beg2));
            }
            else[[unlikely]]{
                this->emplace(std::get<0>(*beg),valueRangesSingle<Ref1,Goal...>{buf});
                buf.clear();
                beg=beg2;
            }
            beg2++;
        }
        if(not buf.empty()){
            this->emplace(std::get<0>(*beg),valueRangesSingle<Ref1,Goal...>{buf});
        }
        return *this;
    }

    /*
     * This function takes the delta of each of the first dimension and also builds a delta with the second dimension
     * over them
     */
    static std::tuple<Ref0, Ref1, std::vector<std::tuple<Sort, Goal...>>>
    double_dual_range_to_point_convert(
            std::tuple<tuple_hash_lib::tuple<Ref0, Ref0>, std::vector<std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>>> input,
            Ref0 ref0, Ref1 ref1) {
        std::vector<std::tuple<Sort,Sort,Goal...>> ref0_output;
        if(std::get<1>(input).size()!=2)[[unlikely]]{
            ERROR << "The input in double_dual_range_to_point_convert did not have a two 1D arrays, but " << std::to_string(std::get<1>(input).size()) <<
                  " arrays coming in. Though there are 2 arrays needed to delta between the two the process could not be finished.";
            return std::make_tuple(ref0,ref1,ref0_output);
        }
        auto ref1D = valueRangesSingle<Ref1,Sort,Goal...>::dual_ranges_to_point_convert(std::get<1>(input),ref1);
        for(auto&item:ref1D){
            std::get<1>(item) = valueRangesSingle<Ref1,Sort,Goal...>::singleSingleDeduplicateGoals(std::get<1>(item));
        }
        std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>> doubleRangeBuffer =
                valueRangesSingle<Ref1,Sort,Goal...>::single_to_dual_ranges_merge(ref1D[0],ref1D[1]);
        auto ref2D = valueRangesSingle<Ref0,Sort,Goal...>::single_dual_range_to_point_convert(doubleRangeBuffer,ref0);
        return std::make_tuple(std::get<0>(ref2D),ref1,std::get<1>(ref2D));
    }

    /*
     * gets sub-range on exact reference
     * function to get information, outputs <Ref1_begin,Ref1_end> --> <Sort_begin, Sort_end, Goal, ...>
     */
    std::tuple<tuple_hash_lib::tuple<Ref0, Ref0>, std::vector<std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>>>
    double_get_range_at(Ref0 ref0, Ref1 ref1) {
        std::vector<std::tuple<tuple_hash_lib::tuple<Ref1,Ref1>,std::vector<std::tuple<Sort,Sort,Goal...>>>> sub_single_get_range_at;
        sub_single_get_range_at.reserve(2);
        std::vector<Ref0> buffer;
        if(this->empty())[[unlikely]]{
            ERROR << "The valueRangesSingle class is empty and cannot return the average over the entire range!" << std::endl;
            return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{0,0},sub_single_get_range_at);
        }
        bool count=false;
        auto le_buf = *std::begin(*this);
        bool end_reached = false;
        for(auto&item:*this){
            if (!count and std::get<0>(item) >= ref0)[[unlikely]]{
                count=true;
                sub_single_get_range_at.push_back(std::get<1>(item).single_get_range_at(ref1));
                buffer.push_back(std::get<0>(item));
                if(ref0 == std::get<0>(item))[[unlikely]]{
                    sub_single_get_range_at.push_back(std::get<1>(item).single_get_range_at(ref1));
                    buffer.push_back(std::get<0>(item));
                    break;
                }
                continue;
            }
            if(count and ref0 <= std::get<0>(item))[[unlikely]]{
                //save and replace candidate
                le_buf = item;
            }
            else[[likely]]{
                //write back candidate
                sub_single_get_range_at.push_back(std::get<1>(le_buf).single_get_range_at(ref1));
                buffer.push_back(std::get<0>(le_buf));
                end_reached = true;
                break;
            }
        }
        if(not end_reached)[[unlikely]]{
            sub_single_get_range_at.push_back(std::get<1>(le_buf).single_get_range_at(ref1));
            buffer.push_back(std::get<0>(le_buf));
        }
        return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{buffer[0],buffer[1]},sub_single_get_range_at);
    }

    /*
     * this function generates a double ranged list to evaluate the 1st dimension between the global averages of 2nd dimension
     */
    std::tuple<tuple_hash_lib::tuple<Ref0, Ref0>, tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    double_get_avg_range_at(Ref0 ref0)
    {
        std::vector<std::tuple<tuple_hash_lib::tuple<Ref1,Ref1>,std::vector<std::tuple<Sort,Sort,Goal...>>>> sub_single_get_range_at;
        sub_single_get_range_at.reserve(2);
        std::vector<Ref0> buffer;
        if(this->empty())[[unlikely]]{
            ERROR << "The valueRangesSingle class is empty and cannot return the average over the entire range!" << std::endl;
            auto avgs = valueRangesSingle<Ref1, Sort, Goal...>::single_dual_ranges_to_average_dual_range(sub_single_get_range_at);
            return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{0,0},std::get<0>(avgs),std::get<1>(avgs));
        }
        bool count=false;
        auto le_buf = *std::begin(*this);
        bool end_reached = false;
        for(auto&item:*this){
            if (!count and std::get<0>(item) >= ref0)[[unlikely]]{
                count=true;
                sub_single_get_range_at.push_back(std::get<1>(item).single_get_info_average_min_max_all());
                buffer.push_back(std::get<0>(item));
                if(ref0 == std::get<0>(item))[[unlikely]]{
                    sub_single_get_range_at.push_back(std::get<1>(item).single_get_info_average_min_max_all());
                    buffer.push_back(std::get<0>(item));
                    break;
                }
                continue;
            }
            if(count and ref0 <= std::get<0>(item))[[unlikely]]{
                //save and replace candidate
                le_buf = item;
            }
            else[[likely]]{
                //write back candidate
                sub_single_get_range_at.push_back(std::get<1>(le_buf).single_get_info_average_min_max_all());
                buffer.push_back(std::get<0>(le_buf));
                end_reached = true;
                break;
            }
        }
        if(not end_reached)[[unlikely]]{
            sub_single_get_range_at.push_back(std::get<1>(le_buf).single_get_info_average_min_max_all());
            buffer.push_back(std::get<0>(le_buf));
        }
        auto avgs = valueRangesSingle<Ref1, Sort, Goal...>::single_dual_ranges_to_average_dual_range(sub_single_get_range_at);
        return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{buffer[0],buffer[1]},std::get<0>(avgs),std::get<1>(avgs));
    }

    /*
     * gets the minimum and maximum range saved to the valueRangesSingle Ref0 and Ref1
     */
    std::tuple<std::tuple<Ref0, Ref0>, std::vector<std::tuple<Ref1, Ref1>>>
    min_max() {
        std::vector<std::tuple<Ref1,Ref1>> sub_min_max;
        if(this->empty())[[unlikely]]{
            return std::make_tuple(std::make_tuple(0,0),sub_min_max);
        }
        for(const auto &item:*this){
            sub_min_max.push_back(std::get<1>(item).min_max());
        }
        Ref0 min=std::get<0>(*this->begin()),max=std::get<0>(*(--this->end()));

        return std::make_tuple(std::make_tuple(min,max),sub_min_max);
    }

    /*
     * this average over all just references to the smallest and the largest entry of the measurement set
     * perfect for quick estimate over all size measurement average over min-max master range
     */

    std::tuple<tuple_hash_lib::tuple<Ref0, Ref0>, std::vector<std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>>>
    single_get_info_average_min_max_all() {
            std::list<std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>> upperRange;
            std::vector<std::tuple<tuple_hash_lib::tuple<Ref1,Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>> sub_single_get_range_at;
            if(this->empty())[[unlikely]]{
                ERROR << "The valueRangesSingle class is empty and cannot return the average over the entire range!" << std::endl;
                sub_single_get_range_at.assign(std::make_move_iterator(upperRange.begin()),std::make_move_iterator(upperRange.end()));
                return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{0,0},sub_single_get_range_at);
            }
            if(this->size()==1){
                auto beg_it=std::begin(*this);
                sub_single_get_range_at.push_back(std::get<1>(*beg_it).single_get_info_average_min_max_all());
                return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{std::get<0>(*beg_it),std::get<0>(*beg_it)},sub_single_get_range_at);
            }
            //always take two 1D averages into account to average them to the from-to range in 2D; in the inner dimension the tup<ref,vec<data>> will just contain one element
            bool first=true;
            for(auto&item:*this){
                if(first)[[unlikely]]{
                    first=false;
                    upperRange.push_back(std::get<1>(item).single_get_info_average_min_max_all());
                }
                else[[likely]]{
                    upperRange.push_back(std::get<1>(item).single_get_info_average_min_max_all());
                    std::vector<std::tuple<tuple_hash_lib::tuple<Ref1, Ref1>, std::vector<std::tuple<Sort, Sort, Goal...>>>> tmp(upperRange.begin(),upperRange.end());
                    sub_single_get_range_at.push_back(valueRangesSingle<Ref1, Sort, Goal...>::single_dual_ranges_to_average_dual_range(tmp));
                    upperRange.pop_front();
                }
            }
            Ref0 min=std::get<0>(*this->begin()),max=std::get<0>(*(--this->end()));
            return std::make_tuple(tuple_hash_lib::tuple<Ref0,Ref0>{min,max},sub_single_get_range_at);
        }
};
#endif //CMAKE_BUILD_DEBUG_ULTIHASH_VALUERANGESDOUBLE_H

