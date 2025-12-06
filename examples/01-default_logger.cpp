#include <tmb/tmb.hpp>

int main(void) {
    tmb::info("aa {}", 3);

    auto lgr = tmb::Logger("eeee");
    lgr.info("woho {} {}", 3); // [format error]s

    lgr.set_default_format("{$BLUE:$}\n");
    lgr.error("eh");
}
