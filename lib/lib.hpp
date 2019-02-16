#ifndef __LIB_INCLUDED
#define __LIB_INCLUDED

#include <cstdlib>

#include <new>
#include <memory>
#include <utility>
#include <limits>
#include <type_traits>

#include <iostream>

#include <version.hpp>

/* Version stuff
 */
static inline std::string ver() {
	return PROJECT_VERSION;
}

static inline unsigned major() {
	return PROJECT_VERSION_MAJOR;
}

static inline unsigned minor() {
	return PROJECT_VERSION_MINOR;
}

static inline unsigned patch() {
	return PROJECT_VERSION_PATCH;
}

/* Allocator stuff
 */
template <typename X, std::size_t N>
class AllocatorN {
private:

public:
	using value_type = X;
	using size_type = std::size_t;

	AllocatorN() noexcept {
	}

	AllocatorN(AllocatorN<X, N> &&) = default;
	AllocatorN(AllocatorN<X, N> const &) = default;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> &&) = delete;

	template <typename Y, std::size_t M = N>
	struct rebind {
		using other = AllocatorN<Y, M>;
	};

	X *allocate(size_type const n) {
		void *const p{malloc(n * sizeof *allocate(n))};

		std::cout << __PRETTY_FUNCTION__ << ", n = " << n << ", p = " << p << std::endl;

		if (!p) {
			throw std::bad_cast{};
		}

		return reinterpret_cast<X *>(p);
	}

	void deallocate(X *const p, size_type const n) noexcept {
		std::cout << __PRETTY_FUNCTION__ << ", n = " << n << ", p = " << p << std::endl;
		free(p);
	}
};

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator ==(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return std::is_same<X, Y>::value && N == M && &x == &y;
}

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator !=(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return !operator ==(x, y);
}

/* Container stuff
 */
template <typename X, typename A = std::allocator<X>>
class ContainerExcecutive {
private:
	static_assert(std::is_nothrow_destructible<X>::value,
		"Types with destructors that can throw exceptions are not supported"
	);

	A &allocator;
	X *ptr;
	std::size_t asize;
	std::size_t dsize;

	static constexpr std::size_t min_size(std::size_t const numerator, std::size_t const denominator, std::size_t const val = 0) {
		return (val * numerator + denominator / 2) / denominator > val ? val : min_size(numerator, denominator, val + 1);
	}

	std::size_t new_size() const {
		enum: std::size_t {
			size_t_max = std::numeric_limits<std::size_t>::max(),
			numerator = 21,
			denominator = 13
		}; // 21 / 13 ~ (sqrt(5) + 1) / 2 (golden section)

		if (asize == size_t_max) {
			throw std::bad_alloc{};
		}

		return
			asize == 0 ?
				min_size(numerator, denominator) :
			asize < (size_t_max - denominator / 2) / numerator ?
				(asize * numerator + denominator / 2) / denominator :
			asize < size_t_max / numerator * denominator ?
				asize / denominator * numerator :
				size_t_max;
	}

protected:
	explicit ContainerExcecutive(A &allocator) noexcept:
	allocator{allocator},
	ptr{nullptr},
	asize{},
	dsize{} {
	}

	ContainerExcecutive(A &allocator, std::size_t const asize):
	allocator{allocator},
	ptr{std::allocator_traits<A>::allocate(allocator, asize)},
	asize{asize},
	dsize{} {
	}

	~ContainerExcecutive() {
		while (dsize) {
			std::allocator_traits<A>::destroy(allocator, ptr + --dsize);
		}

		std::allocator_traits<A>::deallocate(allocator, ptr, asize);
	}

	void reserve(std::size_t const new_cap) {
		if (new_cap > asize) {
			if (asize) {
				ContainerExcecutive<X, A> vec{allocator, new_cap};

				for (; vec.dsize < dsize; vec.dsize++) {
					std::cout << __PRETTY_FUNCTION__ << ": " << this << ", n = " << vec.dsize << ", vec = " << &vec << std::endl;
					if constexpr (std::is_nothrow_move_constructible<X>::value) {
						std::allocator_traits<A>::construct(allocator, &vec.ptr[vec.dsize], std::move(ptr[vec.dsize]));
					} else {
						std::allocator_traits<A>::construct(allocator, &vec.ptr[vec.dsize], ptr[vec.dsize]);
					}
				}

				std::swap(ptr, vec.ptr);
				std::swap(asize, vec.asize);
				std::swap(dsize, vec.dsize);
			} else {
				ptr = std::allocator_traits<A>::allocate(allocator, new_cap);
				asize = new_cap;
			}
		}

	}

	template <typename... Y>
	X &emplace_back(Y &&... y) {
		if (dsize == asize) {
			reserve(new_size());
		}

		std::allocator_traits<A>::construct(allocator, ptr + dsize, std::forward<Y>(y)...);
		return ptr[dsize++];
	}

	X *begin() noexcept {
		return &ptr[0];
	}

	X *end() noexcept {
		return &ptr[dsize];
	}

	bool empty() const noexcept {
		return !ptr;
	}

	std::size_t capacity() const noexcept {
		return asize;
	}

	std::size_t size() const noexcept {
		return dsize;
	}
};

template <typename X, typename A = std::allocator<X>>
class AllocatorWrapper {
protected:
	A allocator;
};

template <typename X, typename A = std::allocator<X>>
class Container: private AllocatorWrapper<X, A>, protected ContainerExcecutive<X, A> {
public:
	Container() noexcept : ContainerExcecutive<X, A>{AllocatorWrapper<X, A>::allocator} {
		std::cout << __PRETTY_FUNCTION__ << ": " << this << std::endl;
	}

	~Container() {
		std::cout << __PRETTY_FUNCTION__ << ": " << this << std::endl;
	}

	using ContainerExcecutive<X, A>::reserve;
	using ContainerExcecutive<X, A>::emplace_back;
	using ContainerExcecutive<X, A>::begin;
	using ContainerExcecutive<X, A>::end;
	using ContainerExcecutive<X, A>::empty;
	using ContainerExcecutive<X, A>::capacity;
	using ContainerExcecutive<X, A>::size;
};

#endif
