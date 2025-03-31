#include "rtable.h"

int RTable::mask_length(uint32_t mask) {
	uint32_t res = 0;
	for (int i = 31; i >= 0; --i) {
		if (mask & (1 << i))
			++res;
		else
			break;
	}
	return res;
}

void RTable::insert(route_table_entry& r) {
	uint32_t prefix = ntohl(r.prefix);
	TrieNode *elem = root;
	int length = mask_length(ntohl(r.mask));
	for (int i = 0; i < length; ++i) {
		int bit = (prefix & (1 << (31 - i))) ? 1 : 0;
		if (!elem->child[bit])
			elem->child[bit] = new TrieNode();
		elem = elem->child[bit];
	}
	elem->rt_entry = &r;
}

route_table_entry* RTable::get_best_route(uint32_t ip_dest) {
	TrieNode *elem = root;
	ip_dest = ntohl(ip_dest);
	route_table_entry *res = NULL;
	for (int i = 31; i >= 0; --i) {
		int bit = (ip_dest >> i) & 1;
		if (!elem->child[bit])
			break;
		elem = elem->child[bit];
		if (elem->rt_entry)
			res = elem->rt_entry;
	}
	return res;
}

int RTable::read_rtable(const char *path) {
	FILE *fp = fopen(path, "r");
	int j = 0, i;
	char *p, line[64];

	while (fgets(line, sizeof(line), fp) != NULL) {
		p = strtok(line, " .");
		i = 0;
		while (p != NULL) {
			if (i < 4)
				*(((unsigned char *)&rtable[j].prefix)  + i % 4) = (unsigned char)atoi(p);

			if (i >= 4 && i < 8)
				*(((unsigned char *)&rtable[j].next_hop)  + i % 4) = atoi(p);

			if (i >= 8 && i < 12)
				*(((unsigned char *)&rtable[j].mask)  + i % 4) = atoi(p);

			if (i == 12)
				rtable[j].interface = atoi(p);
			p = strtok(NULL, " .");
			i++;
		}
		insert(rtable[j]);
		j++;
	}

	fclose(fp);
	return j;
}
