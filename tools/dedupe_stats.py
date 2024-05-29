# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import matplotlib.pyplot as plt
from tabulate import tabulate
from collections import Counter, defaultdict
import sys

pointers_count = Counter()
offsets_count = Counter()
prefixes_count = Counter()
prefixes_str_count = Counter()
frag_size_count = Counter()
pointers_sizes = defaultdict(int)
age_dedupe_count = Counter()
updated_age_dedupe_count = Counter()

fragment_ages = {}
fragment_updated_ages = {}
fragment_dedupe_count = Counter()

age = 0
final_set_size = 0
final_dedupe_count = 0
final_non_dedupe_count = 0
final_total_size = 0
final_effective_size = 0

def record_dedupe (size, prefix, pointer, offset):
    global age

    int_size = int(size)
    int_pointer = int (pointer)
    int_offset = int (offset)
    int_prefix = int.from_bytes(bytes(prefix[0:4], 'utf-8'), byteorder=sys.byteorder)
    prefix = ''.join(c for c in prefix if c.isprintable())
    
    pointers_count[int_pointer] += 1
    pointers_sizes[int_pointer] += int_size
    offsets_count[int_offset] += 1
    prefixes_count[int_prefix] += 1
    prefixes_str_count[prefix] += 1
    frag_size_count[int_size] += 1
    fragment_dedupe_count[int_pointer] += 1

    frag_age = fragment_ages[int_pointer]
    age_dedupe_count[age - frag_age] += 1
    
    frag_updated_age = fragment_updated_ages[int_pointer]
    updated_age_dedupe_count[age - frag_updated_age] += 1
    fragment_updated_ages[int_pointer] = age
    
    age += 1
    
def record_add_to_set (pointer, size):
    global age
    int_pointer = int (pointer)
    fragment_dedupe_count[int_pointer] += 1
    fragment_ages[int_pointer] = age
    fragment_updated_ages[int_pointer] = age
    age += 1
    
def record_stat (set_size, dedupe_count, non_dedupe_count, effective_size, total_size):
    global final_set_size, final_dedupe_count, final_non_dedupe_count, final_total_size, final_effective_size
    final_set_size = max(int(set_size), final_set_size)
    final_dedupe_count += int(dedupe_count)
    final_non_dedupe_count += int(non_dedupe_count)
    final_effective_size += int(effective_size)
    final_total_size += int(total_size)

def load_data (filename):
    file = open(filename, 'r')
    
    line = file.readline()
    while line:
        data = line.split(" ")
        if (data[0] == "dedupe"):
            record_dedupe (data[1], data[2], data[3], data[4])
        elif (data[0] == "stat"):
            record_stat (data[1], data[2], data[3], data[4], data[5])
        elif (data[0] == "non-dedupe"):
            record_add_to_set (data[1], data[2])
    
        line = file.readline()

def draw_plots (log_scale):
    fig, axs = plt.subplots(4, 2, figsize=(20, 30))
    
    axs[0,0].stem(x11, y11, label="dedupe size")
    axs[0,0].set_title ("[1,1] Deduplication Count by Pointer")
    axs[0,0].set_ylabel('Count')
    axs[0,0].set_xlabel('Pointer')
    
    axs[0,1].stem(x12, y12, label="dedupe size")
    axs[0,1].set_title ("[1,2] Deduplication Size by Pointer")
    axs[0,1].set_ylabel('Size')
    axs[0,1].set_xlabel('Pointer')
    
    axs[1,0].stem(x21, y21, label="dedupe count")
    axs[1,0].set_title ("[2,1] Deduplication Count by Data Prefix")
    axs[1,0].set_ylabel('Count')
    axs[1,0].set_xlabel('Prefix')
    
    axs[1,1].stem(x22, y22, label="dedupe count")
    axs[1,1].set_title ("[2,2] Deduplication Count by Fragment Size")
    axs[1,1].set_ylabel('Count')
    axs[1,1].set_xlabel('Fragment Size')
    
    axs[2,0].stem(y31, label="dedupe count")
    axs[2,0].set_title ("[3,1] Deduplication Count of All Fragments")
    axs[2,0].set_ylabel('Count')
    axs[2,0].set_xlabel('X')
    
    axs[2,1].stem(x32, y32, label="dedupe count")
    axs[2,1].set_title ("[3,2] Deduplication Count by Offset in Data")
    axs[2,1].set_ylabel('Count')
    axs[2,1].set_xlabel('Offset in data')
    
    axs[3,0].stem(x41, y41, label="dedupe count")
    axs[3,0].set_title ("[4,1] Deduplication Count for Different Fragment Ages")
    axs[3,0].set_ylabel('Count')
    axs[3,0].set_xlabel('`Fragment Age')
    
    axs[3,1].stem(x42, y42, label="dedupe count")
    axs[3,1].set_title ("[4,2] Deduplication Count for Different Fragment Updated Ages")
    axs[3,1].set_ylabel('Count')
    axs[3,1].set_xlabel('`Fragment Updated Age')
    
    for i in range(0, 4):
        for j in range(0, 2):
            axs[i,j].grid(True)
            if log_scale:
                axs[i,j].set_yscale('log')
    
    plt.show()

def print_stats ():
    print ("Total size (MB): " + str(final_total_size/(1024*1024)))
    print ("Total effective size (MB): " + str(final_effective_size/(1024*1024)))
    print ("Deduplication ratio: " + str(1 - final_effective_size / final_total_size))
    print ("")
    print ("Set size after the upload (total number of fragments): " + str(final_non_dedupe_count))
    print ("Total number of fragments: " + str(fragment_dedupe_count.total()))
    print ("Total number of deduplicated fragments: " + str(final_dedupe_count))
    print ("Counts of fragments with lowest deduplication: ")

    count = 0
    total = 0
    count_dedupe = Counter()
    for c in fragment_dedupe_count.values():
        count_dedupe[c] += 1
        
    for k,c in sorted(count_dedupe.items()):
        print ("\t" + str (c) + " number of fragment(s) are deduplicated " + str(k - 1) + " number of time(s)") 
        count += 1
        total += c
        if (count == 5):
            break
    print("\tRemaining number of fragments: " + str(count_dedupe.total() - total))
    print ("")
    print ("Top 20 most often deduplicated fragment prefixes: ")
    print ("")
    print (tabulate(prefixes_str_count.most_common(20), headers=['Prefix', 'Count'], tablefmt='orgtbl'))

filename = "/var/lib/uh/deduplicator/dedupe_log"    
load_data(filename)

x11, y11 = zip(*sorted (pointers_count.items()))
x12, y12 = zip(*sorted (pointers_sizes.items()))
x21, y21 = zip(*sorted (prefixes_count.items()))
x22, y22 = zip(*sorted (frag_size_count.items()))
y31 = sorted([c for k, c in fragment_dedupe_count.items() if c > 1])
x32, y32 = zip(*sorted(offsets_count.items()))
x41, y41 = zip(*sorted (age_dedupe_count.items()))
x42, y42 = zip(*sorted (updated_age_dedupe_count.items()))

print_stats()

draw_plots(False)
draw_plots(True)
