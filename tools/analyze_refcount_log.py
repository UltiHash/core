#!/usr/bin/python3
import re
from collections import defaultdict

# Regular expression to parse log entries
log_pattern = re.compile(r"(incrementing|decrementing) refcount at offset=(\d+), size=(\d+)")

def analyze_log_file(log_file_path):
    # Dictionaries to store counts of increments and decrements for each (X, Y, Z) tuple
    increments = defaultdict(int)
    decrements = defaultdict(int)

    # Read the log file and parse each line
    with open(log_file_path, 'r') as file:
        for line in file:
            match = log_pattern.search(line)
            if match:
                action, offset, size = match.groups()
                key = (offset, size)  # Unique identifier for the refcount

                if action == "incrementing":
                    increments[key] += 1
                elif action == "decrementing":
                    decrements[key] += 1

    # Check for mismatches
    unmatched_increments = []
    unmatched_decrements = []

    # Find increment entries with no corresponding decrement
    for key, count in increments.items():
        if decrements[key] < count:
            unmatched_increments.append((key, count, decrements[key]))

    # Find decrement entries with no corresponding increment
    for key, count in decrements.items():
        if increments[key] < count:
            unmatched_decrements.append((key, increments[key], count))

    # Report results
    if unmatched_increments or unmatched_decrements:
        print("Unmatched increments:")
        for key, inc_count, dec_count in unmatched_increments:
            print(f"Offset {key[0]}, Size {key[1]} - Incremented {inc_count} times, Decremented {dec_count} times")

        print("\nUnmatched decrements:")
        for key, inc_count, dec_count in unmatched_decrements:
            print(f"Offset {key[0]}, Size {key[1]} - Incremented {inc_count} times, Decremented {dec_count} times")
    else:
        print("All increment and decrement entries match.")

# Example usage
analyze_log_file("../cmake-build-debug/logs/storage-0.log")

