#ifndef __RTABLE_H__
#define __RTABLE_H__ 1

#include "lib.h"

/* Trie implementation for the Routing table */
class RTable {
private:
	class TrieNode {
	public:
		TrieNode *child[2];
		route_table_entry *rt_entry;
		TrieNode() {
			for (int i = 0; i < 2; ++i)
				child[i] = nullptr;
			rt_entry = NULL;
		}
	};

	/* Routing table */
	int rtable_len;
	route_table_entry *rtable;

	/* The maximum number of entries in the RTable */
	static const int MAX_ENTRIES = 100'000;

	/* Root of the Trie */
	TrieNode *root;

	/* Only 1 instance is needed */
	static RTable* instance;
	
	/**
	 * @brief Computed the length of
	 * the @param mask, i.e. if the mask is
	 * 255.255.255.0 => 24
	 * @return returns the length of the mask.
	 */
	int mask_length(uint32_t mask);

	/**
	 * @brief Inserts an entry in the Trie,
	 * using @f maskLength because only the
	 * MSB maskLength of the prefix are important.
	 * 
	 * @param r - routing table entry
	 */
	void insert(route_table_entry& r);

	/**
	 * @brief Deletes Trie.
	 * @param node - the root of the Trie
	 */
	void delete_trie(TrieNode *node) {
		if (!node) return;
		delete_trie(node->child[0]);
		delete_trie(node->child[1]);
		delete node;
	}

	RTable() {
		root = new TrieNode();
		rtable = new route_table_entry[MAX_ENTRIES];
	}

public:

	/* Deleting the copy constructor */
	RTable(const RTable& obj) = delete;

	~RTable() {
		delete[] rtable;
		delete_trie(root);
	}

	static RTable *get_instance() {
		if (instance == nullptr) {
			instance = new RTable();
		}
		return instance;
	}

	/**
	 * @brief Longest Prefix Match algorithm.
	 * @param ip_dest IP Destination
	 * @return returns the longest prefix match entry.
	 */
	route_table_entry *get_best_route(uint32_t ip_dest);

	/** 
	 * @brief Populates a routing table from file.
	 * @param rtable should be allocated.
	 * e.g. rtable = malloc(sizeof(struct route_table_entry) * 80000);
	 * @param path should be the name of a file containing the route table.
	 * @return This function returns the size of the routing table.
	 */
	int read_rtable(const char *path);
};

/* Initialising the instance */
RTable* RTable::instance = nullptr;

#endif /* __RTABLE_H__ */
