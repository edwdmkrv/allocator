#ifndef __LIB_INCLUDED
#define __LIB_INCLUDED

#include <cstdint>
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
class SpecialAllocator {
private:
	using idx_t =
		typename std::conditional<N <= std::numeric_limits<uint8_t>::max(),
			uint8_t,
			typename std::conditional<N <= std::numeric_limits<uint16_t>::max(),
				uint16_t,
				typename std::conditional<N <= std::numeric_limits<uint32_t>::max(),
					uint32_t,
					std::size_t
				>::type
			>::type
		>::type;

	static_assert(sizeof(X) >= sizeof(idx_t),
		"N is too large for type X: sizeof(<the minimal underlying unsigned integral type for N>) must be not larger than sizeof(X)"
	);

	class Data {
	private:
/* Shame on Microsoft: the only one who does not support untagged unions */
#ifdef _MSC_VER
		union U {
#else
		union {
#endif
			X val;
			idx_t idx;
		} (*buf)[N];

		idx_t current;
		idx_t filled;

	public:
		Data() noexcept: buf{}, current{}, filled{} {
		}

		~Data() {
			if (buf) {
				free(buf);
			}
		}

		X *allocate() {
			if (!buf) {
				if (!(buf = reinterpret_cast<decltype(buf)>(malloc(sizeof *buf)))) {
					throw std::bad_alloc();
				}

				idx_t n{};

				for (auto &item: *buf) {
					item.idx = ++n;
				}

				current = 0;
				filled = 0;
			}

			idx_t const allocated = current;

			current = buf[0][current].idx;
			filled++;

			return &buf[0][allocated].val;
		}

		void deallocate(X *const p) noexcept {
			if (filled > 0) {
				idx_t const precurrent{static_cast<idx_t>(p - &buf[0]->val)};

				buf[0][precurrent].idx = current;
				current = precurrent;

				if (!--filled) {
					free(buf);
					buf = nullptr;
					current = 0;
				}
			}
		}

		bool can_allocate() const noexcept {
			return filled != N;
		}

		bool is_valid(X *const p) const noexcept {
			return p >= &buf[0]->val && p < &buf[1]->val;
		}
	};

	std::shared_ptr<Data> data;

public:
	SpecialAllocator() noexcept = default;

	SpecialAllocator(SpecialAllocator &&) noexcept = default;
	SpecialAllocator(SpecialAllocator const &) noexcept(noexcept(std::shared_ptr<Data>(data))) = default;

	SpecialAllocator &operator =(SpecialAllocator &&) = delete;
	SpecialAllocator &operator =(SpecialAllocator const &) = delete;

	X *allocate() {
		if (!data) {
			data = std::make_shared<Data>();
		}

		return data->allocate();
	}

	void deallocate(X *const p) noexcept {
		if (data) {
			data->deallocate(p);
		}
	}

	bool can_allocate() const noexcept {
		return !data || data->can_allocate();
	}

	bool is_valid(X *const p) const noexcept {
		return data && data->is_valid(p);
	}
};

template <typename X, std::size_t N>
class AllocatorN {
private:
	SpecialAllocator<X, N> allocator;

public:
	using value_type = X;
	using size_type = std::size_t;

	AllocatorN() noexcept = default;

	AllocatorN(AllocatorN<X, N> &&) noexcept = default;
	AllocatorN(AllocatorN<X, N> const &) = default;

	template <typename Y, std::size_t M>
	AllocatorN &operator =(AllocatorN<Y, M> &&) = delete;

	template <typename Y, std::size_t M>
	AllocatorN &operator =(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> &&) = delete;

	template <typename Y, std::size_t M = N>
	struct rebind {
		using other = AllocatorN<Y, M>;
	};

	X *allocate(size_type const n) {
		if (n == 1 && allocator.can_allocate()) {
			return allocator.allocate();
		} else {
			void *const p{malloc(n * sizeof *allocate(n))};

			if (!p) {
				throw std::bad_alloc{};
			}

			return reinterpret_cast<X *>(p);
		}
	}

	void deallocate(X *const p, size_type const n) noexcept {
		if (n == 1 && allocator.is_valid(p)) {
			allocator.deallocate(p);
		} else {
			free(p);
		}
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
	ptr{},
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
	Container() noexcept(noexcept(A{})) : ContainerExcecutive<X, A>{AllocatorWrapper<X, A>::allocator} {
	}

	~Container() {
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
