#pragma once

#include <vector>

#define SPACE_CONSISTENT_VECTOR_RANGE_SIZE 128

template<typename T>
class space_consistent_vector
{
public:
	// element access
	T& at(size_t index)
	{
		size_t rangeIndex = static_cast<size_t>(index / SPACE_CONSISTENT_VECTOR_RANGE_SIZE);
		index = index % SPACE_CONSISTENT_VECTOR_RANGE_SIZE;
		return m_data[rangeIndex][index];
	}
	T& operator[](size_t index) { return at(index); }
	T& front() { return m_data.front().front(); }
	T& back() { return m_data.back().back(); }

	// iterators
	// iterator begin();
	// iterator end();

	// capacity
	const bool empty() { return size() == 0; }
	const size_t size() { return m_size; }
	const size_t capacity() { return m_data.size() * SPACE_CONSISTENT_VECTOR_RANGE_SIZE; }
	void reserve(size_t size) { while (capacity() < size) new_range(); }

	// modifiers
	void clear()
	{
		m_data.clear();
		m_size = 0;
	}
	void insert(size_t pos, T element)
	{
		size_t cur = size() - 1;
		push_back(at(cur));
		while (cur > pos)
			at(cur) = at(--cur);
		at(pos) = element;

		m_size++;
	}
	void erase(size_t pos)
	{
		size_t cur = pos;
		while (cur < size() - 1)
			at(cur) = at(++cur);
		pop_back();

		m_size--;
	}
	void push_back(T element)
	{
		if (m_data.empty() || m_data.back().size() == SPACE_CONSISTENT_VECTOR_RANGE_SIZE)
			new_range();
		m_data.back().push_back(element);

		m_size++;
	}
	void pop_back()
	{
		m_data.back().pop_back();
		if (m_data.back().empty())
			m_data.pop_back();

		m_size--;
	}
	void resize(size_t size)
	{
		size_t rangeCount = static_cast<size_t>(size / SPACE_CONSISTENT_VECTOR_RANGE_SIZE) + 1;
		m_data.resize(rangeCount);
		for (auto& range : m_data)
			range.resize(SPACE_CONSISTENT_VECTOR_RANGE_SIZE);
		m_data.back().resize(size % SPACE_CONSISTENT_VECTOR_RANGE_SIZE);

		m_size = size;
	}

private:

	void new_range()
	{
		m_data.push_back(std::vector<T>());
		m_data.back().reserve(SPACE_CONSISTENT_VECTOR_RANGE_SIZE);
	}

	std::vector<std::vector<T>> m_data;
	size_t m_size;

};