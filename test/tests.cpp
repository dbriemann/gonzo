#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <fstream>
#include <libgonzo/error.hpp>
#include <libgonzo/process.hpp>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>

using namespace gonzo;

namespace {
bool process_exists(pid_t pid) {
	auto ret = kill(pid, 0);
	return ret != -1 and errno != ESRCH;
}

char get_process_status(pid_t pid) {
	std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
	std::string   data;
	std::getline(stat, data);
	auto index_of_last_parenthesis = data.rfind(')');
	auto index_of_status_indicator = index_of_last_parenthesis + 2;
	return data[index_of_status_indicator];
}
} // namespace

TEST_CASE("process::launch success", "[process]") {
	auto proc = process::launch("yes");
	REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]") {
	REQUIRE_THROWS_AS(process::launch("process_which_not_exists"), error);
}

TEST_CASE("process::attach success", "[process]") {
	auto target = process::launch("targets/run_endlessly", false);
	auto proc   = process::attach(target->pid());
	REQUIRE(get_process_status(target->pid()) == 't');
}

TEST_CASE("process::attach invlid PID", "[process]") {
	REQUIRE_THROWS_AS(process::attach(0), error);
}

TEST_CASE("process::resume success", "[process]") {
	{
		auto proc = process::launch("targets/run_endlessly");
		proc->resume();
		auto status  = get_process_status(proc->pid());
		auto success = status == 'R' or status == 'S';
		REQUIRE(success);
	}
	{
		auto target = process::launch("targets/run_endlessly", false);
		auto proc   = process::attach(target->pid());
		proc->resume();
		auto status  = get_process_status(proc->pid());
		auto success = status == 'R' or status == 'S';
		REQUIRE(success);
	}
}

TEST_CASE("process::resume already terminated", "[process]") {
	auto proc = process::launch("targets/end_immediately");
	proc->resume();
	proc->wait_on_signal();
	REQUIRE_THROWS_AS(proc->resume(), error);
}
