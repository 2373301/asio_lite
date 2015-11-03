#pragma once

#include <string>

inline unsigned int ELFHash(const char* str, unsigned int len)
{
	unsigned int hash  =   0 ;
	unsigned int g     =   0 ;
	for (unsigned int i = 0; i < len; ++i)
	{
		char temp = str[i];
		hash = (hash << 4) + (temp * 13);
		g = hash & 0xf0000000;

		if (g)
		{
			hash ^= (g >> 24);
			hash ^= g;
		}
	}
	return hash;
}

template<typename Key>
inline unsigned int HashKey(const Key& key)
{
	return (unsigned int)key;
}

template<>
inline unsigned int HashKey<std::string>(const std::string& key)
{
	return ELFHash(key.c_str(), key.length());
}

template<>
inline unsigned int HashKey<unsigned long long>(const unsigned long long& key)
{
	return ELFHash((const char*)&key, sizeof(unsigned long long));
}

template<typename Key, typename Val>
class hash_map
{
	struct hash_node;
public:
	hash_map() : node_ptrs_(0), slot_size_(0), size_(0)
	{
	}

	~hash_map()
	{
		clear();
		delete [] node_ptrs_;
	}

	void init(unsigned int slot_size)
	{
		slot_size_ = slot_size;
		node_ptrs_ = new hash_node*[slot_size_];
		memset(node_ptrs_, 0, sizeof(hash_node*) * slot_size_);
	}

	void insert(const Key& key, const Val& val)
	{
		unsigned int pos = HashKey(key) % slot_size_;

		hash_node* node = new hash_node;
		node->key = key;
		node->val = val;

		node->next = node_ptrs_[pos];
		node_ptrs_[pos] = node;

		++size_;
	}

	void erase(const Key& key)
	{
		unsigned int pos = HashKey(key) % slot_size_;
		hash_node* node = node_ptrs_[pos];
		hash_node* node_prev = 0;
		while (node != 0)
		{
			if (node->key == key)
			{
				if (node_prev == 0)
				{
					node_ptrs_[pos] = node->next;
				}
				else
				{
					node_prev->next = node->next;
				}
				delete node;
				--size_;
			}
			node_prev = node;
			node = node->next;
		}
	}

	bool find(const Key& key, Val* val = 0)
	{
		unsigned int pos = HashKey(key) % slot_size_;
		hash_node* node = node_ptrs_[pos];
		while (node != 0)
		{
			if (node->key == key)
			{
				if (val != 0)
				{
					*val = node->val;
				}
				return true;
			}
			node = node->next;
		}
		return false;
	}

	void clear()
	{
		for (unsigned int i = 0; i < slot_size_; ++i)
		{
			hash_node* node = node_ptrs_[i];
			while (node != 0)
			{
				hash_node* temp = node;
				node = node->next;
				delete temp;
			}
		}
		size_ = 0;
		memset(node_ptrs_, 0, sizeof(hash_node*) * slot_size_);
	}

	unsigned int size()
	{
		return size_;
	}
	
private:
	struct hash_node
	{
		Key key;
		Val val;
		hash_node* next;
	};

	hash_node** node_ptrs_;
	unsigned int slot_size_;
	unsigned int size_;
};
