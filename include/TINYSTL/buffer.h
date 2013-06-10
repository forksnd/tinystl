/*-
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TINYSTL_BUFFER_H
#define TINYSTL_BUFFER_H

#include <TINYSTL/allocator.h>
#include <TINYSTL/new.h>
#include <TINYSTL/traits.h>

namespace tinystl {

	template<typename T, typename Alloc = TINYSTL_ALLOCATOR>
	struct buffer {
		T* first;
		T* last;
		T* capacity;
	};

	template<typename T>
	static inline void buffer_destroy_range_traits(T* first, T* last, pod_traits<T, false>) {
		for (; first < last; ++first)
			first->~T();
	}

	template<typename T>
	static inline void buffer_destroy_range_traits(T*, T*, pod_traits<T, true>) {
	}

	template<typename T>
	static inline void buffer_destroy_range(T* first, T* last) {
		buffer_destroy_range_traits(first, last, pod_traits<T>());
	}

	template<typename T>
	static inline void buffer_fill_urange_traits(T* first, T* last, const T& value, pod_traits<T, false>) {
		for (; first < last; ++first)
			new(placeholder(), first) T(value);
	}

	template<typename T>
	static inline void buffer_fill_urange_traits(T* first, T* last, const T& value, pod_traits<T, true>) {
		for (; first < last; ++first)
			*first = value;
	}

	template<typename T>
	static inline void buffer_move_urange_traits(T* dest, T* first, T* last, pod_traits<T, false>) {
		for (T* it = first; it != last; ++it, ++dest)
			move_construct(dest, *it);
		buffer_destroy_range(first, last);
	}

	template<typename T>
	static inline void buffer_move_urange_traits(T* dest, T* first, T* last, pod_traits<T, true>) {
		for (; first != last; ++first, ++dest)
			*dest = *first;
	}

	template<typename T>
	static inline void buffer_bmove_urange_traits(T* dest, T* first, T* last, pod_traits<T, false>) {
		dest += (last - first);
		for (T* it = last; it != first; --it, --dest) {
			move_construct(dest - 1, *(it - 1));
			buffer_destroy_range(it - 1, it);
		}
	}

	template<typename T>
	static inline void buffer_bmove_urange_traits(T* dest, T* first, T* last, pod_traits<T, true>) {
		dest += (last - first);
		for (T* it = last; it != first; --it, --dest)
			*(dest - 1) = *(it - 1);
	}

	template<typename T>
	static inline void buffer_move_urange(T* dest, T* first, T* last) {
		buffer_move_urange_traits(dest, first, last, pod_traits<T>());
	}

	template<typename T>
	static inline void buffer_bmove_urange(T* dest, T* first, T* last) {
		buffer_bmove_urange_traits(dest, first, last, pod_traits<T>());
	}

	template<typename T>
	static inline void buffer_fill_urange(T* first, T* last, const T& value) {
		buffer_fill_urange_traits(first, last, value, pod_traits<T>());
	}

	template<typename T, typename Alloc>
	static inline void buffer_init(buffer<T, Alloc>* b) {
		b->first = b->last = b->capacity = 0;
	}

	template<typename T, typename Alloc>
	static inline void buffer_destroy(buffer<T, Alloc>* b) {
		buffer_destroy_range(b->first, b->last);
		Alloc::static_deallocate(b->first, (size_t)((char*)b->first - (char*)b->last));
	}

	template<typename T, typename Alloc>
	static inline void buffer_reserve(buffer<T, Alloc>* b, size_t capacity) {
		if (b->first + capacity <= b->capacity)
			return;

		typedef T* pointer;
		const size_t size = (size_t)(b->last - b->first);
		pointer newfirst = (pointer)Alloc::static_allocate(sizeof(T) * capacity);
		buffer_move_urange(newfirst, b->first, b->last);
		Alloc::static_deallocate(b->first, sizeof(T) * capacity);

		b->first = newfirst;
		b->last = newfirst + size;
		b->capacity = newfirst + capacity;
	}

	template<typename T, typename Alloc>
	static inline void buffer_resize(buffer<T, Alloc>* b, size_t size, const T& value) {
		buffer_reserve(b, size);

		buffer_fill_urange(b->last, b->first + size, value);
		buffer_destroy_range(b->first + size, b->last);
		b->last = b->first + size;
	}

	template<typename T, typename Alloc>
	static inline void buffer_clear(buffer<T, Alloc>* b) {
		buffer_destroy_range(b->first, b->last);
		b->last = b->first;
	}

	template<typename T, typename Alloc>
	static inline void buffer_insert(buffer<T, Alloc>* b, T* where, const T* first, const T* last) {
		const size_t offset = (size_t)(where - b->first);
		const size_t newsize = (size_t)((b->last - b->first) + (last - first));
		if (b->first + newsize > b->capacity)
			buffer_reserve(b, (newsize * 3) / 2);

		typedef T* pointer;
		where = b->first + offset;
		const size_t count = (size_t)(last - first);

		if (where != b->last)
			buffer_bmove_urange(where + count, where, b->last);

		for (; first != last; ++first, ++where)
			new(placeholder(), where) T(*first);

		b->last = b->first + newsize;
	}

	template<typename T, typename Alloc>
	static inline T* buffer_erase(buffer<T, Alloc>* b, T* first, T* last) {
		typedef T* pointer;
		for (pointer it = last, end = b->last, dest = first; it != end; ++it, ++dest)
			move(*dest, *it);

		buffer_destroy_range(b->last - (last - first), b->last);

		b->last -= (last - first);
		return first;
	}

	template<typename T, typename Alloc>
	static inline void buffer_swap(buffer<T, Alloc>* b, buffer<T, Alloc>* other) {
		typedef T* pointer;
		const pointer tfirst = b->first, tlast = b->last, tcapacity = b->capacity;
		b->first = other->first, b->last = other->last, b->capacity = other->capacity;
		other->first = tfirst, other->last = tlast, other->capacity = tcapacity;
	}
}

#endif
