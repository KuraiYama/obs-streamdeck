#pragma once

/*
 * Std Includes
 */
#include <map>
#include <vector>
#include <memory>
#include <algorithm>

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef char byte;

class Memory {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		size_t m_internalSize;

		byte* m_internalMemory;

		byte* m_internalPointer;

		bool m_allocate;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Memory();

		Memory(const Memory& memory);

		Memory(size_t size);

		Memory(byte* external_memory, size_t external_size);

		~Memory();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		byte*
		lock(byte* memory, size_t size);

		byte*
		unlock();

		Memory&
		alloc(size_t size);

		void
		free();

		void
		reset();

		size_t
		write(byte* src, size_t size);

		size_t
		read(byte* dst, size_t size);

		byte*
		seek(size_t size);

		byte*
		tell() const;

		size_t
		size() const;

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		operator byte*() const;

		Memory& operator=(const Memory&);

};