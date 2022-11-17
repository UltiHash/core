//
// Created by benjamin-elias on 1/2/22.
//

#ifndef ULTIHASH_VALUERANGE_H
#define ULTIHASH_VALUERANGE_H

#include "hashLibraries.h"

//hardware_name,hardware_count,container_variant
using benchHardwareType = typename tuple_hash_lib::tuple<std::string, unsigned long long int, unsigned long long int>;

//overlapping ranges pointing to ordered goal data with linear start and end times_measurement
//WARNING: start point for benchmark is never 0!!
//Ref0 is size of container, Sort is time and Goal was earlier only container variant, but now it can be any hardware configuration
//desired structure:
//map<tuple<start_big_ref,end_big_ref>,tuple<vector<tuple<goal,rest...>>,map<tuple<start_sub_ref,end_sub_ref>,vector<tuple<start_sort,end_sort>>>>>
//valueRangesSingle for unary operation testing but 3+ data entries in one tuple
//old: std::size_t Ref0 = 0, std::size_t Sort = 1, std::size_t Goal = 2

/*
 * valueRangesSingle contains the linearization of time sorted Hardware components measurements structured by ranges
 * of the same order of Hardware performance
 */
template<UnsignedIntegral Ref, UnsignedIntegral Sort, typename...Goal>
class valueRangesSingle
: virtual public boost::unordered_map<tuple_hash_lib::tuple<Ref, Ref>,
                std::tuple<
                        std::vector<std::tuple<Goal...>>,
                        boost::unordered_map<
                                tuple_hash_lib::tuple<Ref, Ref>,
                                std::vector<std::tuple<Sort,Sort>>
                        >
                >
        >{
//usual input is std::vector<unsigned long long, unsigned long long, unsigned long long,...> as vector<Ref0,Sort,Goal,...Rest as extended goal>
public:
    //constructor structure <ref_a,time,benchHardwareType>
    template<template<class> class SequentialContainerType>
    explicit valueRangesSingle(SequentialContainerType<std::tuple<Ref, Sort, Goal...>> input)
            : boost::unordered_map<tuple_hash_lib::tuple<Ref, Ref>,
            std::tuple<
                    std::vector<std::tuple<Goal...>>,
                    boost::unordered_map<
                            tuple_hash_lib::tuple<Ref, Ref>,
                            std::vector<std::tuple<Sort,Sort>>
                    >
            >
    >() {
        (void) this->init(input);
    }
    /*
     * this function has to check and map duplicate <goal, rest...> and average their times and re-sort
     */
    static std::vector<std::tuple<Sort, Goal...>>
    singleSingleRangeDeduplicateGoals(std::vector<std::tuple<Sort, Goal...>> sortedGoalList) {
        //sort times
        boost::sort::pdqsort(sortedGoalList.begin(), sortedGoalList.end(), [](auto a, auto b) -> bool{
            return std::get<0>(a) < std::get<0>(b);
        });
        std::vector<std::tuple<Sort, Goal...>> sort_goal_tuples;
        std::vector<std::tuple<std::vector<Sort>,Goal...>>goalTimes;
        for(const auto &item2:sortedGoalList){
            bool toBeContinued=false;
            for (auto &item3:goalTimes) {
                if(filterTupElement<0>(item3) == filterTupElement<0>(item2))[[unlikely]]{
                    std::get<0>(item3).push_back(std::get<0>(item2));
                    toBeContinued= true;
                    break;
                }
            }
            if(toBeContinued)[[unlikely]]{
                toBeContinued=false;
                continue;
            }
            goalTimes.emplace_back(std::vector<Sort>{std::get<0>(item2)},filterTupElement<0>(item2));
        }
        //average times and push to sort_goal_tuples
        for(const auto&item4:goalTimes){
            sort_goal_tuples.push_back(std::tuple_cat((Sort)std::accumulate(std::get<0>(item4).begin(), std::get<0>(item4).end(), 0) / std::get<0>(item4).size(),
                                                      filterTupElement<0>(item4)));
        }
        //sort result vector "sort_goal_tuples" for times_measurement and remember saved vector. The to emplace sequence is set if the order is not the same anymore
        boost::sort::pdqsort(sort_goal_tuples.begin(), sort_goal_tuples.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });
        return sort_goal_tuples;
    }

    /*
     * first read of any sequential container delivering the measurement tuples
     */
    template<template<typename> class SequentialContainerType>
    std::vector<std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>>>
    singleSingleRangeInit(SequentialContainerType<std::tuple<Ref,Sort,Goal...>> input, Ref min_ref){
        //tmp contains all different goals and rest tuple combinations within one range
        std::vector<std::tuple<Sort,Goal...>> tmp{};
        //emplace back tuple_hash_lib::tuple<sort_start,sort_end>,vector<sort> from sort_goal_tuples and filter its order to be set into an unordered map
        //vector<tuple<sort, goal, rest...>>
        std::vector<std::tuple<Ref,std::vector<std::tuple<Sort,Goal...>>>> singleRangeBuffer;
        //query the average off all these results and their takes since the size and type of the incoming data will not be analyzed yet
        boost::sort::pdqsort(input.begin(), input.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });
        for (const auto &item: input) {
            tmp.push_back(filterTupElement<0>(item));
            //if one range is set, work through the available goals and finalize
            if (std::get<0>(item) != min_ref)[[unlikely]] {
                //analyse the length found, sort by Goal type and reduce to average times_measurement for each goal type
                //output vector with tuple<time,goal,rest...> where there is only one combination of tuple<goal, rest...> left, filtering multiple takes
                //give one size to function with various times and the goals for that
                std::vector<std::tuple<Sort,Goal...>> sort_goal_tuples = singleSingleRangeDeduplicateGoals(tmp);
                //now we know all unique tuple<sort,goal,rest...> and their minimal times_measurement
                //save the times_measurement in order and also write them back to the single Range buffer, so they can be set to average over all single ranges
                //only compare orders from data between two indexes
                if (!std::any_of(singleRangeBuffer.begin(),singleRangeBuffer.end(),[min_ref](auto a){return std::get<0>(a)==min_ref;}))[[likely]]{
                    singleRangeBuffer.emplace_back(min_ref, sort_goal_tuples);
                } else[[unlikely]]{//DEBUG, should never reach this branch
                    ERROR << "The forbidden branch in valueRangesSingle for singleBufferRange was reached! The deduplication failed for some reason!";
                    std::get<1>(singleRangeBuffer.at(min_ref)) = sort_goal_tuples;
                }
                min_ref = std::get<0>(item);
                tmp.clear();
            }
        }
        if(not tmp.empty())[[likely]]{
            //analyse the length found, sort by Goal type and reduce to average times_measurement for each goal type
            //output vector with tuple<time,goal,rest...> where there is only one combination of tuple<goal, rest...> left, filtering multiple takes
            //give one size to function with various times and the goals for that
            std::vector<std::tuple<Sort,Goal...>> sort_goal_tuples = singleSingleRangeDeduplicateGoals(tmp);
            //now we know all unique tuple<sort,goal,rest...> and their minimal times_measurement
            //save the times_measurement in order and also write them back to the single Range buffer, so they can be set to average over all single ranges
            //only compare orders from data between two indexes
            if (!std::any_of(singleRangeBuffer.begin(),singleRangeBuffer.end(),[min_ref](auto a){return std::get<0>(a)==min_ref;}))[[likely]]{
                singleRangeBuffer.emplace_back(min_ref, sort_goal_tuples);
            } else[[unlikely]]{//DEBUG, should never reach this branch
                ERROR << "The forbidden branch in valueRangesSingle for singleBufferRange was reached! The deduplication failed for some reason!";
                std::get<1>(singleRangeBuffer.at(min_ref)) = sort_goal_tuples;
            }
        }
        return singleRangeBuffer;
    }

    /*
     * while measurements usually are a single point we now want to build linear lines between the measuring points
     * to build ranges
     */
    boost::unordered_map<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    calcSingleDoubleRangeBuffer(
            std::vector<std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>>> singleRangeBuffer) {
        //current structure of singleRangeBuffer: unordered_map<ref,vector<tuple<sort,goal,rest...>>>
        //speedup with ordered singleRangeBufferLookup vector<ref>
        //now we need to intermediate the single ranges to double ranges with sort_start_sort_end
        //for every count between start and end we will calculate the intermediate between start and endpoint and sort the order
        boost::sort::pdqsort(singleRangeBuffer.begin(), singleRangeBuffer.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });

        boost::unordered_map<tuple_hash_lib::tuple<Ref,Ref>,std::vector<std::tuple<Sort,Sort,Goal...>>> doubleRangeBuffer{};
        if(singleRangeBuffer.empty()){
            return doubleRangeBuffer;
        }

        auto lastSingleRange_it = singleRangeBuffer.begin();
        auto singleRange_it = lastSingleRange_it;
        singleRange_it++;

        while (singleRange_it != singleRangeBuffer.end()) {
            //desired format: unordered_map<tuple_hash_lib::tuple<start_ref,end_ref>,vector<tuple<start_sort,end_sort,goal,rest...>>>
            //look for a merge by finding a tuple<goal,rest...> to get start_sort and end_sort
            for (const auto &item1: lastSingleRange_it->second) {
                for (const auto &item2: singleRange_it->second) {
                    if (filterTupElement<0>(item1) == filterTupElement<0>(item2)) {
                        auto insert_tup = tuple_hash_lib::tuple<Ref,Ref>{lastSingleRange_it->first, singleRange_it->first};
                        if (doubleRangeBuffer.find(insert_tup) == doubleRangeBuffer.end()) {
                            doubleRangeBuffer.emplace(insert_tup, std::vector<std::tuple<Sort,Sort,Goal...>>{
                                    std::tuple_cat(std::make_tuple(std::get<0>(item1), std::get<0>(item2)),filterTupElement<0>(item1))});
                        } else {
                            doubleRangeBuffer.at(insert_tup).push_back(
                                    std::tuple_cat(std::make_tuple(std::get<0>(item1), std::get<0>(item2)),filterTupElement<0>(item1)));
                        }
                        break;
                    }
                }
            }
            lastSingleRange_it++;
            singleRange_it++;
        }

        return doubleRangeBuffer;
    }


    /*
     * reads a list of tuples from single input measurement values and builds the main linearization from it
     */
    template<template<typename> class SequentialContainerType>
    auto init(SequentialContainerType<std::tuple<Ref,Sort,Goal...>> input){
        this->clear();
        if (input.empty())[[unlikely]] {
            return *this;
        }
        //first get min and max of a (size)
        const auto[min_a, max_a]=std::minmax_element(input.begin(), input.end(),
                                               [](auto a, auto b) -> bool {
                                                   return std::get<0>(a) < std::get<0>(b);
                                               });
        Ref min2_a = std::get<0>(*min_a);
        Ref max2_a = std::get<0>(*max_a);
        //get all sorted and average times for each length of the input
        std::vector<std::tuple<Ref,std::vector<std::tuple<Sort,Goal...>>>> singleRangeBuffer = singleSingleRangeInit(input, min2_a);
        //current structure of singleRangeBuffer: unordered_map<ref,vector<tuple<sort,goal,rest...>>>
        //speedup with ordered singleRangeBufferLookup vector<ref>
        //now we need to intermediate the single ranges to double ranges with sort_start_sort_end
        //for every count between start and end we will calculate the intermediate between start and endpoint and sort the order
        if (singleRangeBuffer.empty())[[unlikely]]{
            return *this;
        }
        if (singleRangeBuffer.size() > 1)[[likely]]{
            boost::unordered_map<tuple_hash_lib::tuple<Ref, Ref>,
                std::vector<std::tuple<Sort,Sort,Goal...>>
            > doubleRangeBuffer = calcSingleDoubleRangeBuffer(singleRangeBuffer);

            //now we have a double range, the last step will be to iterate single through all ranges and create a single range
            //for each step and sort over the average; first reset min and iterate until max

            //last order with tuple<goal, rest...>
            std::vector<std::tuple<Goal...>> last_order{};
            std::vector<std::tuple<Sort,Sort>> last_sort_val;
            //save the sub double ranges before pushing to this and continue from a partition of a sub range limiting it with i
            min2_a = std::get<0>(doubleRangeBuffer.begin()->first.var);
            //desired structure:
            //map<tuple<start_big_ref,end_big_ref>,tuple<vector<tuple<goal,rest...>>,map<tuple<start_sub_ref,end_sub_ref>,vector<tuple<start_sort,end_sort>>>>>

            //master range is set to this
            //sub ranges for the last master range
            boost::unordered_map<
                tuple_hash_lib::tuple<Ref,Ref>,
                std::vector<std::tuple<Sort,Sort>>
            > sub_ranges_map{};
            min2_a = std::get<0>(doubleRangeBuffer.begin()->first.var);//get first master start
            for (const auto &item1: doubleRangeBuffer) {
                auto tmp_sub_min = std::get<0>(item1.first.var);
                for (Ref i = std::get<0>(item1.first.var); i < std::get<1>(item1.first.var); ++i) {
                    //get the linear equivalent between start and end for all double ranges and sort to a set of single ranges
                    //start a range and end it if the order of vector<tuple<goal,rest...>> is changing
                    std::vector<std::tuple<Sort,Sort,Goal...>> singleOutput{};
                    for (const auto &item2: item1.second) {
                        auto start = static_cast<long double>(std::get<0>(item2));
                        auto start_old = start;
                        auto end = static_cast<long double>(std::get<1>(item2));
                        start += std::round((end - start) *
                                            (static_cast<long double>(i - std::get<0>(item1.first.var)) /
                                             static_cast<long double>(std::get<1>(item1.first.var) -
                                                                      std::get<0>(item1.first.var))));//result time
                        start_old += std::round((end - start) *
                                                (static_cast<long double>(tmp_sub_min - std::get<0>(item1.first.var)) /
                                                 static_cast<long double>(std::get<1>(item1.first.var) -
                                                                          std::get<0>(item1.first.var))));//result time
                        if (start < 0) {
                            start = 0;
                        }
                        singleOutput.push_back(
                                std::tuple_cat(std::make_tuple(static_cast<Sort>(start_old)),std::make_tuple(static_cast<Sort>(start)),filterTupElement<0>(filterTupElement<0>(item2))));
                    }
                    boost::sort::pdqsort(singleOutput.begin(), singleOutput.end(),
                                         [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });
                    decltype(last_order) new_order{};
                    decltype(last_sort_val) new_sort_val;
                    //split tuple<goal,rest...> from tuple<begin_sort,end_sort> but keep order
                    if (last_order.empty()) {
                        for (const auto &item2: singleOutput) {
                            last_order.push_back(filterTupElement<0>(filterTupElement<0>(item2)));
                            last_sort_val.push_back(std::tuple<Sort,Sort>{std::get<0>(item2),std::get<1>(item2)});
                        }
                    }
                    for (const auto &item2: singleOutput) {
                        new_order.push_back(filterTupElement<0>(filterTupElement<0>(item2)));
                        new_sort_val.push_back(std::tuple<Sort, Sort>{std::get<0>(item2),std::get<1>(item2)});
                    }
                    if (!boost::range::equal(last_order, new_order)) {//
                        //if the order is not equal, we need to end the range
                        //end the last range partition on i and start the next one on i, save the first result to this,
                        //push back the first subrange
                        sub_ranges_map.emplace(tuple_hash_lib::tuple<Ref, Ref>{tmp_sub_min, i - 1},last_sort_val);//TODO: check i-1
                        this->emplace(tuple_hash_lib::tuple<Ref, Ref>(min2_a, i - 1),std::make_tuple(last_order, sub_ranges_map));
                        sub_ranges_map.clear();
                        min2_a = i;
                        tmp_sub_min = min2_a;
                        last_order = new_order;
                    } else {
                        if (i == std::get<1>(item1.first.var)) {//last element write full range
                            sub_ranges_map.emplace(tuple_hash_lib::tuple<Ref, Ref>{tmp_sub_min, i},new_sort_val);
                        }
                    }
                    last_sort_val = new_sort_val;
                }
            }
            //if there is still stuff in the sub_ranges_map we need to add it to this
            if (!sub_ranges_map.empty()) {
                this->emplace(tuple_hash_lib::tuple<Ref, Ref>(min2_a, max2_a),std::make_tuple(last_order, sub_ranges_map));
            }
        }
        else[[unlikely]]{
            //save to this only the one sub range, take the already sorted information
            auto only_at = std::get<0>(*singleRangeBuffer.begin());
            std::vector<std::tuple<Goal...>> goal_sort{};
            std::vector<std::tuple<Sort,Sort>> sort_val;
            for (const auto &item2: std::get<1>(*singleRangeBuffer.begin())) {
                goal_sort.push_back(filterTupElement<0>(item2));
                sort_val.emplace_back(std::get<0>(item2),std::get<0>(item2));
            }
            boost::unordered_map<
                    tuple_hash_lib::tuple<Ref,Ref>,
                    std::vector<std::tuple<Sort,Sort>>
            > sub_ranges_map{};
            sub_ranges_map.emplace(
                    tuple_hash_lib::tuple<Ref,Ref>{only_at, only_at},
                    sort_val
            );
            this->emplace(tuple_hash_lib::tuple<Ref,Ref>(only_at, only_at),std::make_tuple(goal_sort, sub_ranges_map));
        }
        return *this;
    }

    /*
     * gets the minimum and maximum range saved to the valueRangesSingle
     */
    std::tuple<Ref, Ref> min_max() {
        Ref from = std::numeric_limits<Ref>::max(),to = std::numeric_limits<Ref>::min();
        for(const auto &item:*this){
            if(std::get<0>(item.first.var)<from){
                from = std::get<0>(item.first.var);
            }
            if(std::get<1>(item.first.var)>to){
                to = std::get<1>(item.first.var);
            }
        }
        return std::make_tuple(from,to);
    }

    /*
     * function that gets linear input and calculates all the exact points over the given reference
     */
    static std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>>
    single_dual_range_to_point_convert(
            std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>> input, Ref ref) {
        //average of Ref0 over linear function:
        auto start_x = static_cast<long double>(std::get<0>(std::get<0>(input).var));
        auto end_x = static_cast<long double>(std::get<1>(std::get<0>(input).var));

        if(ref < start_x)[[unlikely]]{ERROR << "The conversion could not be calculated, because the ref was smaller than the start_x of the range!" << std::endl;}
        if(ref > end_x)[[unlikely]]{ERROR << "The conversion could not be calculated, because the ref was greater than the end_x of the range!" << std::endl;}

        std::vector<std::tuple<Sort,Goal...>> output;

        for(const auto &i:std::get<1>(input)){
            //start_x + delta y / delta x
            auto start_y = static_cast<long double>(std::get<0>(i));
            auto end_y = static_cast<long double>(std::get<1>(i));
            start_y += ((end_y-start_y)/(end_x-start_x)) * (ref-start_x);
            output.push_back(std::tuple_cat(std::make_tuple(static_cast<Sort>(std::round(start_y))), filterTupElement<0>(
                    filterTupElement<0>(i))));
        }
        //sort
        boost::sort::pdqsort(output.begin(), output.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });
        return std::make_tuple(ref,output);
    }

    /*
     * all given sort ranges to point ranges at a specific spot
     */
    static std::vector<std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>>>
    dual_ranges_to_point_convert(
            std::vector<std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>> input,
            Ref ref) {
        std::vector<std::tuple<Ref,std::vector<std::tuple<Sort,Goal...>>>> output;
        for(const auto&item:input){
            auto tmp = single_dual_range_to_point_convert(item, ref);
            output.emplace_back(std::get<0>(tmp),std::get<1>(tmp));
        }
        return output;
    }

    /*
     * this function is taking two single ranges and fits the results to the correct goals into one double range
     */
    static std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    single_to_dual_ranges_merge(
            std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>> a,
            std::tuple<Ref, std::vector<std::tuple<Sort, Goal...>>> b) {
        if(not std::get<0>(a)==std::get<0>(b))[[unlikely]]{
            DEBUG << "While searching for the conjunction of two Ranges there was an error where both range values did not match up!";
        }
        std::get<1>(a)=singleSingleRangeDeduplicateGoals(std::get<1>(a));
        std::get<1>(b)=singleSingleRangeDeduplicateGoals(std::get<1>(b));
        boost::unordered_map<tuple_hash_lib::tuple<Goal...>,Sort> goal_to_sort_a,goal_to_sort_b,goal_to_sort_a_count,goal_to_sort_b_count;
        for(const auto item:std::get<1>(a)){
            auto key_a = tuple_hash_lib::tuple<Goal...>{filterTupElement<0>(item)};
            if(goal_to_sort_a.find(key_a)==goal_to_sort_a.end())[[likely]]{
                goal_to_sort_a.emplace(key_a,std::get<0>(item));
            }
            else[[unlikely]]{
                DEBUG << "single_to_dual_ranges_merge a key (a) of hardware was duplicate!";
                goal_to_sort_a.at(key_a)+=std::get<0>(item);
            }

        }
        for(const auto item:std::get<1>(b)){
            auto key_b = tuple_hash_lib::tuple<Goal...>{filterTupElement<0>(item)};
            if(goal_to_sort_b.find(key_b)==goal_to_sort_b.end())[[likely]]{
                goal_to_sort_b.emplace(key_b,std::get<0>(item));
            }
            else[[unlikely]]{
                DEBUG << "single_to_dual_ranges_merge a key (b) of hardware was duplicate!";
                goal_to_sort_b.at(key_b)+=std::get<0>(item);
            }
        }
        std::vector<std::tuple<Sort,Sort,Goal...>> output;
        for(const auto&item:goal_to_sort_a){
            output.push_back(std::tuple_cat(std::make_tuple(item.second),std::make_tuple(goal_to_sort_b.at(item.first)),item.first.var));
        }

        return std::make_tuple(tuple_hash_lib::tuple<Ref,Ref>{std::get<0>(a),std::get<0>(b)},output);
    }

    /*
     * gives conjunction of a range that is shared along input ranges
     */
    static std::vector<std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>>
    single_dual_ranges_to_min_conjunction_dual_range_convert(
            std::vector<std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>> input) {
        std::vector<std::tuple<tuple_hash_lib::tuple<Ref,Ref>,std::vector<std::tuple<Sort,Sort,Goal...>>>> output;
        if(input.empty())[[unlikely]]{
            return output;
        }
        //get smallest range that can be represented within all the input ranges
        Ref highest_min = std::get<0>(std::get<0>(std::max_element(input.begin(),input.end(),[](auto a, auto b) -> bool { return std::get<0>(std::get<0>(a).var) < std::get<0>(std::get<0>(b).var); })).var),
                lowest_max = std::get<1>(std::get<0>(std::min_element(input.begin(),input.end(),[](auto a, auto b) -> bool { return std::get<1>(std::get<0>(a).var) < std::get<1>(std::get<0>(b).var); })).var);

        std::vector<std::tuple<Ref,std::vector<std::tuple<Sort,Goal...>>>> fromSingleRange,// = single_dual_range_to_point_convert(input,highest_min);
        toSingleRange;// = single_dual_range_to_point_convert(input,lowest_max);
        for(const auto&item:input){
            fromSingleRange.push_back(single_dual_range_to_point_convert(item, highest_min));
            toSingleRange.push_back(single_dual_range_to_point_convert(item, lowest_max));
        }

        auto from_begin=fromSingleRange.begin();
        auto from_end=fromSingleRange.end();
        auto to_begin=toSingleRange.begin();
        auto to_end=toSingleRange.end();

        while(from_begin!=from_end and to_begin!=to_end){
            output.push_back(single_to_dual_ranges_merge(*from_begin,*to_begin));
            from_begin++;
            to_begin++;
        }
        return output;
    }

    /*
     * averages two linear double ranges into one single double range
     */
    static std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    single_dual_ranges_to_average_dual_range(
            std::vector<std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>> input) {
        std::vector<std::tuple<tuple_hash_lib::tuple<Ref,Ref>,std::vector<std::tuple<Sort,Sort,Goal...>>>>
                conjunction = single_dual_ranges_to_min_conjunction_dual_range_convert(input);
        boost::unordered_map<tuple_hash_lib::tuple<Goal...>,std::tuple<long double,long double>> avg_to_goal_sort;//tuple<Sort,Sort>
        tuple_hash_lib::tuple<Ref,Ref> test_range;
        bool test_range_set = false;

        for(const auto&item:conjunction){
            if(not test_range_set)[[unlikely]]{
                test_range_set = true;
                test_range = std::get<0>(item);
            }
            if(not std::get<0>(item)==test_range)[[unlikely]]{
                DEBUG << "The test_range in single_dual_ranges_to_average_dual_range shifted unexpectedly to ("+
                         std::to_string(std::get<0>(std::get<0>(item).var))+","+std::to_string(std::get<1>(std::get<0>(item).var))+
                         ") instead of ("+std::to_string(std::get<0>(test_range.var))+","+std::to_string(std::get<1>(test_range.var))+").";
            }
            for(const auto&item2:std::get<1>(item)){
                auto key = filterTupElement<0>(filterTupElement<0>(item2));
                if(avg_to_goal_sort.find(key)==avg_to_goal_sort.end())[[unlikely]]{
                    avg_to_goal_sort.emplace(key,std::tuple<long double,long double>{static_cast<long double>(std::get<0>(item2)),static_cast<long double>(std::get<0>(item2))});
                }
                else[[likely]]{
                    std::get<0>(avg_to_goal_sort.at(key))+=static_cast<long double>(std::get<0>(item2));
                    std::get<1>(avg_to_goal_sort.at(key))+=static_cast<long double>(std::get<1>(item2));
                }
            }
        }
        std::vector<std::tuple<Sort,Sort,Goal...>> output;
        for(const auto&pair:avg_to_goal_sort){
            output.push_back(std::tuple_cat(std::make_tuple(static_cast<Sort>(std::get<0>(pair.second)/input.size())),
                                            std::make_tuple(static_cast<Sort>(std::get<1>(pair.second)/input.size())),
                                            pair.first.var));
        }
        return std::make_tuple(test_range,output);
    }

    /*
     * gets sub-range on exact reference
     * function to get information, outputs <Ref1_begin,Ref1_end> --> <Sort_begin, Sort_end, Goal, ...>
     */
    std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    single_get_range_at(Ref ref) {
        if(this->empty())[[unlikely]]{ERROR << "The valueRangesSingle class is empty and cannot return the range of Ref0: " << std::to_string(ref) << std::endl;}
        auto min_max_tup = min_max();
        Ref min = std::get<0>(min_max_tup);
        Ref max = std::get<1>(min_max_tup);
        if (ref < min) {
            return single_get_range_at(min);
        }
        if (max < ref) {
            return single_get_range_at(max);
        }

        std::vector<std::tuple<Sort,Sort,Goal...>> merge_vec{};

        for (const auto &item: *this) {
            if (std::get<0>(item.first.var) <= ref and ref <= std::get<1>(item.first.var))[[unlikely]]{
                for (const auto &item2: std::get<1>(item.second)) {
                    //item2 is first and second of boost::unordered_map<tuple_hash_lib::tuple<rangeType<benchTuple,Ref0>,rangeType<benchTuple,Ref0>>,std::vector<std::tuple<rangeType<benchTuple,Sort>,rangeType<benchTuple,Sort>>>>
                    if (std::get<0>(item2.first.var) <= ref and ref <= std::get<1>(item2.first.var)) {
                        //create a single range from the double range, structure:
                        //vector<tuple<sort, goal, rest...>>
                        //get average in getOptimal for individual information within ltb --> merge sorted <Goal, ...> list with Sort linear function
                        auto it_goal = std::get<0>(item.second).begin();
                        auto it_goal_end = std::get<0>(item.second).end();
                        auto it_sort_range = item2.second.begin();
                        auto it_sort_range_end = item2.second.end();
                        while (it_goal != it_goal_end and it_sort_range != it_sort_range_end) {
                            //merge
                            merge_vec.push_back(std::tuple_cat(*it_sort_range,*it_goal));
                            it_goal++;
                            it_sort_range++;
                        }
                        return std::make_tuple(item2.first, merge_vec);
                    }
                }
            }
        }
        ERROR << "The Reference \""+std::to_string(ref)+"\" was not found";
        return std::make_tuple(tuple_hash_lib::tuple<Ref,Ref>{(Ref) 0, (Ref) 0}, merge_vec);
    }

    /*
     * manually read one value ordered
     */
    std::tuple<Ref,std::vector<std::tuple<Sort,Goal...>>> operator[](Ref ref){
        return single_dual_range_to_point_convert(single_get_range_at(ref));
    }

    /*
     * this average over all just references to the smallest and the largest entry of the measurement set
     * perfect for quick estimate over all size measurement average over min-max master range
     */
    std::tuple<tuple_hash_lib::tuple<Ref, Ref>, std::vector<std::tuple<Sort, Sort, Goal...>>>
    single_get_info_average_min_max_all() {
        if(this->empty())[[unlikely]]{
            ERROR << "The valueRangesSingle class is empty and cannot return the average over the entire range!" << std::endl;
        }
        auto min_max_tup = min_max();
        Ref min = std::get<0>(min_max_tup);
        Ref max = std::get<1>(min_max_tup);
        auto min_info = single_get_range_at(min);
        auto max_info = single_get_range_at(max);
        auto min_info_it = std::get<1>(min_info).begin();
        auto min_info_it_end = std::get<1>(min_info).end();
        auto max_info_it = std::get<1>(max_info).begin();
        auto max_info_it_end = std::get<1>(max_info).end();

        boost::unordered_map<tuple_hash_lib::tuple<Goal...>,std::tuple<long double, long double>//std::tuple<boost::mp11::mp_at_c<benchTuple, Sort>, boost::mp11::mp_at_c<benchTuple, Sort>>
        > average_info_map;
        //calculate average ranges to map
        while(min_info_it!=min_info_it_end and max_info_it!=max_info_it_end){
            //merge min_info and max_info to generate a min and max average for each <Goal, Rest...>, return linear list
            //get the min of min_info Ref0 and the max of max_info Ref0 and average both linear Sort support points
            auto min_key = filterTupElement<0>(filterTupElement<0>(*min_info_it));
            if (average_info_map.find(min_key) == average_info_map.end())[[likely]]{
                average_info_map.emplace(min_key, std::make_tuple(
                        static_cast<long double>(std::get<0>(*min_info_it))/2,
                        static_cast<long double>(std::get<1>(*min_info_it))/2
                ));
            }
            else[[unlikely]]{
                std::get<0>(average_info_map.at(min_key))+=static_cast<long double>(std::get<0>(*min_info_it))/2;
                std::get<1>(average_info_map.at(min_key))+=static_cast<long double>(std::get<1>(*min_info_it))/2;
            }

            auto max_key = pop_tuple_front(pop_tuple_front(*max_info_it));
            if (average_info_map.find(max_key) == average_info_map.end())[[unlikely]]{
                average_info_map.emplace(max_key, std::make_tuple(
                        static_cast<long double>(std::get<0>(*max_info_it))/2,
                        static_cast<long double>(std::get<1>(*max_info_it))/2
                ));
            }
            else[[likely]]{
                std::get<0>(average_info_map.at(max_key))+=static_cast<long double>(std::get<0>(*max_info_it))/2;
                std::get<1>(average_info_map.at(max_key))+=static_cast<long double>(std::get<1>(*max_info_it))/2;
            }

            min_info_it++;
            max_info_it++;
        }
        //put averages together into a single list and sort
        std::vector<std::tuple<Sort,Sort,Goal...>> average_info;

        for (const auto &i:average_info_map) {
            auto start_val = static_cast<Sort>(std::round(std::get<0>(i.second)));
            auto end_val = static_cast<Sort>(std::round(std::get<1>(i.second)));
            average_info.push_back(std::tuple_cat(std::tie(start_val),std::tie(end_val),i.first.var));
        }

        boost::sort::pdqsort(average_info.begin(), average_info.end(),
                             [](auto a, auto b) -> bool { return std::get<0>(a) < std::get<0>(b); });

        return std::make_tuple(tuple_hash_lib::tuple<Ref,Ref>{std::get<0>(std::get<0>(min_info).var), std::get<1>(std::get<0>(max_info).var)}, average_info);
    }
};


#endif //ULTIHASH_VALUERANGE_H
