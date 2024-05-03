#include <cstdint>
#include <cstring>
#include <string>
#include "io.hpp"

struct PrintOrder {
    uint32_t order_id;
    intmax_t timestamp;

    // Lets set 0 to be cancelled, 1 to be matched and 2 to be added
    uint32_t state;

    bool is_cancelled;

    int deducted;
    int execution_id;
    uint32_t resting_id;
    uint32_t price;
    std::string instrument;
    uint32_t count;
    bool is_sell;
};
