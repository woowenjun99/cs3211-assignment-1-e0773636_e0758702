#include <iostream>
#include <thread>

#include "io.hpp"
#include "engine.hpp"

Exchange exchange;

void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

void Engine::connection_thread(ClientConnection connection)	{
	while(true)	{
		ClientCommand input {};
		switch(connection.readInput(input))	{
			case ReadResult::Error: SyncCerr {} << "Error reading input" << std::endl;
			case ReadResult::EndOfFile: return;
			case ReadResult::Success: break;
		}
		exchange.process_orders(input);
	}
}
