#include <list>
#include <mutex>
#include <string>
#include "print.hpp"

class Order {
    private:
        std::mutex mut;
	public:
        uint32_t order_id;
	    uint32_t price;
	    uint32_t count;
	    intmax_t timestamp;
        std::string instrument;
	    int execution_id = 0;
	    bool is_cancelled;
        bool is_sell;

    Order(ClientCommand input, int64_t timestamp) {
        order_id = input.order_id;
        price = input.price;
        count = input.count;
        this->timestamp = timestamp;
        this->is_cancelled = false;
        is_sell = input.type == input_sell;
        instrument = std::string(input.instrument);
    }

    std::optional<PrintOrder> match_resting_buy(intmax_t timestamp, ClientCommand* input) {
        std::lock_guard<std::mutex> lock{mut};
        if (
            count > 0 and 
            input->count > 0 and 
            this->timestamp <= timestamp and
            input->price <= price and 
            not is_cancelled and
            instrument == std::string(input->instrument)
        ) {
            int deducted = 0;
            execution_id++;
            if (count >= input->count) {
                deducted = input->count;
                count -= input->count;
                input->count = 0;
            } else {
                deducted = count;
                input->count -= count;
                count = 0;
            }

            PrintOrder order = PrintOrder{};
            order.price = price;
            order.deducted = deducted;
            order.execution_id = execution_id;
            order.resting_id = order_id;
            order.order_id = input->order_id;
            order.timestamp = timestamp;
            order.state = 1;

            return order;
        }
        return std::nullopt;
    }

    std::optional<PrintOrder> match_resting_sell(intmax_t timestamp, ClientCommand* input) {
        std::lock_guard<std::mutex> lock{mut};
        if (
            count > 0 and 
            input->count > 0 and 
            this->timestamp <= timestamp and
            input->price >= price and 
            not is_cancelled and 
            instrument == std::string(input->instrument)
        ) {
            int deducted = 0;
            execution_id++;
            if (count >= input->count) {
                deducted = input->count;
                count -= input->count;
                input->count = 0;
            } else {
                deducted = count;
                input->count -= count;
                count = 0;
            }

            PrintOrder order = PrintOrder{};
            order.state = 1;
            order.price = price;
            order.deducted = deducted;
            order.execution_id = execution_id;
            order.resting_id = order_id;
            order.order_id = input->order_id;
            order.timestamp = timestamp;
            return order;
        }
        return std::nullopt;
    }

    std::optional<PrintOrder> cancel(
        ClientCommand input, int64_t timestamp) {
        std::lock_guard<std::mutex> lock{mut};
        if (input.order_id == order_id and count > 0) {
            is_cancelled = true;
            PrintOrder order = PrintOrder{};
            order.state = 0;
            order.order_id = order_id;
            order.timestamp = timestamp;
            order.is_cancelled = true;
            return order;
        }
        return std::nullopt;
    }
};
