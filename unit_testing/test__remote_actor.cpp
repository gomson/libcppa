#include <string>
#include <boost/thread.hpp>

#include "test.hpp"
#include "ping_pong.hpp"
#include "cppa/cppa.hpp"
#include "cppa/exception.hpp"

using namespace cppa;

namespace {

void client_part(const std::vector<std::string>& argv)
{
    if (argv.size() != 2) throw std::logic_error("argv.size() != 2");
    std::istringstream iss(argv[1]);
    std::uint16_t port;
    iss >> port;
    auto ping_actor = remote_actor("localhost", port);
    try
    {
        pong(ping_actor);
    }
    catch (...)
    {
    }
}

} // namespace <anonymous>

size_t test__remote_actor(const char* app_path, bool is_client,
                          const std::vector<std::string>& argv)
{
    CPPA_TEST(test__remote_actor);
    if (is_client)
    {
        client_part(argv);
        return 0;
    }
    auto ping_actor = spawn(ping);
    std::uint16_t port = 4242;
    bool success = false;
    while (!success)
    {
        try
        {
            publish(ping_actor, port);
            success = true;
        }
        catch (bind_failure&)
        {
            // try next port
            ++port;
        }
    }
    std::string cmd;
    {
        std::ostringstream oss;
        oss << app_path << " test__remote_actor " << port << " &>/dev/null";
        cmd = oss.str();
    }
    // execute client_part() in a separate process,
    // connected via localhost socket
    boost::thread child([&cmd] () { system(cmd.c_str()); });
    await_all_others_done();
    CPPA_CHECK_EQUAL(pongs(), 5);
    // wait until separate process (in sep. thread) finished execution
    child.join();
    return CPPA_TEST_RESULT;
}