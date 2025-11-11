#include <tmb/tmb.hpp>

int main(void) {
    tmb::info("aa {}", 3);

    auto lgr = tmb::Logger("eeee");
    lgr.info("woho {} {}", 3);
}
