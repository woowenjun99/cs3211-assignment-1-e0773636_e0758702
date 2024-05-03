from random import randint
from subprocess import run
from collections import defaultdict

COMMAND_TYPE = ["B", "S", "C"]

with open("generated.in", "w") as f:
    """
    Specify the parameters

    Number of threads to open
    Number of orders to generate
    Number of instruments (can leave it as integer for now)
    Whether we need buy
    """
    # edit this and run `python3 script.py`
    num_threads = 40
    test_cases = 2000
    num_instruments = 50
    f.write(f"{str(num_threads)} \n")

    if num_threads == 1: f.write("o\n")
    else: 
        for i in range(num_threads): f.write(f"{i} o\n")

    # Lets try to divide evenly
    existing_orders = []
    counter = 1
    for i in range(test_cases):

        # If there is nothing to cancel, we choose buy or sell. Otherwise we can try cancelling.
        selected_command_type = None
        if not len(existing_orders): selected_command_type = COMMAND_TYPE[randint(0, 1)]
        else: selected_command_type = COMMAND_TYPE[randint(0, 2)]

        # If we have to cancel
        if selected_command_type == "C":
            index_to_cancel = randint(0, len(existing_orders) - 1)
            if index_to_cancel != len(existing_orders) - 1:
                existing_orders[index_to_cancel], existing_orders[-1] = existing_orders[-1], existing_orders[index_to_cancel]
            thread_id, order_id = existing_orders.pop()
            if num_threads == 1: f.write(f"C {order_id}\n")
            else: f.write(f"{thread_id} C {order_id}\n")
            continue

        price = randint(1, 5000)
        quantity = randint(1, 100)
        thread_id = randint(0, num_threads - 1)
        instrument = randint(0, num_instruments - 1)

        # Delete the instrument
        existing_orders.append([thread_id, counter])
        if num_threads == 1:
            f.write(f"{selected_command_type} {counter} {instrument} {price} {quantity}\n")
        else:
            f.write(f"{thread_id} {selected_command_type} {counter} {instrument} {price} {quantity}\n")
        counter += 1

    # Closing the threads
    if num_threads == 1: f.write("x\n")
    else:
        for i in range(num_threads): f.write(f"{i} x\n")

# run(["chmod", "+x", "./runtests.sh"])
# run(["./runtests.sh"])
# run(["rm", "-f", "generated.in"])