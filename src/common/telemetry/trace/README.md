# Trace Asio

This library extends Boost Asio to support OpenTelemetry tracing.

You can initiate a trace for a coroutine call chain by calling `start_trace()` on the return type of the root coroutine, `traced_awaitable`.

```cpp
resp = co_await handle_request(s, *req, id).start_trace();
```

To utilize this method, you must return `boost::asio::traced_awaitable` instead of `boost::asio_awaitable`. All coroutines that return `traced_awaitable` will inherit the context of their parent span in the background and indicate their running time with start/end spans.

To propagate the context, you must retrieve the current context by awaiting `boost::asio::this_coro::context`.

```cpp
auto context = co_await boost::asio::this_coro::context;
```

You can encode the context using the `encode_context` function, send it through a communication channel, and decode it with `decode_context` provided by `trace.h`.

To continue creating a span on a different process, call `continue_trace(context)` on the return type of the root coroutine, `traced_awaitable`.

```cpp
co_await handle_dedupe(hdr, m).continue_trace(std::move(context));
```

You can debug trace facilities with `iterate_call_stack`.

```cpp
auto span = co_await boost::asio::this_coro::span;
span->iterate_call_stack(
    [](boost::source_location loc) { LOG_INFO() << loc; });
```

You can also verify context with `check_context`.

## TODOs

- [x] Create `recv_header_with_context` and use it to retrieve context only
- [x] Remove previous context propagation from the code
- [ ] Instantiate `co_spawn` and `async_result` for `traced_awaitable`
- [ ] Include logs in spans
- [ ] Utilize references to retrieve context
