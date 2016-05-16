u32 pending;

pending = local_softirq_pending();
if (pending) {
	struct softirq_action *h;

	/* reset the pending bitmask */
	set_softirq_pending();

	h = softirq_vec;
	do {
		if (pending & 1)
			h->action(h);
		h++;
		pending >>= 1;
	} while (pending);
}
