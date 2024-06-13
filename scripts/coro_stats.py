#!/usr/bin/env python3

import datetime
import fileinput

contexts = dict()

def collect_context_log(date, msg):
    action = 'enter' if msg.startswith('ctx enter') else 'leave'

    colon = msg.find(':')
    msg = msg[colon + 1:]

    [ctx, loc] = msg.split(', loc:')

    loc = loc.strip()

    if not loc in contexts:
        contexts[loc] = []

    contexts[loc].append((date, action, ctx))

def refine(calls):
    stack = dict()
    rv = []

    for (date, action, ctx) in calls:
        if ctx in stack:
            assert action == 'leave'
            period = date - stack[ctx]
            rv.append((ctx, period, stack[ctx]))
            stack.pop(ctx, None)
        else:
            assert action == 'enter'
            stack[ctx] = date

    if not len(stack) == 0:
        print("the following contexts are still active:")
        for key in stack:
            print(f"+ {key}")

    return rv

for line in fileinput.input():
    line = line.rstrip()
    fields = line.split('\t')

    try:
        date = datetime.datetime.fromisoformat(fields[0])
        level = fields[1]
        msg = fields[2]

        if msg.startswith(('ctx enter', 'ctx leave')):
            collect_context_log(date, msg)
    except:
        print("error parsing line: " + line)


contexts_refined = dict()
for ctx in contexts:
    contexts_refined[ctx] = refine(contexts[ctx])

for ctx in contexts_refined:
    print(f"run times for context {ctx}")
    min_period = datetime.timedelta.max
    max_period = datetime.timedelta.min
    max_ctx = None
    max_start = None
    sum = datetime.timedelta()
    for call in contexts_refined[ctx]:
        if max_period < call[1]:
            max_period = call[1]
            max_ctx = call[0]
            max_start = call[2]
        if min_period > call[1]:
            min_period = call[1]
        sum += call[1]

    count = len(contexts_refined[ctx])
    print(f"  max: {max_period}, min: {min_period}, avg: {sum/count}, calls: {count}")
    print(f"  max runtime in coroutine {max_ctx}, start: {max_start}")
