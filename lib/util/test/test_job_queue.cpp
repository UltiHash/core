#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil JobQueue Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <util/job_queue.h>
#include <stdexcept>


using namespace uh;
using namespace uh::util;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(wait_empty)
{
    job_queue<void, int> queue(2, [](int x) -> void {});

    queue.wait();

    BOOST_CHECK(true);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(nowait_workers)
{
    std::atomic<unsigned> accumulator = 0;

    {
        job_queue<void, int> queue(2, [&accumulator](int x) -> void { accumulator += x; });

        queue.push(6);
        queue.push(5);
        queue.push(4);
        queue.push(3);
        queue.push(2);
        queue.push(1);
    }

    BOOST_CHECK(true);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(void_workers)
{
    std::atomic<uint64_t> accumulator = 0;

    {
        job_queue<void, int> queue(50, [&accumulator](int x) -> void { accumulator += x; });

        for (unsigned i = 1; i <= 1000000; ++i)
        {
            queue.push(i);
        }

        queue.wait();
    }

    BOOST_CHECK_EQUAL(accumulator, 500000500000);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(container)
{
    job_queue<int, int> queue(2, [](int x) -> int { return x * 2; });

    std::list<int> inputs{ 1, 2, 3, 5, 8, 2222 };
    std::list<std::future<int>> futures;

    for (auto& i : inputs)
    {
        futures.push_back(queue.push(std::move(i)));
    }

    queue.wait();

    std::list<int> results;
    for (auto& f : futures)
    {
        results.push_back(f.get());
    }

    std::list<int> expected{ 2, 4, 6, 10, 16, 4444};
    results.sort();

    BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                  results.begin(), results.end());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(exceptions)
{
    job_queue<int, int> queue(2, [](int x) -> int { throw std::runtime_error(""); });

    std::future<int> result = queue.push(1);
    queue.wait();

    BOOST_CHECK_THROW(result.get(), std::exception);
}

// ---------------------------------------------------------------------

} // namespace
