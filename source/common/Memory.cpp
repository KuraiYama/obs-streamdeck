/*
 * Plugin Includes
 */
#include "include/common/Memory.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Memory::Memory() :
	m_allocate(false),
	m_internalMemory(nullptr),
	m_internalPointer(nullptr),
	m_internalSize(0) {
}

Memory::Memory(size_t size) :
	m_allocate(false) {
	alloc(size);
}

Memory::Memory(byte* external_memory, size_t external_size) :
	m_allocate(false) {
	lock(external_memory, external_size);
}

Memory::Memory(const Memory& memory) {
	m_allocate = memory.m_allocate;
	Memory& tmp = const_cast<Memory&>(memory);
	tmp.m_allocate = false;
	m_internalMemory = memory.m_internalMemory;
	m_internalPointer = memory.m_internalPointer;
	m_internalSize = memory.m_internalSize;
}

Memory::~Memory() {
	if(m_allocate)
		free();
	unlock();
}

/*
========================================================================================================
	Operators
========================================================================================================
*/

Memory::operator byte*() const {
	return this->m_internalMemory;
}

Memory&
Memory::operator=(const Memory& memory) {
	m_allocate = memory.m_allocate;
	Memory& tmp = const_cast<Memory&>(memory);
	tmp.m_allocate = false;
	m_internalMemory = memory.m_internalMemory;
	m_internalPointer = memory.m_internalPointer;
	m_internalSize = memory.m_internalSize;

	return *this;
}

/*
========================================================================================================
	Memory Handling
========================================================================================================
*/

Memory&
Memory::alloc(size_t size) {
	m_internalSize = size;
	m_internalMemory = (byte*)malloc(m_internalSize);
	if(m_internalMemory != nullptr) {
		m_internalPointer = m_internalMemory;
		memset(m_internalPointer, 0, m_internalSize);
	}
	m_allocate = true;
	return *this;
}

void
Memory::free() {
	::free((void*)m_internalMemory);
	unlock();
}


byte*
Memory::lock(byte* memory, size_t size) {
	if(m_allocate) free();
	m_allocate = false;
	m_internalMemory = memory;
	m_internalSize = size;
	m_internalPointer = m_internalMemory;
	return m_internalMemory;
}

byte*
Memory::unlock() {
	byte* ptr = m_internalMemory;
	m_allocate = false;
	m_internalMemory = nullptr;
	m_internalPointer = nullptr;
	m_internalSize = 0;
	return ptr;
}

void
Memory::reset() {
	m_internalPointer = m_internalMemory;
}

size_t
Memory::write(byte* src, size_t size) {
	size_t max_size = (m_internalMemory + m_internalSize) - m_internalPointer;
	if(max_size <= 0) return 0;
	size = std::min<size_t>(size, max_size);
	memcpy(m_internalPointer, src, size);
	m_internalPointer += size;
	return size;
}

size_t
Memory::read(byte* dst, size_t size) {
	size_t max_size = (m_internalMemory + m_internalSize) - m_internalPointer;
	if(max_size <= 0) return 0;
	size = std::min<size_t>(size, max_size);
	memcpy(dst, m_internalPointer, size);
	m_internalPointer += size;
	return size;
}

byte*
Memory::seek(size_t size) {
	if(size < m_internalSize)
		m_internalPointer = m_internalMemory + size;

	return m_internalPointer;
}

byte*
Memory::tell() const {
	return m_internalPointer;
}

size_t
Memory::size() const {
	return m_internalSize;
}