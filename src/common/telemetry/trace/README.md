# trace

## TODOs

- [x] Call `start_trace` on root coroutine's awaitable. Remove special constructor for context as a first argument.
- Broken coroutines chain 
    - [ ] In `catch_frag` function, trasfering context to `read_fragment`(`src/common/global_data/default_global_data_view.cpp|51`) using `CURRENT_CONTEXT`: 
- [x] Interfaces
    - [x] A single remote deduplicator is spawned. Why? in `src/entrypoint/commands/s3/put_object.cpp|29` col 15| upload function
    - [x] `src/common/utils/address_utils.cpp|55` col 22| `perform_for_address`
    - [x] `run_for_all` uses `co_spawn`.

- [x] Find `set_attribute` and `sub_context`. Change those using `trace_span`.
- [x] Propagate parent context

- [ ] Turn off trace and see how it works
- [ ] Create `recv_header` and `recv_header_with_context`
- [ ] Remove previous context propagation from the codes
- [ ] Tweak `co_spawn`
- [ ] Add logs into spans
- [ ] Create `m_span` on promise's creation, not on initial suspend.
- [ ] Tweak `async_result` macro to start and end spans for async operations according to the design step 7.

