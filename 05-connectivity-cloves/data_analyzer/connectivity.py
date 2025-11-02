import os
import sys
import re
import numpy as np

def parse_process_file(log_file):
    idsmap = dict()
    tx = dict()
    rx = dict()
    rssi = dict()

    # Regular expressions to parse the file
    regex_id = re.compile(r".* (?P<node_id>\d+).firefly < b'Rime configured with address (?P<addr>\w\w:\w\w).*\n")
    regex_tx = re.compile(r".*TX (?P<addr>\w\w:\w\w).*\n")
    regex_rx = re.compile(r".*RX (?P<addr_from>\w\w:\w\w)->(?P<addr_to>\w\w:\w\w), RSSI = (?P<rssi>-?\d+)dBm.*\n")

    # Open the log file and read it line by line to match Rime short addresses to testbed IDs
    with open(log_file, 'r') as f:
        for line in f:

            # Match Rime short addresses strings and map them to testbed IDs
            m = regex_id.match(line)
            if m:

                # Get dictionary of matched groups (map the group names to their captured values)
                d = m.groupdict()

                # Add to the dictionary
                idsmap[d['addr']] = d['node_id']

    # Open the log file and read line by line to count TX and RX
    with open(log_file, 'r') as f:
        for line in f:

            # Match transmissions strings
            m = regex_tx.match(line)
            if m:

                # Get dictionary of matched groups
                d = m.groupdict()

                # Increase the count of transmissions from this address
                try:
                    tx_id = idsmap[d['addr']]
                    tx[tx_id] = tx.get(tx_id, 0) + 1
                except KeyError:
                    print(f"ID not found for address {d['addr']}")

                continue # If there is a match, go to the next line

            # Match reception strings
            m = regex_rx.match(line)
            if m:

                # Get dictionary of matched groups
                d = m.groupdict()

                # Increase the count of receptions for the given link
                key_found = True
                try:
                    from_id = idsmap[d['addr_from']]
                except KeyError:
                    print(f"ID not found for address {d['addr_from']}")
                    key_found = False
                try:
                    to_id = idsmap[d['addr_to']]
                except KeyError:
                    print(f"ID not found for address {d['addr_to']}")
                    key_found = False

                if key_found:
                    rx_update = rx.get(from_id, dict())
                    rx_update[to_id] = rx_update.get(to_id, 0) + 1
                    rx[from_id] = rx_update

                    # Collect RSSI measurements for the given link
                    rssi_update = rssi.get(from_id, dict())
                    rssi_update[to_id] = rssi_update.get(to_id, 0) + int(d['rssi'])
                    rssi[from_id] = rssi_update

                continue

    # Diplay results
    ratios = []
    rssis = []
    for tx_id in tx:
        print(f"FROM: {tx_id:<5}\t\t")
        for rx_id in rx[tx_id]:
            curr_ratio = rx[tx_id][rx_id] * 100 / tx[tx_id]
            ratios.append(curr_ratio)
            curr_rssi = rssi[tx_id][rx_id]
            rssis.append(curr_rssi)
            print(f"\tTO: {rx_id:<5}\t#  {curr_ratio:.2f}% RSSI {curr_rssi}")
        print("\n")

    print(f"{len(ratios)} {len(rssis)} {ratios[0]} {rssis[0]}")
    print(f"correlation {np.corrcoef(ratios, rssis)}")


if __name__ == '__main__':
    # Check that the file to be parsed and processed is specified (e.g., python3 connectivity.py job.log)
    if len(sys.argv) < 2:
        print("Error: Missing log file.")
        sys.exit(1)

    # Get the log path and check that it exists
    log_file = sys.argv[1]
    if not os.path.isfile(log_file) or not os.path.exists(log_file):
        print("Error: Log file not found.")
        sys.exit(1)

    # Parse and process the log file
    parse_process_file(log_file)
