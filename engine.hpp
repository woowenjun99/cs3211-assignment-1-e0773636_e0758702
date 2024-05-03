#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <chrono>
#include <list>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <queue>
#include <condition_variable>
#include "io.hpp"
#include "order.hpp"

struct Engine	{
public:
	void accept(ClientConnection conn);

private:
	void connection_thread(ClientConnection conn);
};

inline std::chrono::microseconds::rep getCurrentTimestamp() noexcept	{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
};

class Exchange {
	private:
		struct ReturnData {
			Order* new_order;
			std::queue<PrintOrder> q;
			int64_t timestamp;
		};

		std::list<Order*> resting_buys;
		std::list<Order*> resting_sells;
		std::list<ReturnData> returned_datas;

		std::mutex mut;
		std::condition_variable condition;
		CommandType current_direction = input_buy;
		int threads_processing = 0;
		std::queue<Order*> to_add;

		void enter_bridge(ClientCommand original) {
			std::unique_lock<std::mutex> lock{mut};
			while (threads_processing and current_direction != original.type) condition.wait(lock);
			threads_processing++;
			current_direction = original.type;
			lock.unlock();
			condition.notify_all();
		}

		void leave_bridge(ReturnData result) {
			std::unique_lock<std::mutex> lock{mut};
			threads_processing--;

			// Add in a new order if there is
			if (result.new_order) to_add.push(result.new_order);

			// Add in the returned data to the list
			returned_datas.push_back(result);

			if (not threads_processing) {
				// Sort the returned data by timestamp and print out the values.
				returned_datas.sort([] (const ReturnData &a, const ReturnData &b) {
					return a.timestamp < b.timestamp;
				});

				for (auto it: returned_datas) {
					std::queue<PrintOrder> q = it.q;
					while (not q.empty()) {
						PrintOrder front = q.front();
						if (front.state == 2) {
							Output::OrderAdded(
								front.order_id,
								front.instrument.c_str(),
								front.price,
								front.count,
								front.is_sell,
								front.timestamp
							);
						} else if (front.state == 0) {
							Output::OrderDeleted(
								front.order_id, 
								front.is_cancelled, 
								front.timestamp
							);
						} else {
							Output::OrderExecuted(
								front.resting_id,
								front.order_id,
								front.execution_id,
								front.price,
								front.deducted,
								front.timestamp
							);
						}
						q.pop();
					}
				}

				returned_datas.clear();

				// Delete all the invalid data
				std::vector<std::list<Order*>::iterator> v;
				for (auto it = resting_buys.begin(); it != resting_buys.end(); ++it) {
					if ((*it)->is_cancelled or not (*it)->count) v.push_back(it);
				}
				for (auto it = resting_sells.begin(); it != resting_sells.end(); ++it) {
					if ((*it)->is_cancelled or not (*it)->count) v.push_back(it);
				}
				for (auto it: v) {
					Order* order = *it;
					if ((*it)->is_sell) resting_sells.erase(it);
					else resting_buys.erase(it);
					delete order;
				}

				while (not to_add.empty()) {
					if (to_add.front()->is_sell) resting_sells.push_back(to_add.front());
					else resting_buys.push_back(to_add.front());
					to_add.pop();
				}

				resting_buys.sort([] (Order* a, Order* b) {
					if (a->price == b->price) return a->timestamp < b->timestamp;
					return a->price > b->price;
				});
				resting_sells.sort([] (Order* a, Order* b) {
					if (a->price == b->price) return a->timestamp < b->timestamp;
					return a->price < b->price;
				});
			}
			lock.unlock();
			condition.notify_one();
		}


		ReturnData process(ClientCommand input) {
			ReturnData returned = ReturnData{};
			returned.new_order = nullptr;
			returned.timestamp = getCurrentTimestamp();

			if (input.type == input_cancel) {
				for (auto it = resting_buys.begin(); it != resting_buys.end(); ++it) {
					auto success = (*it)->cancel(input, returned.timestamp);
					if (success != std::nullopt) {
						returned.q.push(*success);
						return returned;
					}
				}

				for (auto it = resting_sells.begin(); it != resting_sells.end(); ++it) {
					auto success = (*it)->cancel(input, returned.timestamp);
					if (success != std::nullopt) {
						returned.q.push(*success);
						return returned;
					}
				}
				PrintOrder order = PrintOrder{};
				order.state = 0;
				order.order_id = input.order_id;
				order.timestamp = returned.timestamp;
				order.is_cancelled = false;
				returned.q.push(order);
				return returned;
			} 
			
			if (input.type == input_buy) {
				for (auto it = resting_sells.begin(); it != resting_sells.end() and input.count > 0; ++it) {
					auto matched = (*it)->match_resting_sell(returned.timestamp, &input);
					if (matched != std::nullopt) returned.q.push(*matched);
				}
			} else {
				for (auto it = resting_buys.begin(); it != resting_buys.end() and input.count > 0; ++it) {
					auto matched = (*it)->match_resting_buy(returned.timestamp, &input);
					if (matched != std::nullopt) returned.q.push(*matched);
				}
			}
			
			if (not input.count) return returned;
			PrintOrder order = PrintOrder{};
			order.state = 2;
			order.order_id = input.order_id;
			order.timestamp = returned.timestamp;
			order.instrument = std::string(input.instrument);
			order.price = input.price;
			order.count = input.count;
			order.is_sell = input.type == input_sell;
			returned.q.push(order);
			returned.new_order = new Order(input, returned.timestamp);
			return returned;
		}
	public:
		~Exchange() {
			for (auto it = resting_buys.begin(); it != resting_buys.end(); ++it) delete *it;
			for (auto it = resting_sells.begin(); it != resting_sells.end(); ++it) delete *it;
		}

		void process_orders(ClientCommand input) {
			enter_bridge(input);
			ReturnData result = process(input);
			leave_bridge(result);
		}
};

#endif