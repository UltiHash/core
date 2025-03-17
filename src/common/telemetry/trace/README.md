# Trace Asio

This extends Boost Asio to support OpenTelemetry trace.

You can start a trace for a coroutine call chain by calling `start_trace()` on the root coroutine's return type, `traced_awaitable`.

```cpp
resp = co_await handle_request(s, *req, id).start_trace();
```

To use this method, you need to return `boost::asio::traced_awaitable` instead of `boost::asio_awaitable`.
All coroutines that return `traced_awaitable` will inherit the context of their parent span in the background and indicate their running time with start/end spans.

To propagate the context, you need to retrieve the current context by awaiting `boost::asio::this_coro::context`.

```cpp
auto context = co_await boost::asio::this_coro::context;
```

You can encode the context using the `encode_context` function, send it through a communication channel, and decode it with `decode_context` provided by `trace.h`.

To continue creating a span on a different process, call `continue_trace(context)` on the root coroutine's return type, `traced_awaitable`.

```cpp
co_await handle_dedupe(hdr, m).continue_trace(std::move(context));
```

## TODOs

- [ ] Create `recv_header_with_context` and use it to get context from it only
- [ ] Remove previous context propagation from the code
- [ ] Tweak `async_result` macro to start and end spans for async operations
- [ ] Tweak `co_spawn` to spawn coroutines that return `traced_awaitable`
- [ ] Add logs to spans
- [ ] Use reference to get context
