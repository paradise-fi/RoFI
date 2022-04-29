packet_counter = dict()

def filter(packet, sender, receiver):
    global packet_counter
    last_value = packet_counter.get(sender.module_id, 0)
    packet_counter[sender.module_id] = last_value + 1
    if last_value % 2 == 0:
        return packet, 0
    else:
        return packet, -1
